# Canonical Rendering Capture Runbook

## Purpose

Produce comparable, decision-grade Godot and Unreal evidence for the Phase 1 Everward engine gate. This runbook does not choose an engine and must not be used to invent measurements.

## Fairness controls

Use the same physical computer, display resolution, operating-system power mode, driver state, background-workload conditions, canonical `scenario.json`, camera sequence, capture duration, and intended visual target for both candidates. Do not intentionally polish one candidate beyond the other.

If any material condition differs, record it in `capture.notes` and treat the pair as non-equivalent until the difference is understood.

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
4. Execute the camera sequence in the prescribed order and timing.
5. Record representative CPU frame time and GPU frame time using the engine profiler or equivalent engine-native instrumentation.
6. Record peak process memory during the canonical run.
7. Capture screenshots proving each camera stage and the required visual features.
8. Exercise the interactive HUD and accelerated simulation clock during the run.
9. Exercise the procedural-scene workflow used to assemble or vary the benchmark content and document material friction.
10. Exercise the simulation-core integration boundary and document any engine-specific coupling required.
11. Exercise the large-coordinate approach used for the benchmark and record observed precision/workflow issues.
12. Perform a minimal save/load integration smoke test and record architectural implications.
13. Produce a distributable development build/export and record build size plus material packaging friction.
14. Stop implementation-hour tracking only after the candidate reaches equivalent benchmark completeness.

## Evidence recording

Use `evidence_template.create_evidence_template()` to create the exact field scaffold for `godot` or `unreal`. Fill every real evidence field. `notes` may be empty only when there truly is nothing additional to record.

Required evidence includes engine/OS/hardware versions, project settings, CPU and GPU frame times, peak memory, implementation hours, build size, screenshots, and notes.

Do not convert measurements into normalized scores manually. After capture, validate the completed object with `run_record.py`; only validated raw records may flow into normalization and the decision packet.

## Pair-comparison gate

Before scoring, verify:

- both records reference the same scenario name and version;
- both runs used materially equivalent hardware and test conditions;
- every required capture field is present;
- screenshots are retained and inspectable;
- subjective workflow observations are supported by concrete notes;
- no metric was omitted because it favored the other candidate.

If the two captures are not genuinely comparable, rerun the affected candidate instead of adjusting scores to compensate.

## Completion condition

Prototype C is evidence-complete only when both candidate implementations have complete validated raw run records for the same canonical scenario and the resulting decision packet is decision-ready, or explicitly states that additional evidence is required because the candidates remain too close to call.
