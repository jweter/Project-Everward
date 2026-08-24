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
- with Sensors selected, `Enter` starts a 10-second bootstrap scan and `Backspace` cancels it;
- with Propulsion selected, `Up` / `Down` nudges authoritative X velocity by 1 m/s and `Space` commands zero velocity;
- with Computation selected, `Enter` installs Basic Survival and `Backspace` clears it.

The hard-coded bootstrap scan target is explicitly temporary. It exists only to make the already-built scan lifecycle interactively testable before Phase 3 provides real target selection.

The HUD remains presentation only. It does not own mechanical state or policy decisions.

The project still contains no authored production map. `DefaultEngine.ini` selects the production game mode, so Unreal's startup map can spawn the single probe without duplicating simulation ownership.

The next Phase 2 playtest-preparation slice is a reproducible One Probe test environment with camera/look controls, visible movement references, and a visible test scan target, followed by local Unreal 5.8 build/run evidence and control-feel refinement.
