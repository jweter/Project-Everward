#!/usr/bin/env node
// A real compiled C++ oracle, not a copied constant or actor transform.
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import {existsSync} from 'node:fs';
import {DESIGN, forwardKinematics} from '../../procedural_mechanics/prime-arm/model.mjs';

const executable = process.argv[2];
if (!executable || !existsSync(executable)) {
  throw new Error('Require path to compiled C++ bridge oracle, from same repo checkout');
}
const oracle = mode => JSON.parse(execFileSync(executable, [mode], {
  timeout: 30000, encoding: 'utf8', windowsHide: true,
}));
const exact = oracle('contract');
for (const key of [
  'shoulder_m','link_m','sample_radius_m','fold_pitch_degrees',
  'fold_roll_degrees','joint_limits_degrees',
]) assert.deepEqual(DESIGN[key], exact[key], 'C++ source layout drift: ' + key);
const poses = oracle('poses').cases;
assert.equal(poses.length, 40, 'C++ oracle must provide 40 varied poses');
let samples = 0;
for (const item of poses) {
  const pose = forwardKinematics({
    arm:item.arm, deploymentFraction:item.deployment_fraction,
    anglesDegrees:item.angles_degrees,
  });
  for (const [name, key] of [
    ['elbow','elbow_m'],['wrist','wrist_m'],['toolTip','tool_tip_m'],
  ]) {
    item[key].forEach((expected, axis) => {
      const got = pose[name][axis];
      assert.ok(Number.isFinite(got) && Math.abs(got-expected) <= 1e-9,
        'C++ pose mismatch: '+item.arm+'/'+name+'/'+axis+' got='+got+' expected='+expected);
      samples++;
    });
  }
  assert.ok(Math.abs(pose.toolTipRadiusM - item.contact_radius_m) <= 1e-9);
}
console.log('EVERWARD_CPP_BRIDGE_PARITY=PASS');
console.log('EVERWARD_CPP_BRIDGE_ORACLE_POSES='+poses.length);
console.log('EVERWARD_CPP_BRIDGE_COORDINATE_CHECKS='+samples);
