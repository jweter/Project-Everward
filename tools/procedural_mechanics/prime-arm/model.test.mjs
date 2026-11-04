import test from 'node:test';
import assert from 'node:assert/strict';
import {DESIGN, forwardKinematics, poseAt} from './model.mjs';

const near = (a, b, epsilon = 1e-11) => Math.abs(a - b) < epsilon;
const distance = (a, b) => Math.hypot(...a.map((x, i) => x - b[i]));
test('both shoulders agree with the C++-derived 3D contract', () => {
  const port = forwardKinematics({arm:'port'});
  const starboard = forwardKinematics({arm:'starboard'});
  assert.deepEqual(port.shoulder, [1.2, -1.05, -0.92]);
  assert.deepEqual(starboard.shoulder, [1.2, 1.05, -0.92]);
  assert.equal(port.maximumExtensionFromShoulderM, 4.09);
  assert.equal(port.toolTipRadiusM, 0.17);
});

test('probe-local tool contact is on the real composed wrist orientation', () => {
  for (const arm of ['port', 'starboard']) {
    const p = forwardKinematics({arm, deploymentFraction: 1, anglesDegrees: {shoulder:0,elbow:0,wrist:0}});
    assert.ok(near(distance(p.shoulder, p.elbow), DESIGN.link_m.upper));
    assert.ok(near(distance(p.elbow, p.wrist), DESIGN.link_m.forearm));
    assert.ok(near(distance(p.wrist, p.toolTip), DESIGN.link_m.tool_tip_from_wrist));
    assert.ok(distance(p.shoulder, p.toolTip) <= p.maximumExtensionFromShoulderM + 1e-10);
  }
});

test('3D parity surfaces never emit NaN and rotations remain orthonormal', () => {
  for (const arm of ['port', 'starboard']) {
    for (let d = 0; d <= 10; d++) {
      const p = forwardKinematics({
        arm, deploymentFraction: d / 10,
        anglesDegrees: {shoulder: 25, elbow: 85, wrist: -70},
      });
      assert.equal(p.rotations.shoulder.length, 9);
      for (const xyz of [p.elbow, p.wrist, p.toolTip]) {
        assert.equal(xyz.length, 3);
        assert.ok(xyz.every(Number.isFinite));
      }
      for (const m of Object.values(p.rotations)) {
        for (let row = 0; row < 3; row++) {
          const sq = [0, 1, 2].reduce((v, col) => v + m[row * 3 + col] ** 2, 0);
          assert.ok(near(sq, 1), 'Rotation row must remain unit length');
        }
      }
    }
  }
});

test('24-second studio poses are deterministic, within C++ limits and render-only', () => {
  const check = [0, 3, 8, 10, 12, 16, 18, 24, 29];
  const first = check.map(poseAt);
  const second = check.map(poseAt);
  assert.deepEqual(first, second);
  assert.deepEqual(poseAt(5), poseAt(29));
  assert.equal(poseAt(0).explosionFraction, 0);
  assert.equal(poseAt(12).explosionFraction, 1);
  assert.equal(poseAt(18).explosionFraction, 0);
  for (const p of first) {
    assert.equal(p.presentationOnly, true);
    for (const arm of ['port', 'starboard']) {
      assert.ok(p.arms[arm].toolTip.every(Number.isFinite));
      for (const joint of ['shoulder', 'elbow', 'wrist']) {
        const [min, max] = DESIGN.joint_limits_degrees[joint];
        assert.ok(p.arms[arm].anglesDegrees[joint] >= min);
        assert.ok(p.arms[arm].anglesDegrees[joint] <= max);
      }
    }
  }
});

test('invalid, unsafe, nonfinite and unknown arm inputs fail rather than fabricating values', () => {
  for (const arm of ['unknown', '', null]) assert.throws(() => forwardKinematics({arm}), RangeError);
  for (const t of [-1, NaN, Infinity]) assert.throws(() => poseAt(t), RangeError);
  for (const val of [-91, 91, NaN]) {
    assert.throws(
      () => forwardKinematics({anglesDegrees:{shoulder:val, elbow:0, wrist:0}}),
      RangeError,
    );
  }
  assert.throws(() => forwardKinematics({anglesDegrees:{shoulder:0, elbow:151, wrist:0}}), RangeError);
  assert.throws(() => forwardKinematics({deploymentFraction:NaN}), RangeError);
});
