# Phase 2 — Camera Righting + Orientation Skin Product Reality Test

Status: implementation branch; local Unreal Product Reality required before the slice is considered complete.

## Purpose

This pass addresses the 2026-08-26 local Phase 2 playtest finding that yaw/pitch now work and scanning has better payoff, but the temporary probe body does not clearly communicate forward/up/level orientation and probe-relative movement becomes harder after attitude changes.

The implementation remains intentionally Generation-1: useful, slow, mechanical, and slightly clunky rather than frictionless.

## Player-visible changes

### Orientation skin

The temporary engineering body now has deliberately asymmetric geometry:

- a forward sensor blister on local +X;
- a dorsal marker on local +Z;
- port and starboard shoulder structures that make the lateral plane readable.

This is not the final Prime Probe A art asset. It is an embodiment/readability skin that should make forward, up, and roll obvious enough to test attitude and movement before production modeling lands.

### Camera-aligned auto-righting

Press `R` once to begin righting the probe toward the camera's current facing direction.

The target uses:

- camera/view yaw;
- camera/view pitch;
- zero roll, so the probe finishes visually level relative to the camera framing.

Righting does **not** teleport the actor or directly set Unreal rotation. It issues repeated small authoritative attitude commands through `UProbeSimulationAdapter` at a deliberately chunky cadence. Default tuning is 36 degrees/second in 0.10-second command steps.

Press `R` again during the maneuver to cancel it.

If propulsion is unavailable and the authoritative simulation rejects the maneuver, righting stops rather than retrying forever.

## Local UE 5.8 test script

Run the exact CI-green branch/build and test in one session:

1. Confirm the forward sensor blister makes the nose/forward end obvious before moving.
2. Confirm the dorsal marker makes local up and roll direction obvious from the normal third-person camera.
3. Use `J/L`, `I/K`, and `U/O` to put the probe into an awkward yaw/pitch/roll attitude.
4. Orbit the camera to the direction you want to travel.
5. Press `R` once.
6. Confirm the probe rotates toward the camera's current yaw/pitch while roll returns to level.
7. Confirm the motion is visibly gradual and chunky rather than instant, but does not feel painfully slow.
8. While righting, press `R` again and confirm the maneuver cancels cleanly.
9. Start righting again and let it finish; then use `W/S`, `A/D`, and `E/Q` and confirm movement is intuitive relative to the newly aligned probe.
10. Confirm `Space` still globally stops translation and does not interfere with attitude/righting state.
11. Confirm scanning and the compact subsystem HUD still work unchanged.

## Product Reality questions

Record these after the run:

- Can you identify the probe's forward direction immediately without reading a number?
- Can you identify which side is up and whether the probe is rolled?
- Does `R` consistently finish pointed where the camera suggests it should?
- Does 36 deg/s with 0.10 s steps feel "clunky but not painful"?
- After righting, is forward movement easier to predict?
- Does the temporary skin feel like a meaningful improvement over the previous ellipsoid even though it is not final art?

If righting points opposite the expected camera direction, overshoots, jitters, or makes movement less intuitive, treat that as a Product Reality blocker and fix it before building further controls on top of the behavior.
