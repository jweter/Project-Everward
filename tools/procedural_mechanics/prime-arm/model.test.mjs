import test from 'node:test';
import assert from 'node:assert/strict';
import {DESIGN, forwardKinematics, poseAt} from './model.mjs';

const epsilon = 1e-11;
test('straight arm has a predictable finite full extension', () => {
  const out = forwardKinematics({shoulderRad: 0, elbowRad: 0, wristRad: 0});
  assert.ok(Math.abs(out.tip.x) < epsilon);
  assert.ok(Math.abs(out.tip.y + out.maximumReachM) < epsilon);
  assert.ok(Math.abs(out.nominalReachM - out.maximumReachM) < epsilon);
});

test('right angle at shoulder agrees with coordinate convention', () => {
  const out = forwardKinematics({
    shoulderRad: Math.PI / 2, elbowRad: 0, wristRad: 0,
  });
  assert.ok(Math.abs(out.tip.x - out.maximumReachM) < epsilon);
  assert.ok(Math.abs(out.tip.y) < epsilon);
});

test('every film frame has finite joints and a physical nominal reach', () => {
  for (let n = 0; n <= 24 * 120; n++) {
    const p = poseAt(n / 120);
    const {shoulderRad, elbowRad, wristRad} = p.angles;
    assert.ok(shoulderRad >= DESIGN.shoulderMinRad && shoulderRad <= DESIGN.shoulderMaxRad);
    assert.ok(elbowRad >= DESIGN.elbowMinRad && elbowRad <= DESIGN.elbowMaxRad);
    assert.ok(wristRad >= DESIGN.wristMinRad && wristRad <= DESIGN.wristMaxRad);
    for (const c of [p.tip.x, p.tip.y, p.wrist.x, p.wrist.y, p.nominalReachM, p.explosionFraction]) {
      assert.ok(Number.isFinite(c));
    }
    assert.ok(p.nominalReachM <= p.maximumReachM + epsilon);
    assert.ok(p.explosionFraction >= 0 && p.explosionFraction <= 1);
  }
});

test('render timeline is repeatable and has no hidden state', () => {
  const first = [0, 1.25, 8.5, 11, 17, 23.9].map(poseAt);
  const second = [0, 1.25, 8.5, 11, 17, 23.9].map(poseAt);
  assert.deepEqual(first, second);
  assert.deepEqual(poseAt(5), poseAt(29));
});

test('exploded demonstration opens and closes while starting intact', () => {
  assert.equal(poseAt(0).explosionFraction, 0);
  assert.equal(poseAt(8).explosionFraction, 0);
  assert.equal(poseAt(12).explosionFraction, 1);
  assert.equal(poseAt(18).explosionFraction, 0);
  assert.equal(poseAt(23).explosionFraction, 0);
});

test('invalid times and nonfinite joints fail explicitly', () => {
  for (const t of [-1, Number.NaN, Infinity]) {
    assert.throws(() => poseAt(t), RangeError);
  }
  assert.throws(
    () => forwardKinematics({shoulderRad: 0, elbowRad: NaN, wristRad: 0}),
    RangeError,
  );
});
