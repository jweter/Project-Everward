# Canonical Unreal Rendering Capture Runbook

## Purpose

Produce decision-grade Unreal Engine evidence for the Phase 1 Everward rendering gate. Unreal Engine is the accepted production renderer; this runbook validates that direction rather than comparing engines.

## Canonical conditions

Use the canonical `scenario.json` and generated `handoff.json`. Record the physical computer, operating-system power mode, driver state, background workload, engine version, and project settings.

Target conditions:

- Unreal Engine 5.8.x;
- 2560×1440;
- 60 FPS target;
- 5-second warmup;
- 120-second measured capture;
- 86,400 simulated seconds per real second;
- `local_mining_closeup`: 0–40 s;
- `probe_and_asteroid_medium`: 40–80 s;
- `planetary_context_wide`: 80–120 s.

## Prepare

```bash
python prototypes/rendering-benchmark/prepare_hardware_capture.py \
  --output-dir prototypes/rendering-benchmark/captures
```

Copy `captures/handoff.json` to:

`prototypes/rendering-benchmark/unreal/Content/benchmark_handoff.json`

Run the readiness audit if needed:

```bash
python prototypes/rendering-benchmark/capture_readiness.py \
  --handoff prototypes/rendering-benchmark/captures/handoff.json
```

A zero exit status means the repository is structurally ready. It is not evidence that Unreal has actually run.

## Scene acceptance

The capture must exercise the canonical Everward workload: stellar directional illumination, planetary backdrop, icy asteroid, moving mining machinery, deterministic debris, environmental volumetrics, HUD telemetry, accelerated simulation time, and all three camera stages.

Do not benchmark over unrelated starter-map landscape, duplicate lights, starter sky systems, or other accidental scene content. If such contamination is present, fix the benchmark scene and rerun.

## Capture procedure

1. Launch the Unreal project from a clean application start.
2. Let shaders/assets settle before measurement.
3. Start the benchmark.
4. Confirm the log reports the 5-second warmup completion and capture start.
5. Let the complete 120-second capture finish without pausing.
6. Record representative GPU frame time with `stat GPU`, `profilegpu`, or Unreal Insights.
7. Retain screenshots proving all three camera stages and required visual features.
8. Record project settings, implementation effort, packaging/build size, workflow notes, large-coordinate observations, save/load implications, and simulation-core integration notes.
9. Confirm the log reports that `Saved/unreal_capture_observation.json` was written.
10. Preserve that observation as immutable measured evidence.

The Unreal capture component automatically records CPU game-thread timing, peak process physical memory, engine/OS/CPU/GPU identity, RAM, capture duration, and sample count. GPU timing remains manual evidence and must not be inferred from CPU timing.

## Assemble the run record

Create manual evidence containing exactly the fields requested by the observation, then run:

```bash
python prototypes/rendering-benchmark/assemble_run_record.py \
  --scenario prototypes/rendering-benchmark/scenario.json \
  --observation path/to/unreal_capture_observation.json \
  --manual-evidence path/to/unreal_manual_evidence.json \
  --output prototypes/rendering-benchmark/captures/unreal-run-record.json
```

The assembler rejects scenario drift, missing evidence, unexpected manual fields, and attempts to override engine-measured values.

## Finalize Phase 1

```bash
python prototypes/rendering-benchmark/finalize_engine_decision.py \
  --scenario prototypes/rendering-benchmark/scenario.json \
  --unreal-record prototypes/rendering-benchmark/captures/unreal-run-record.json \
  --output prototypes/rendering-benchmark/captures/phase1-engine-decision.json
```

A complete validated Unreal record produces a `decision_ready` artifact recommending `unreal`. That artifact can then be supplied to `prototypes/phase1_exit_gate.py`.
