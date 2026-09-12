# Phase 2 — Surface Hover Command Wiring Product Reality Test

## Scope

`PHASE2_VERTICAL_SLICE_PLAN.md`'s Slice 10 status previously named this exact
gap: `surface_descent_guidance.hpp`'s hover command-shaping math
(`SurfaceHoverProfile`, `constrain_surface_hover_velocity()`,
`controlled_hover_velocity_command()`) landed as a standalone,
ctest-covered module, but nothing outside its own tests called it — the
same "math foundation only, not yet wired" gap the earlier controlled-
descent pass already closed for `constrain_surface_approach_velocity()`.

This pass closes exactly that wiring gap for hover, following the same
read-only-query/apply-command pattern `GetControlledDescentVelocityCommand()`/
`CommandSetControlledDescentVelocityMetersPerSecond()` already established:

`constrain_surface_hover_velocity()` (existing, unchanged, from
`surface_descent_guidance.hpp`) -> `controlled_hover_velocity_command()`
(existing, unchanged, same header — fails closed to `std::nullopt` when no
planetary body is registered) -> `UProbeSimulationAdapter::
GetControlledHoverVelocityCommand()` (new, read-only query, added to the
existing `ProbeSurfaceDescentBridge.cpp`) and `UProbeSimulationAdapter::
CommandSetControlledHoverVelocityMetersPerSecond()` (new, applies the
constrained command through the existing `Core->set_velocity_mps()`
mutation boundary — no second velocity-mutation path).

No new simulation state, physics, or math is introduced — this pass only
wires the already-tested hover math into the adapter boundary. With no
planetary body registered (every existing deep-space scenario) both the
query and the command fail closed exactly like their descent counterparts,
and no other behavior changes. This qualifies for the parallel-safe lane
the same way the controlled-descent command wiring did.

## Behavior

- `GetControlledHoverVelocityCommand(RequestedVelocityMetersPerSecond,
  MaxTangentialSpeedMetersPerSecond, MinimumClearanceMeters,
  TargetAltitudeMeters, AltitudeGainPerSecond,
  MaxVerticalCorrectionSpeedMetersPerSecond)` is a `BlueprintPure` query:
  given a requested inertial velocity and the caller's tunable
  envelope/hover profile, it reports `bHasResult=false` with no registered
  planetary body, or the constrained `CommandVelocityMetersPerSecond` plus
  `bDescentRateLimited` (set when the vertical altitude-hold correction was
  clamped to `MaxVerticalCorrectionSpeedMetersPerSecond`) and
  `bTangentialRateLimited` flags otherwise. It does not move the probe.
  This reuses the existing `FEverwardControlledDescentCommand` result
  struct rather than introducing a second one, since the underlying
  `constrain_surface_hover_velocity()`/`constrain_surface_approach_velocity()`
  functions already return the identical `SurfaceApproachVelocityCommand`
  shape.
- `CommandSetControlledHoverVelocityMetersPerSecond(...)` is a
  `BlueprintCallable` command taking the same parameters: it computes the
  identical constrained velocity and, only when a planetary body is
  registered, applies it through `Core->set_velocity_mps()` — the same
  authoritative mutation point manual translation and the controlled-descent
  command already use. With no planetary body registered it is rejected
  ("no planetary body registered") rather than silently falling back to an
  unconstrained raw velocity command.
- No key binding, HUD row, or controller state machine was added in this
  pass. Unlike controlled descent (whose command-wiring pass was followed
  by a dedicated player-facing engage/cancel loop pass), hover's player
  loop remains later work — this closes only the command-shaping wiring
  gap `PHASE2_VERTICAL_SLICE_PLAN.md` named as the next concrete step.

## CI-verifiable acceptance

- `src/simulation/tests/surface_descent_guidance_tests.cpp`
  (`everward_surface_descent_guidance_tests` in `src/simulation/CMakeLists.txt`)
  already covers `controlled_hover_velocity_command()` and
  `constrain_surface_hover_velocity()` directly and is unchanged by this
  pass — no new simulation-layer behavior was introduced. All
  `src/simulation` ctest suites pass.
- `tools/test_surface_hover_command_surface.py` (new) confirms the hover
  math remains engine-independent and ctest-covered; that the adapter
  declares `GetControlledHoverVelocityCommand()` and
  `CommandSetControlledHoverVelocityMetersPerSecond()`; that the bridge file
  actually calls `controlled_hover_velocity_command()` rather than merely
  declaring the methods; that both new functions route through
  `Core->set_velocity_mps()` and reject with "no planetary body registered"
  the same way the descent command does; that
  `GetControlledHoverVelocityCommand()` reuses
  `FEverwardControlledDescentCommand` rather than a second struct; and that
  this status document's method names are reflected in the vertical-slice
  plan and project-status record.

No Unreal Editor/UBT build was available in this sandbox to compile-verify
`ProbeSimulationAdapter.h` or `ProbeSurfaceDescentBridge.cpp`. The new
functions follow the exact `Core == nullptr` guard,
`RecordCommandResult`/try-catch-`std::exception` rejection pattern, and
read-only-query struct-return shape already compiling in the same file's
`GetControlledDescentVelocityCommand()`/
`CommandSetControlledDescentVelocityMetersPerSecond()`. The next local
Unreal Product Reality pass should specifically confirm the project still
compiles under UBT before relying on this further.

## Local Unreal Product Reality acceptance

1. Launch the exact CI-green build and enter PIE in a scene with a
   registered planetary body (none exists in the current Phase-2 test
   environment yet — this requires the still-pending dedicated Slice 9/10
   scene, or a temporary registration for manual verification).
2. From Blueprint or a temporary debug binding, call
   `GetControlledHoverVelocityCommand` below the configured
   `TargetAltitudeMeters` with the probe descending and confirm the
   returned velocity's radial component corrects upward while the
   requested tangential velocity is preserved.
3. Repeat above the target altitude and confirm the radial correction now
   points downward toward the target.
4. Call the same query with no planetary body registered and confirm
   `bHasResult` is false.
5. Call `CommandSetControlledHoverVelocityMetersPerSecond` with a
   registered body and confirm the probe's velocity visibly updates to the
   constrained value and `GetLastCommandResult()` reports acceptance.
6. Call the same command with no planetary body registered and confirm it
   is rejected ("no planetary body registered") and the probe's velocity is
   unchanged.
7. Confirm this pass has not changed manual translation, controlled
   descent, José Take the Wheel, gravity/surface-contact integration, or
   any other existing command.
8. Record any discrepancy (wrong clamping, a mutation despite no registered
   body, or a build/compile failure) as Product Reality evidence.

## Explicitly not complete in this pass

- No key binding, HUD row, or engage/cancel controller loop exists yet —
  the query/command are reachable only from Blueprint or a temporary debug
  binding, not from ordinary play. A follow-on pass should add this
  following `EverwardPlayerControllerDescent.cpp`'s exact
  toggle/advance/cancel shape.
- No dedicated Unreal scene with a registered planetary body exists yet to
  actually exercise this near a real surface in PIE.
- Slice 10's other minimum interactions (terrain avoidance/contact beyond
  Slice 9's existing surface-contact resolution, scan/manipulate a surface
  sample, departure back toward space) remain unattempted.

## Status

Hover command shaping is now wired into the adapter boundary; the
player-facing engage/cancel loop and Product Reality evidence both remain
pending. Does not by itself advance Slice 10's completion gate.
