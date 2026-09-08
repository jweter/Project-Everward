# Phase 2 Tractor-Field Playable Test

## Purpose

This slice turns the already-merged tractor-field physics foundation into the first player-operable Unreal interaction without moving authoritative motion into presentation code.

The tractor technology remains speculative, but the mechanical result reuses `tractor_field.hpp`: equal-and-opposite coupling force, mass-dependent acceleration, finite range, finite field force, and conserved momentum when no external thrust is added.

## Current controls

1. Use **T** to select/cycle a registered physical target.
2. Move within the Generation-1 tractor field's **40 m surface-to-surface range**.
3. **Hold B** to engage the tractor field at the current prototype setting of **1.0 kN**.
4. Watch the cyan line/sphere presentation while the coupling is active.
5. **Release B** to disengage. The target keeps the velocity it acquired in zero gravity.
6. Use the existing propulsion controls before or during tractor work to experiment with relative motion and momentum.

A persistent temporary tractor readout keeps the control discoverable and reports target/probe mass, mass ratio, current surface gap, and field range. The first coupling attempt also produces an on-screen status message. A rejected attempt states why, for example no selected target, target outside field range, or target already held by a manipulator.

## Phase-2 mass calibration

The three existing registered test bodies now deliberately exercise the three important mass regimes against EV-0001's canonical 2,500 kg mass:

| Target | Mass | Expected tractor behavior |
|---|---:|---|
| `phase2-test-target-001` | 500 kg | Target accelerates much more than EV-0001 |
| `phase2-test-target-002` | 10,000 kg | EV-0001 accelerates much more; target behaves like an anchor |
| `phase2-test-target-003` | 2,500 kg | Equal acceleration magnitudes in opposite directions |

The bootstrap body starts at exactly the Gen-1 tractor limit in the current test geometry: 50 m center distance minus the probe's 8 m collision envelope and the target's 2 m radius = 40 m surface gap.

## Authority boundary

This is not an Unreal-physics shortcut.

- The controller owns only input and the temporary cyan beam/readout presentation.
- `ProbeTractorFieldBridge.cpp` constructs `TractorBodyState` values and calls the engine-independent `TractorFieldSystem`.
- Probe velocity is written back through `DamageAwareProbeRuntime::set_velocity_mps()`.
- Target position is written back through `update_static_sphere_body_position()`.
- Existing target selection, manipulator reach, collision/world registration, mining presentation, and target meshes therefore continue reading the same registered body center.
- Released target velocity is temporarily retained by the adapter so a pulled object continues drifting instead of stopping magically when the field turns off.

`tools/test_phase2_tractor_field_surface.py` protects this boundary and explicitly rejects presentation-side `SetActorLocation` / `SetWorldLocation` as the tractor implementation.

## What this slice proves

A successful local Product Reality pass should establish all of the following:

- a selected nearby physical body can be coupled by holding B;
- a visible field line makes the active coupling unmistakable;
- the 500 kg body moves toward EV-0001 much more readily than EV-0001 moves toward it;
- the 10,000 kg body produces the opposite qualitative result and pulls EV-0001 toward itself more strongly;
- the equal-mass body produces visibly shared motion;
- releasing B stops field force without erasing the target's acquired drift;
- moving targets remain the same authoritative targets used by selection and manipulator systems.

## Known prototype limitations

This is intentionally the first playable bridge, not the final tractor system.

- `StaticSphereBody` predates movable-body mass and velocity, so the three Phase-2 bodies use explicit test mass constants and the adapter temporarily owns target velocity. A later world-body schema should make mass and velocity first-class simulation state.
- The current target-selection closing-speed telemetry still treats registered spheres as static bodies; it does not yet subtract tractor-created target velocity.
- Target-target collision and full dynamic-body contact resolution are not generalized yet.
- Engine commands already change the probe's authoritative velocity and therefore change the momentum state entering the next tractor step, but this first bridge does not yet expose a dedicated continuous engine-force vector to the tractor solver.
- The cyan `DrawDebugLine`/`DrawDebugSphere` and debug readout are temporary Product Reality feedback. A production field effect, sound, emitter hardware animation, power draw, heat, and permanent tractor HUD panel remain later presentation/system work.

These limitations should be removed incrementally without replacing the momentum-conserving mechanic with an arcade pickup rule.

## Next high-value tractor slices

1. Promote physical-body mass and velocity into the authoritative registered-body schema.
2. Make target range/closing telemetry relative to moving targets.
3. Add permanent HUD rows for target mass, probe mass, ratio, range, field force, and predicted dominant mover.
4. Add explicit power draw and thermal load while the field is engaged.
5. Feed real propulsion thrust force into the coupling step for instrumented powered towing.
6. Add collision/capture behavior for pulled debris entering manipulator or storage/recycler handling range.
7. Replace debug rendering with a production field-emitter visual/audio treatment.
