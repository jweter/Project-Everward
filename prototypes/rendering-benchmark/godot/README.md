# Godot Prototype C adapter

This directory is the first engine-side implementation of Everward's Phase 1 rendering benchmark. It consumes the renderer-neutral handoff bundle; it does not recreate simulation truth independently.

## Prepare the canonical input

From `prototypes/rendering-benchmark`:

```bash
python export_handoff.py --output godot/benchmark_handoff.json
```

`benchmark_handoff.json` is generated evidence input and should be regenerated from the authoritative scenario/manifest whenever those contracts change. Do not hand-edit it.

## Run

Open this directory as a Godot 4 project and run `main.tscn`. The adapter:

- validates handoff version 2 and the canonical `icy-asteroid-mining` scenario;
- places the asteroid, probe, planet, and directional star light from the handoff;
- executes the canonical camera sequence and stage durations;
- derives asteroid and mining-mechanism animation phase only from benchmark elapsed time and handoff periods;
- displays the handoff-defined accelerated simulation time in HUD telemetry.

## Visual-feature shell

The scene now contains concrete Godot implementations for every canonical presentation class needed before profiling: stellar directional illumination, the large planet backdrop, an icy asteroid material, visible moving mining machinery, GPU debris particles, environmental volumetric fog, and HUD telemetry. The mining arm motion is still deterministic and derives from the canonical `mining_mechanism` animation period; particles and volumetrics are presentation-only and do not alter benchmark truth.

This remains a benchmark shell rather than final decision-grade art. Placeholder geometry and materials must be brought to an equivalent visual-quality target in both engines before comparative screenshots or performance measurements are accepted.

## Next benchmark work

Run the Godot project on benchmark hardware, refine equivalent-quality presentation without changing the canonical workload, and capture profiler, memory, build-size, implementation-time, screenshot, save/load, procedural-scene, and large-coordinate evidence according to `../CAPTURE_RUNBOOK.md`. The Unreal candidate must implement the same required feature set from the same handoff before engine scoring.

Do not score or claim the Godot result until the scene satisfies every canonical required feature at the agreed visual quality and the completed run record validates.
