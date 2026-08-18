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
- derives animation phase only from benchmark elapsed time and handoff periods;
- displays the handoff-defined accelerated simulation time in HUD telemetry.

The current geometry is intentionally a scene shell, not final visual evidence. It proves that Godot can consume the same deterministic scene truth that Unreal will receive without moving authoritative simulation behavior into engine objects.

## Next benchmark work

Replace placeholder meshes/materials with equivalent-quality benchmark presentation while preserving the adapter boundary. Add the required debris particles and environmental volumetrics, then capture profiler, memory, build-size, implementation-time, screenshot, save/load, procedural-scene, and large-coordinate evidence according to `../CAPTURE_RUNBOOK.md`.

Do not score or claim the Godot result until the scene satisfies every canonical required feature and the completed run record validates.
