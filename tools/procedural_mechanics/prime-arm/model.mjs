/**
 * Everward Mechanical Bridge v0.1 — 3D probe-local FK.
 *
 * Authoritative source: manipulator_hull_contact.hpp +
 * manipulator_tool_contact.hpp + manipulator.hpp. This checked-in projection
 * is checked against a C++ oracle in CI, so source numeric changes cannot
 * silently change the browser model. Unreal owns all actual simulation state.
 *
 * Coordinates: (+X forward, +Y starboard, +Z up), metres/degrees.
 * The renderer and GLB export translate these axes for their own convention.
 */
import layout from './layout.json' with {type: 'json'};

export const DESIGN = Object.freeze(layout);
const DEG = Math.PI / 180;
const clamp = (n, a, b) => Math.max(a, Math.min(b, n));
const plus = (a, b) => a.map((v, i) => v + b[i]);
const radians = (degrees) => degrees * DEG;
const identity = () => [1, 0, 0, 0, 1, 0, 0, 0, 1];
const rotateX = (r) => {
  const c = Math.cos(r), s = Math.sin(r);
  return [1, 0, 0, 0, c, -s, 0, s, c];
};
const rotateY = (r) => {
  const c = Math.cos(r), s = Math.sin(r);
  return [c, 0, s, 0, 1, 0, -s, 0, c];
};
const multiply = (a, b) => {
  const out = Array(9).fill(0);
  for (let row = 0; row < 3; ++row)
    for (let col = 0; col < 3; ++col)
      for (let k = 0; k < 3; ++k)
        out[row * 3 + col] += a[row * 3 + k] * b[k * 3 + col];
  return out;
};
const apply = (m, p) => [
  m[0] * p[0] + m[1] * p[1] + m[2] * p[2],
  m[3] * p[0] + m[4] * p[1] + m[5] * p[2],
  m[6] * p[0] + m[7] * p[1] + m[8] * p[2],
];
const smoother = (x) => {
  const u = clamp(x, 0, 1);
  return u * u * (3 - 2 * u);
};

/**
 * One arm's live-compatible 3D kinematic contract.
 * Positions are probe local, NOT world transformed and NOT a collision result.
 * The C++ rig and its guards alone decide which commanded poses are reachable.
 */
export function forwardKinematics({
  arm = 'port',
  deploymentFraction = 1,
  anglesDegrees = {shoulder: 0, elbow: 0, wrist: 0},
} = {}) {
  if (arm !== 'port' && arm !== 'starboard') throw new RangeError('Unknown arm');
  if (!Number.isFinite(deploymentFraction)) throw new RangeError('Bad deployment fraction');
  for (const key of ['shoulder', 'elbow', 'wrist']) {
    const value = anglesDegrees[key];
    if (!Number.isFinite(value)) throw new RangeError('Non-finite joint angle: ' + key);
    const [lo, hi] = DESIGN.joint_limits_degrees[key];
    if (value < lo || value > hi) throw new RangeError('Joint outside C++ limits: ' + key);
  }

  const fraction = clamp(deploymentFraction, 0, 1);
  const side = arm === 'port' ? -1 : 1;
  const foldPitch = DESIGN.fold_pitch_degrees.stowed +
    fraction * (DESIGN.fold_pitch_degrees.deployed - DESIGN.fold_pitch_degrees.stowed);
  const foldRoll = DESIGN.fold_roll_degrees.stowed +
    fraction * (DESIGN.fold_roll_degrees.deployed - DESIGN.fold_roll_degrees.stowed);
  const shoulderRotation = multiply(
    rotateY(radians(foldPitch + anglesDegrees.shoulder)),
    rotateX(radians(side * foldRoll)),
  );
  const elbowRotation = multiply(shoulderRotation, rotateY(radians(anglesDegrees.elbow)));
  const wristRotation = multiply(elbowRotation, rotateY(radians(anglesDegrees.wrist)));
  const shoulder = [...DESIGN.shoulder_m[arm]];
  const elbow = plus(shoulder, apply(shoulderRotation, [DESIGN.link_m.upper, 0, 0]));
  const wrist = plus(elbow, apply(elbowRotation, [DESIGN.link_m.forearm, 0, 0]));
  const toolTip = plus(wrist, apply(wristRotation, [DESIGN.link_m.tool_tip_from_wrist, 0, 0]));
  return {
    arm, deploymentFraction: fraction, anglesDegrees: {...anglesDegrees},
    shoulder, elbow, wrist, toolTip,
    rotations: {shoulder: shoulderRotation, elbow: elbowRotation, wrist: wristRotation},
    toolTipRadiusM: DESIGN.sample_radius_m.tool_tip,
    maximumExtensionFromShoulderM:
      DESIGN.link_m.upper + DESIGN.link_m.forearm + DESIGN.link_m.tool_tip_from_wrist,
  };
}

/**
 * Deterministic reference poses for the standalone engineering studio. Only
 * the real adapter should provide live rotations in Unreal. The explosion
 * fraction is intentionally separate and NEVER applied to contact math.
 */
export function poseAt(seconds) {
  if (!Number.isFinite(seconds) || seconds < 0)
    throw new RangeError('Expected non-negative finite time');
  const t = seconds % 24;
  const f = smoother(Math.min(1, t / 3));
  const explode = smoother((t - 8) / 2) * (1 - smoother((t - 16) / 2));
  const animate = (phase) => ({
    shoulder: -8 + 18 * Math.sin(t * 0.4 + phase),
    elbow: 54 + 22 * Math.sin(t * 0.45 + phase),
    wrist: 15 * Math.sin(t * 0.63 + phase),
  });
  return {
    timeSeconds: t,
    explosionFraction: explode,
    arms: {
      port: forwardKinematics({arm:'port', deploymentFraction:f, anglesDegrees:animate(0)}),
      starboard: forwardKinematics({arm:'starboard', deploymentFraction:f, anglesDegrees:animate(0.8)}),
    },
    presentationOnly: true,
  };
}
