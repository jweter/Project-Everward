# Everward Unreal Production Project

This is the production Unreal Engine project for Phase 2 and later gameplay work. It is intentionally separate from `prototypes/rendering-benchmark/unreal/`, which remains frozen Phase 1 evidence.

## Architectural boundary

- `src/simulation/` owns mechanical truth and must remain buildable/testable without Unreal.
- `UProbeSimulationAdapter` is the only Unreal-side type authorized to call the simulation core during the One Probe kickoff slice.
- Unreal converts presentation units and renders authoritative state; it does not independently resolve mass, energy, thermal, propulsion, scan, storage, capability, or policy outcomes.
- The adapter advances simulation using a fixed-step accumulator rather than feeding variable render-frame timing directly into the simulation.
- Authoritative spatial state is stored in metres. The adapter converts it to Unreal centimetres exactly once when synchronizing the owning probe pawn's presentation transform.

## Opening the project

Open `Everward.uproject` with Unreal Engine 5.8. Generate project files/build the C++ module when prompted.

The Phase 2 runtime bootstrap supplies `AEverwardGameMode` and one default `AEverwardProbePawn`. The pawn owns exactly one visible engine-sphere presentation, one `UProbeSimulationAdapter`, and a third-person camera; the adapter constructs the canonical EV-0001 loadout. The engine sphere is an explicit bootstrap placeholder, not the final probe art direction.

The adapter synchronizes the pawn's actor location from `SimulationCore::snapshot().position_m` after deterministic fixed-step simulation advancement. The pawn does not author its own mechanical position.

## Capability-driven HUD foundation

`AEverwardHUD` is the first production HUD shell. It intentionally avoids a permanently expanded cockpit interface:

- compact always-visible telemetry shows probe identity/generation, energy, thermal state, storage, velocity, and simulation time;
- critical energy/thermal lockouts promote themselves into visible alerts;
- the systems/control area remains collapsed by default;
- `Tab` expands or collapses the systems panel;
- `[` and `]` browse installed capabilities when the panel is open;
- the panel is populated from `UProbeSimulationAdapter::GetInstalledCapabilities()` rather than a universal hard-coded player ability list;
- each capability reports operational/available state, allocated power, and whether it exposes manual-control and automation surfaces.

The current Generation-1 capability descriptors are propulsion, sensors, computation, and thermal control because those are the authoritative subsystem concepts the simulation currently models. Future drills, manipulators, lasers, fabrication systems, new sensor modalities, and other descendant hardware should extend the same capability/read-model pattern rather than adding unrelated HUD special cases.

The HUD is presentation only. It never calls `SimulationCore` directly. Manual UI actions and future script/automation actions must ultimately submit the same authoritative command types through the adapter/command boundary.

The project still contains no authored production map. `DefaultEngine.ini` selects the production game mode, so Unreal's startup map can spawn the single probe without duplicating simulation ownership.

The next Phase 2 interaction slices are adapter-backed scanning and power-management commands, then the first software-policy/automation surface. Those controls should appear contextually from the installed capability model rather than remaining permanently visible.
