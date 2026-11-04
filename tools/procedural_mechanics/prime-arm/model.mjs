/**
 * Everward procedural arm: deterministic, pure kinematics for an isolated
 * engineering demonstration. Not canonical Prime A dimensions or game physics.
 * Units: metres, seconds, radians. Coordinate frame: +X right, +Y up.
 */
export const DESIGN = Object.freeze({
  upperM: 1.8,
  forearmM: 1.42,
  toolM: 0.55,
  upperRadiusM: 0.14,
  forearmRadiusM: 0.115,
  shoulderMinRad: -1.2,
  shoulderMaxRad: 0.65,
  elbowMinRad: -0.2,
  elbowMaxRad: 1.5,
  wristMinRad: -0.35,
  wristMaxRad: 0.35,
});

const clamp = (x, low, high) => Math.min(high, Math.max(low, x));
const smooth = (x) => {
  const u = clamp(x, 0, 1);
  return u * u * (3 - 2 * u);
};
const segment = (length, angle) => ({
  x: length * Math.sin(angle),
  y: -length * Math.cos(angle),
});

/** Pure forward kinematics in the shoulder's XY plane. */
export function forwardKinematics(angles, dims = DESIGN) {
  const shoulder = angles.shoulderRad;
  const fore = shoulder + angles.elbowRad;
  const tool = fore + angles.wristRad;
  if (![shoulder, fore, tool].every(Number.isFinite)) {
    throw new RangeError("Joint angles must be finite");
  }
  const u = segment(dims.upperM, shoulder);
  const f = segment(dims.forearmM, fore);
  const tipPart = segment(dims.toolM, tool);
  const elbow = {x: u.x, y: u.y};
  const wrist = {x: u.x + f.x, y: u.y + f.y};
  const tip = {x: wrist.x + tipPart.x, y: wrist.y + tipPart.y};
  return {
    elbow,
    wrist,
    tip,
    nominalReachM: Math.hypot(tip.x, tip.y),
    maximumReachM: dims.upperM + dims.forearmM + dims.toolM,
  };
}

/**
 * Repeatable 24-second engineering-film pose. The exploded phase is a visual
 * illustration only; gameplay collision/tool placement must use live Unreal
 * authoritative state and must not use this film's offsets.
 */
export function poseAt(seconds) {
  if (!Number.isFinite(seconds) || seconds < 0) {
    throw new RangeError("Time must be a non-negative finite number");
  }
  const t = seconds % 24;
  const shoulderRad = clamp(
    -0.32 + 0.46 * Math.sin(t * 0.42),
    DESIGN.shoulderMinRad, DESIGN.shoulderMaxRad,
  );
  const elbowRad = clamp(
    0.64 + 0.35 * Math.sin(t * 0.48 + 0.3),
    DESIGN.elbowMinRad, DESIGN.elbowMaxRad,
  );
  const wristRad = clamp(
    0.22 * Math.sin(t * 0.67),
    DESIGN.wristMinRad, DESIGN.wristMaxRad,
  );
  const explode = smooth((t - 8) / 2) * (1 - smooth((t - 16) / 2));
  const angles = {shoulderRad, elbowRad, wristRad};
  return {
    timeSec: t,
    angles,
    ...forwardKinematics(angles),
    explosionFraction: explode,
    visualizationOnly: true,
  };
}
