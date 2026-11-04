/**
 * Test Everward 3D JS bridge against the independently compiled authoritative
 * C++ headers on *the same git commit*. No precomputed hand-copied pose fixtures.
 * No networking, npm install, browser or Unreal required.
 */
import test from 'node:test';
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import {mkdtempSync, rmSync} from 'node:fs';
import path from 'node:path';
import os from 'node:os';
import {fileURLToPath} from 'node:url';
import {DESIGN, forwardKinematics} from './model.mjs';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const ROOT = path.resolve(HERE, '../../..');
const TMP = mkdtempSync(path.join(os.tmpdir(), 'everward-oracle-'));
const EXE = path.join(TMP, process.platform === 'win32' ? 'oracle.exe' : 'oracle');
const CXX = process.env.CXX || (process.platform === 'win32' ? 'g++' : 'c++');
try {
  execFileSync(CXX, [
    '-std=c++20', '-O1',
    '-I' + path.join(ROOT, 'src/simulation/include'),
    path.join(HERE, 'bridge_oracle.cpp'), '-o', EXE,
  ], {timeout:120000, stdio:['ignore','pipe','pipe']});
} catch (e) {
  rmSync(TMP, {recursive:true, force:true});
  throw new Error('C++ oracle did not compile: ' + String(e.stderr || e.message));
}

const oracle = (mode) => JSON.parse(execFileSync(EXE, [mode], {encoding:'utf8',timeout:30000}));
const close = (actual, expected, label) => {
  assert.ok(Number.isFinite(actual));
  assert.ok(Math.abs(actual - expected) <= 1e-9,
    label + ': JS=' + actual + ' C++=' + expected);
};

test('the checked-in design contract is exactly the C++ contract', () => {
  const actual = oracle('contract');
  for (const key of [
    'shoulder_m', 'link_m', 'sample_radius_m',
    'fold_pitch_degrees', 'fold_roll_degrees', 'joint_limits_degrees',
  ]) {
    assert.deepEqual(DESIGN[key], actual[key], 'C++ layout drift in ' + key);
  }
});

test('40 independent C++ oracle poses equal JS 3D FK: BOTH arms, unfolding, arbitrary joints', () => {
  const cases = oracle('poses').cases;
  assert.equal(cases.length, 40);
  for (const c of cases) {
    const p = forwardKinematics({
      arm:c.arm,
      deploymentFraction:c.deployment_fraction,
      anglesDegrees:c.angles_degrees,
    });
    for (const [name, key] of [
      ['elbow', 'elbow_m'], ['wrist', 'wrist_m'], ['toolTip', 'tool_tip_m'],
    ]) {
      c[key].forEach((value, axis) => close(
        p[name][axis], value, c.arm + '/' + c.deployment_fraction + '/' + name + axis,
      ));
    }
    close(p.toolTipRadiusM, c.contact_radius_m, 'contact radius');
  }
});

// Test runner cleanup after all synchronous assertions, including failure.
process.on('exit', () => rmSync(TMP, {recursive:true, force:true}));
