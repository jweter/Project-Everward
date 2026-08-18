# Canonical Rendering Capture Runbook

## Purpose

Produce comparable, decision-grade Godot and Unreal evidence for the Phase 1 Everward engine gate. This runbook does not choose an engine and must not be used to invent measurements.

## Fairness controls

Use the same physical computer, display resolution, operating-system power mode, driver state, background-workload conditions, canonical `scenario.json`, camera sequence, capture duration, and intended visual target for both candidates. Do not intentionally polish one candidate beyond the other.

If any material condition differs, record it in `capture.notes` and treat the pair as non-equivalent until the difference is understood.

## Prepare capture records

Before opening either engine, generate both scenario-bound evidence scaffolds from the canonical scenario in one command:

```bash
python prototypes/rendering-benchmark/prepare_capture.py \
  --scenario prototypes/rendering-benchmark/scenario.json \
  --output-dir prototypes/rendering-benchmark/captures
```

This creates `godot-run-record.json` and `unreal-run-record.json` with the same authoritative scenario name/version and identical required capture fields. The command refuses to overwrite existing evidence by default so an accidental rerun cannot silently erase measurements.

The generated files are intentionally incomplete and are not decision-grade evidence until real measurements replace the blank fields and `run_record.py` accepts them.

## Generate and audit the engine handoff

Generate the self-contained renderer handoff immediately before the real hardware run:

```bash
python prototypes/rendering-benchmark/export_handoff.py \
  --output prototypes/rendering-benchmark/handoff.json
```

Then run the structural readiness audit:

```bash
python prototypes/rendering-benchmark/capture_readiness.py
```

The audit verifies that both engine adapters, both capture-session implementations, the evidence-assembly/decision pipeline, and a handoff v2 artifact bound to the current canonical scenario are present. A zero exit status means the repository is structurally ready to begin hardware capture; it does **not** mean either engine has been executed or that benchmark evidence exists.

If the audit fails, resolve the listed blocker rather than compensating manually during capture. This is specifically intended to catch stale handoff artifacts and missing engine-side files before implementation time or measurements are spent.

## Canonical playback contract

Both engine implementations must now use the timing values in `scenario.json`; engine-local timing constants are not authoritative.

- total capture: 120 real seconds;
- simulation acceleration: 86,400 simulated seconds per real second (one simulated day per real second);
- `local_mining_closeup`: real seconds 0 through <40;
- `probe_and_asteroid_medium`: real seconds 40 through <80;
- `planetary_context_wide`: real seconds 80 through 120.

`playback.py` is the renderer-neutral reference implementation for deriving the active camera stage, stage progress, and accelerated simulation time from elapsed real time. Godot and Unreal do not need to embed Python; their implementations must reproduce the same state transitions from the scenario data.

This removes two benchmark ambiguities that could otherwise invalidate the comparison: different camera dwell times and different time-acceleration rates.

## Before implementation

1. Record the engine version and OS version.
2. Record CPU, GPU, and installed RAM.
3. Start implementation-hour tracking before scene work begins.
4. Read `scenario.json` and satisfy every `required_feature` without substituting a simpler scene.
5. Keep simulation truth outside presentation-specific effects; the benchmark should exercise integration rather than move authoritative rules into the renderer.

## Scene acceptance checklist

Both candidates must include the canonical icy-asteroid mining scene with stellar directional illumination, large planetary backdrop, icy asteroid surface, moving mining machinery, particle debris, environmental volumetrics, interactive HUD telemetry, accelerated simulation clock, and the full three-stage camera sequence.

Target capture conditions are 2560×1440, 60 FPS target, and a 120-second measurement window as defined by `scenario.json`.

## Capture procedure

1. Launch the candidate project from a clean application start.
2. Allow shaders/assets and the scene to settle before the timed run; do not include one-time compilation stalls unless they are representative of normal shipped play.
3. Begin the canonical 120-second run.
4. Execute the camera sequence in the prescribed order and timing from `scenario.json`.
5. Record representative CPU frame time and GPU frame time using the engine profiler or equivalent engine-native instrumentation.
6. Record peak process memory during the canonical run.
7. Capture screenshots proving each camera stage and the required visual features.
8. Exercise the interactive HUD and accelerated simulation clock at the canonical 86,400:1 simulation-to-real-second ratio during the run.
9. Exercise the procedural-scene workflow used to assemble or vary the benchmark content and document material friction.
10. Exercise the simulation-core integration boundary and document any engine-specific coupling required.
11. Exercise the large-coordinate approach used for the benchmark and record observed precision/workflow issues.
12. Perform a minimal save/load integration smoke test and record architectural implications.
13. Produce a distributable development build/export and record build size plus material packaging friction.
14. Stop implementation-hour tracking only after the candidate reaches equivalent benchmark completeness.

## Evidence recording

Each engine capture session writes an observation containing the measurements that candidate can collect reliably plus an explicit `manual_evidence_still_required` list. Treat the observation as immutable measured evidence; do not copy those measured values into a hand-edited record and do not override them manually.

Create a separate JSON object containing exactly the manual fields listed by that observation. Then assemble the decision-grade record with:

```bash
python prototypes/rendering-benchmark/assemble_run_record.py \
  --scenario prototypes/rendering-benchmark/scenario.json \
  --observation path/to/engine_capture_observation.json \
  --manual-evidence path/to/engine_manual_evidence.json \
  --output prototypes/rendering-benchmark/captures/engine-run-record.json
```

The assembler rejects scenario/version drift, incomplete evidence partitions, missing manual fields, unexpected manual fields, and attempts to override engine-measured values. The completed output must also pass the strict `run_record.py` contract before it is written successfully.

For Godot, decision-grade peak process memory remains manual because its current capture adapter records static memory only as diagnostic context. For Unreal, peak process physical memory is captured automatically and therefore cannot be replaced by manual evidence. GPU frame time remains manual for both candidates and must come from engine-native profiling.

Required final evidence includes engine/OS/hardware versions, project settings, CPU and GPU frame times, peak memory, implementation hours, build size, screenshots, and notes. Do not convert measurements into normalized scores manually; only validated raw records may flow into normalization and the decision packet.

## Pair-comparison gate

Before scoring, verify:

- both records reference the same scenario name and version;
- both runs used materially equivalent hardware and test conditions;
- both runs used the canonical camera dwell times and simulation acceleration;
- every required capture field is present;
- screenshots are retained and inspectable;
- subjective workflow observations are supported by concrete notes;
- no metric was omitted because it favored the other candidate.

If the two captures are not genuinely comparable, rerun the affected candidate instead of adjusting scores to compensate.

## Completion condition

Prototype C is evidence-complete only when both candidate implementations have complete validated raw run records for the same canonical scenario and the resulting decision packet is decision-ready, or explicitly states that additional evidence is required because the candidates remain too close to call.
