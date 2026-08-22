# Everward Unreal Production Project

This is the production Unreal Engine project for Phase 2 and later gameplay work. It is intentionally separate from `prototypes/rendering-benchmark/unreal/`, which remains frozen Phase 1 evidence.

## Architectural boundary

- `src/simulation/` owns mechanical truth and must remain buildable/testable without Unreal.
- `UProbeSimulationAdapter` is the only Unreal-side type authorized to call the simulation core during the One Probe kickoff slice.
- Unreal converts presentation units and renders authoritative state; it does not independently resolve mass, energy, thermal, propulsion, scan, storage, or policy outcomes.
- The adapter advances simulation using a fixed-step accumulator rather than feeding variable render-frame timing directly into the simulation.

## Opening the project

Open `Everward.uproject` with Unreal Engine 5.8. Generate project files/build the C++ module when prompted.

The current kickoff contains no authored map or production assets yet. Its purpose is to establish the production project, compile the simulation boundary, and make the first probe state accessible to Unreal/Blueprint presentation code.
