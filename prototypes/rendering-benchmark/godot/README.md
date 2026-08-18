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

The scene contains concrete Godot implementations for every canonical presentation class needed before profiling: stellar directional illumination, the large planet backdrop, an icy asteroid material, visible moving mining machinery, GPU debris particles, environmental volumetric fog, and HUD telemetry. The mining arm motion remains deterministic and derives from the canonical `mining_mechanism` animation period; particles and volumetrics are presentation-only and do not alter benchmark truth.

This remains a benchmark shell rather than final decision-grade art. Placeholder geometry and materials must be brought to an equivalent visual-quality target in both engines before comparative screenshots or final performance measurements are accepted.

## Capture-session instrumentation

`capture_session.gd` is a separate evidence-observation node. It does not alter simulation truth. When the scene starts it:

1. allows a five-second warmup for scene/shader settling;
2. asks the adapter to restart canonical playback at exactly benchmark time zero;
3. observes one complete handoff-defined capture duration;
4. records Godot `Performance.TIME_PROCESS` frame-time samples and a diagnostic static-memory high-water mark;
5. records engine, OS, CPU, GPU-adapter, and installed-memory metadata directly from the runtime;
6. writes `user://godot_capture_observation.json` after the canonical run.

The observation contains mean, p50, p95, and maximum CPU frame time plus a `run_record_prefill` section for fields that can be grounded automatically. It deliberately does **not** fabricate GPU frame time, process peak memory, implementation hours, build size, screenshots, project-setting evidence, or qualitative notes. Those remain listed under `manual_evidence_still_required` and must be captured according to `../CAPTURE_RUNBOOK.md` before `run_record.py` can validate decision-grade evidence.

The static-memory diagnostic uses Godot's debug-build memory monitor and is not substituted for the required peak-process-memory measurement. The p50 CPU frame time is offered as the automatically grounded representative CPU-frame measurement while the full distribution remains in the observation for audit.

## Next benchmark work

Run the Godot project on benchmark hardware, refine equivalent-quality presentation without changing the canonical workload, and complete the remaining profiler, GPU, process-memory, build-size, implementation-time, screenshot, save/load, procedural-scene, and large-coordinate evidence according to `../CAPTURE_RUNBOOK.md`. The Unreal candidate must implement the same required feature set from the same handoff before engine scoring.

Do not score or claim the Godot result until the scene satisfies every canonical required feature at the agreed visual quality and the completed run record validates.
