# Rendering Benchmark

## Purpose

Validate Everward's accepted **Unreal Engine** production direction with representative, measured runtime evidence before Phase 2 gameplay architecture hardens around it.

This prototype is no longer an engine-selection comparison. Unreal is the production renderer. The benchmark exists to prove that the canonical Everward scene, simulation handoff, capture instrumentation, large-coordinate approach, packaging path, and visual systems work together on real hardware.

## Canonical benchmark scenario

`scenario.json` is the authoritative renderer-neutral simulation/presentation contract. The current scenario is a probe mining an icy asteroid near a large planet and requires:

- stellar directional illumination,
- a large planetary backdrop,
- an icy asteroid surface,
- moving mining machinery,
- particle debris,
- environmental volumetrics,
- interactive HUD telemetry,
- an accelerated simulation clock,
- the canonical three-stage camera sequence.

The canonical run target is 2560×1440, 60 FPS target, and a 120-second capture window after the 5-second warmup.

## Unreal implementation

The production benchmark project is:

`unreal/EverwardBenchmark.uproject`

The Unreal adapter consumes `Content/benchmark_handoff.json`, applies the canonical object/camera/timing truth, records CPU game-thread timing and peak process memory automatically, and writes `Saved/unreal_capture_observation.json` after a complete run.

GPU frame time remains manual evidence and must come from Unreal-native profiling such as `stat GPU`, `profilegpu`, or Unreal Insights.

## Capture preparation

Run from the repository root:

```bash
python prototypes/rendering-benchmark/prepare_hardware_capture.py \
  --output-dir prototypes/rendering-benchmark/captures
```

The command creates:

- `handoff.json`,
- `unreal-run-record.json`,
- `capture-preparation-summary.json`.

A successful result means the repository is structurally ready for an Unreal hardware capture. It does not claim the benchmark has run.

## Decision-grade evidence

A complete Unreal run record must include engine/OS/hardware identity, project settings, CPU and GPU frame time, peak memory, implementation effort, build size, screenshots, and notes. `run_record.py` rejects incomplete or mismatched records.

After assembling a complete Unreal run record, finalize Phase 1 with:

```bash
python prototypes/rendering-benchmark/finalize_engine_decision.py \
  --scenario prototypes/rendering-benchmark/scenario.json \
  --unreal-record path/to/unreal-run-record.json \
  --output path/to/phase1-engine-decision.json
```

The finalizer does not reopen the engine choice. It verifies complete Unreal evidence and emits the `decision_ready` artifact consumed by `prototypes/phase1_exit_gate.py`.

## Hardware validation rule

Do not benchmark over starter-map content, duplicate lighting, unrelated landscape systems, or other accidental scene contamination. The final capture must exercise the canonical Everward workload and retain screenshots proving the camera stages and required visual systems.

## Contract tests

```bash
python -m unittest discover -s prototypes/rendering-benchmark -p 'test_*.py' -v
```

## Gate

Phase 1 is complete when the other technical prototypes are present and the Unreal benchmark has complete, real, auditable evidence that produces a `decision_ready` artifact recommending `unreal`. Then Phase 2 — One Probe is authorized.
