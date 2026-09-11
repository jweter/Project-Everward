# Phase 2 — Controlled-Descent Command Wiring Product Reality Test

## Scope

`PHASE2_VERTICAL_SLICE_PLAN.md`'s Slice 10 status previously named this exact
gap: `surface_descent_guidance.hpp`'s command-shaping math (altitude-tapered
descent speed, radial/tangential velocity-command clamping toward a
`ControlledDescentEnvelope`) landed earlier as a standalone, ctest-covered
module, but no authoritative command, `ProbeRuntime`/adapter method, or
player input called it — unlike Slice 9's gravity/contact math, which an
earlier pass wired directly into the live tick.

This pass closes exactly that wiring gap, following the same read-only-query
pattern `GetJoseGuidanceCommand()` already established for Slice 7's
destination navigation (`docs/JOSE_TAKE_THE_WHEEL.md`):

`constrain_surface_approach_velocity()` (existing, unchanged, from
`surface_descent_guidance.hpp`) -> `controlled_descent_velocity_command()`
(new, engine-independent, same header — fails closed to `std::nullopt` when
no planetary body is registered) -> `UProbeSimulationAdapter::
GetControlledDescentVelocityCommand()` (new, read-only query, new
`ProbeSurfaceDescentBridge.cpp`) and `UProbeSimulationAdapter::
CommandSetControlledDescentVelocityMetersPerSecond()` (new, applies the
constrained command through the existing `Core->set_velocity_mps()`
mutation boundary `CommandSetVelocityMetersPerSecond()` already uses — no
second velocity-mutation path).

No new simulation state or physics is introduced. The command only
constrains a requested velocity to whatever `SphericalPlanetaryBody` Slice
9's `set_planetary_body()` currently has registered; with none registered
(every existing deep-space scenario) both the query and the command fail
closed rather than fabricating a descent envelope, and no other behavior
changes. This qualifies for the parallel-safe lane the same way Slice 9's
gravity/contact wiring did: it reads/derives from already-authoritative
state through the existing simulation/adapter boundary and does not assume
any still-pending contact/collision/damage Product Reality evidence is
correct.

## Behavior

- `GetControlledDescentVelocityCommand(RequestedVelocityMetersPerSecond,
  MaxDescentSpeedMetersPerSecond, MaxTangentialSpeedMetersPerSecond,
  MinimumClearanceMeters, FullSpeedAltitudeMeters,
  TouchdownDescentSpeedMetersPerSecond)` is a `BlueprintPure` query: given a
  requested inertial velocity and the caller's tunable envelope/profile, it
  reports `bHasResult=false` with no registered planetary body, or the
  constrained `CommandVelocityMetersPerSecond` plus
  `bDescentRateLimited`/`bTangentialRateLimited` flags otherwise. It does not
  move the probe.
- `CommandSetControlledDescentVelocityMetersPerSecond(...)` is a
  `BlueprintCallable` command taking the same parameters: it computes the
  identical constrained velocity and, only when a planetary body is
  registered, applies it through `Core->set_velocity_mps()` — the same
  authoritative mutation point manual translation (`CommandSetVelocityMetersPerSecond`)
  already uses. With no planetary body registered it is rejected
  ("no planetary body registered") rather than silently falling back to an
  unconstrained raw velocity command.
- No key binding, HUD row, or controller state machine was added in this
  pass — this closes the command-shaping wiring gap `PHASE2_VERTICAL_SLICE_PLAN.md`
  named as the next concrete step, not the full player-facing descent-mode
  loop. A future pass following José's exact precedent (an engage toggle on
  the player controller calling this query every tick and issuing the
  resulting velocity command) remains the next step to make this reachable
  from actual play.

## CI-verifiable acceptance

- `src/simulation/tests/surface_descent_guidance_tests.cpp`
  (`everward_surface_descent_guidance_tests` in `src/simulation/CMakeLists.txt`)
  covers `controlled_descent_velocity_command()` directly: it fails closed
  (`std::nullopt`) with no registered body, and matches
  `constrain_surface_approach_velocity()` exactly once a body is registered.
  All 27 `src/simulation` ctest suites pass.
- `tools/test_surface_descent_command_surface.py` confirms the new wrapper
  exists and is ctest-covered; that the adapter declares
  `FEverwardControlledDescentCommand`, `GetControlledDescentVelocityCommand()`,
  and `CommandSetControlledDescentVelocityMetersPerSecond()`; that the new
  bridge file actually calls `controlled_descent_velocity_command()`,
  `Core->planetary_body()`, and `Core->snapshot().position_m` rather than
  merely declaring the methods; that the command routes through
  `Core->set_velocity_mps()` with no `SetActorLocation`/`Teleport` shortcut;
  that it fails closed with no registered body; and that this status
  document's method names are reflected in the vertical-slice plan and
  project-status record.

No Unreal Editor/UBT build was available in this sandbox to compile-verify
`ProbeSimulationAdapter.h` or the new `ProbeSurfaceDescentBridge.cpp`. The
new bridge file follows the exact `Core == nullptr` guard,
`RecordCommandResult`/try-catch-`std::exception` rejection pattern, and
read-only-query struct-return shape already compiling elsewhere in
`ProbeTargetSelectionBridge.cpp` (`GetJoseGuidanceCommand`,
`CommandSetVelocityMetersPerSecond`). The next local Unreal Product Reality
pass should specifically confirm the project still compiles under UBT
before relying on this further.

## Local Unreal Product Reality acceptance

1. Launch the exact CI-green build and enter PIE in a scene with a
   registered planetary body (none exists in the current Phase-2 test
   environment yet — this requires the still-pending dedicated Slice 9/10
   scene, or a temporary registration for manual verification).
2. From Blueprint or a temporary debug binding, call
   `GetControlledDescentVelocityCommand` with a steep inward velocity and
   confirm `bHasResult` is true, the returned velocity's descent component is
   clamped to the configured `MaxDescentSpeedMetersPerSecond`, and
   `bDescentRateLimited` is true.
3. Call the same query with no planetary body registered and confirm
   `bHasResult` is false.
4. Call `CommandSetControlledDescentVelocityMetersPerSecond` with a
   registered body and confirm the probe's velocity visibly updates to the
   constrained value and `GetLastCommandResult()` reports acceptance.
5. Call the same command with no planetary body registered and confirm it is
   rejected ("no planetary body registered") and the probe's velocity is
   unchanged.
6. Confirm this pass has not changed manual translation, José Take the
   Wheel, gravity/surface-contact integration, or any other existing
   command.
7. Record any discrepancy (wrong clamping, a mutation despite no registered
   body, or a build/compile failure) as Product Reality evidence.

## Explicitly not complete in this pass

- No key binding, HUD row, or controller-side "controlled descent" session
  state machine — the command surface exists but is not yet reachable from
  ordinary play, the same way `GetJoseGuidanceCommand()` alone did not make
  José reachable until `EverwardPlayerControllerAutopilot.cpp`'s later pass
  added the engage/cancel loop.
- No dedicated Unreal scene with a registered planetary body exists yet to
  actually exercise this near a real surface in PIE.
- Slice 10's other minimum interactions (stable hover/translation,
  terrain avoidance/contact beyond Slice 9's existing surface-contact
  resolution, scan/manipulate a surface sample, departure back toward space)
  remain unattempted.

## Status

Implemented in the parallel-safe lane; Product Reality pending. Does not by
itself advance Slice 10's completion gate.
