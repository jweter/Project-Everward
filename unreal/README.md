# Everward Unreal Production Project

This is the production Unreal Engine project for Phase 2 and later gameplay work. It is intentionally separate from `prototypes/rendering-benchmark/unreal/`, which remains frozen Phase 1 evidence.

## Architectural boundary

- `src/simulation/` owns mechanical truth and must remain buildable/testable without Unreal.
- `UProbeSimulationAdapter` is the only Unreal-side type authorized to call the simulation core during the One Probe kickoff slice.
- Unreal converts presentation units and renders authoritative state; it does not independently resolve mass, energy, thermal, propulsion, scan, storage, or policy outcomes.
- The adapter advances simulation using a fixed-step accumulator rather than feeding variable render-frame timing directly into the simulation.
- Authoritative spatial state is stored in metres. The adapter converts it to Unreal centimetres exactly once when synchronizing the owning probe pawn's presentation transform.

## Opening the project

Open `Everward.uproject` with Unreal Engine 5.8. Generate project files/build the C++ module when prompted.

The Phase 2 runtime bootstrap supplies `AEverwardGameMode` and one default `AEverwardProbePawn`. The pawn owns exactly one visible engine-sphere presentation, one `UProbeSimulationAdapter`, and a third-person camera; the adapter constructs the canonical EV-0001 loadout. The engine sphere is an explicit bootstrap placeholder, not the final probe art direction.

The adapter now synchronizes the pawn's actor location from `SimulationCore::snapshot().position_m` after deterministic fixed-step simulation advancement. The pawn does not author its own mechanical position. This is the first visible embodiment link between authoritative simulation truth and the Unreal presentation.

The project still contains no authored production map or original production assets. `DefaultEngine.ini` selects the production game mode, so Unreal's startup map can spawn the single probe without duplicating simulation ownership.

The next Phase 2 presentation slice is the minimal inspect/HUD read model for mass, energy, temperature, storage, velocity, and simulation time, followed by Unreal adapter/Blueprint exposure of scanning and power-management commands.
