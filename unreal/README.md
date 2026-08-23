# Everward Unreal Production Project

This is the production Unreal Engine project for Phase 2 and later gameplay work. It is intentionally separate from `prototypes/rendering-benchmark/unreal/`, which remains frozen Phase 1 evidence.

## Architectural boundary

- `src/simulation/` owns mechanical truth and must remain buildable/testable without Unreal.
- `UProbeSimulationAdapter` is the only Unreal-side type authorized to call the simulation core during the One Probe kickoff slice.
- Unreal converts presentation units and renders authoritative state; it does not independently resolve mass, energy, thermal, propulsion, scan, storage, or policy outcomes.
- The adapter advances simulation using a fixed-step accumulator rather than feeding variable render-frame timing directly into the simulation.

## Opening the project

Open `Everward.uproject` with Unreal Engine 5.8. Generate project files/build the C++ module when prompted.

The Phase 2 runtime bootstrap now supplies `AEverwardGameMode` and one default `AEverwardProbePawn`. The pawn owns exactly one visible engine-sphere presentation, one `UProbeSimulationAdapter`, and a third-person camera; the adapter constructs the canonical EV-0001 loadout. The engine sphere is an explicit bootstrap placeholder, not the final probe art direction.

The project still contains no authored production map or original production assets. `DefaultEngine.ini` selects the production game mode, so Unreal's startup map can spawn the single probe without duplicating simulation ownership. The next presentation slice drives that pawn from the authoritative snapshot; the pawn must not independently author mechanical position.
