# Everward Unreal Production Project

This is the production Unreal Engine project for Phase 2 and later gameplay work. It is intentionally separate from `prototypes/rendering-benchmark/unreal/`, which remains frozen Phase 1 evidence.

## Architectural boundary

- `src/simulation/` owns mechanical truth and must remain buildable/testable without Unreal.
- `UProbeSimulationAdapter` is the only Unreal-side type authorized to call the authoritative One Probe runtime during the kickoff slice.
- Unreal converts presentation units and renders authoritative state; it does not independently resolve mass, energy, thermal, propulsion, scan, storage, capability, or policy outcomes.
- The adapter advances simulation using a fixed-step accumulator rather than feeding variable render-frame timing directly into the simulation.
- Authoritative spatial state is stored in metres. The adapter converts it to Unreal centimetres exactly once when synchronizing the owning probe pawn's presentation transform.

## Opening the project

Open `Everward.uproject` with Unreal Engine 5.8. Generate project files/build the C++ module when prompted.

The Phase 2 runtime bootstrap supplies `AEverwardGameMode` and one default `AEverwardProbePawn`. The pawn owns exactly one visible engine-sphere presentation, one `UProbeSimulationAdapter`, and a third-person camera; the adapter constructs the canonical EV-0001 `ProbeRuntime`. The engine sphere is an explicit bootstrap placeholder, not the final probe art direction.

The adapter synchronizes the pawn's actor location from the authoritative runtime snapshot after deterministic fixed-step simulation advancement. The pawn does not author its own mechanical position.

## Reproducible Phase-2 first-run environment

The production project still does not contain an authored Phase-3 star-system map. Instead, `AEverwardGameMode::InitGame()` creates a temporary Phase-2 integration environment entirely from source so every clean checkout can run the same One Probe test.

The bootstrap now creates:

- a deterministic `APlayerStart` at the world origin;
- `AEverwardPhase2TestEnvironment`;
- one visible scan target 50 m along +X;
- six spatial reference markers for parallax and movement perception;
- a temporary point light so visibility does not depend on the editor map;
- an in-world label: `PHASE-2 TARGET // SCAN-001`.

The visible target's canonical temporary identifier is `phase2-test-target-001`. The Sensors action uses that exact identifier. This is not real targeting; Phase 3 must replace it with selected world-object state.

See `docs/PHASE2_FIRST_RUN_ENVIRONMENT.md`.

## Camera controls

The first-run third-person camera is presentation-only:

- mouse X: orbit yaw;
- mouse Y: orbit pitch;
- mouse wheel: zoom;
- zoom is clamped to a practical inspection range.

Camera movement never authors probe position or other simulation state.

## Capability-driven HUD and command shell

`AEverwardHUD` is the first production HUD/control shell. It intentionally avoids a permanently expanded cockpit interface:

- compact always-visible telemetry shows probe identity/generation, energy, total power allocation/capacity, thermal state, storage, velocity, and simulation time;
- active scanning is promoted while it is running;
- critical energy/thermal lockouts promote themselves into visible alerts;
- the systems/control area remains collapsed by default;
- `Tab` expands or collapses the systems panel;
- `[` and `]` browse installed capabilities when the panel is open;
- the panel is populated from `UProbeSimulationAdapter::GetInstalledCapabilities()` rather than a universal hard-coded player ability list;
- each capability reports operational/available state, allocated power, and whether it exposes manual-control and automation surfaces.

The current Generation-1 capability descriptors are propulsion, sensors, computation, and thermal control because those are the authoritative subsystem concepts the simulation currently models. Future drills, manipulators, lasers, fabrication systems, new sensor modalities, and other descendant hardware should extend the same capability/read-model pattern rather than adding unrelated HUD special cases.

## Shared authoritative commands

`UProbeSimulationAdapter` exposes the first shared command methods:

- `CommandSetVelocityMetersPerSecond`;
- `CommandStartScan`;
- `CommandCancelScan`;
- `CommandAllocatePower`.

Each command returns `FEverwardProbeCommandResult` with a sequence number, command id, accepted/rejected state, and detail. The adapter catches authoritative runtime rejections and makes the reason visible to presentation callers instead of silently swallowing it.

Manual input uses these command methods. Software policy actions resolve through the same underlying `SimulationCore` commands rather than writing separate automation-only mechanical state.

## Generation-1 software policy

The authoritative runtime is now `everward::simulation::ProbeRuntime`, which wraps `SimulationCore` with the first primitive, engine-independent policy evaluator.

Generation 1 intentionally supports only:

- one active policy;
- at most two scalar condition/action rules;
- energy-fraction or temperature conditions;
- power-allocation actions;
- execution only when computation is operational and receives at least 25 W.

The temporary player-facing preset is `gen1_basic_survival`:

- below 60% stored energy: set sensor power to 0 W;
- above 350 K: set propulsion power to 0 W.

The 60% threshold is deliberately aggressive for integration testing so EV-0001 can demonstrate automation immediately; it is not final balance.

With Computation selected:

- `Enter`: install Basic Survival;
- `Backspace`: clear the policy;
- `Page Up` / `Page Down`: adjust computation power and cross the 25 W execution threshold.

The HUD displays the installed policy and whether its executor is running or waiting for sufficient compute power. See `docs/GEN1_SOFTWARE_POLICY.md`.

## Temporary Phase-2 engineering controls

- `Tab`: open/close systems panel;
- `[` / `]`: select capability;
- `Page Up` / `Page Down`: increase/decrease selected subsystem power;
- with Sensors selected, `Enter` starts a 10-second scan of the visible Phase-2 target and `Backspace` cancels it;
- with Propulsion selected:
  - `W` / `S`: +X / -X velocity trim;
  - `D` / `A`: +Y / -Y velocity trim;
  - `E` / `Q`: +Z / -Z velocity trim;
  - `Space`: command zero velocity;
  - Up / Down remain +X / -X aliases from the earlier shell;
- with Computation selected, `Enter` installs Basic Survival and `Backspace` clears it.

Every velocity trim is 1 m/s and still routes through `CommandSetVelocityMetersPerSecond`. This is deliberately primitive Generation-1 integration control, not a final thruster or attitude model.

The HUD remains presentation only. It does not own mechanical state or policy decisions.

## Next validation step

The next required evidence is a local Unreal Engine 5.8 build and first-run session exercising the environment, camera, HUD, power management, movement, visible scan target, and Generation-1 policy together.

Hosted CI still cannot compile the Unreal module, so source-contract tests protect architecture but do not replace that local UBT/Editor validation. After the first run, control feel and any compile/runtime defects should be corrected before expanding Phase 2 further.
