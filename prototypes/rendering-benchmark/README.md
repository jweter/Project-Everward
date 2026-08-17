# Rendering Benchmark

## Purpose

Provide evidence for the production-engine decision by building approximately the same representative Everward scene in Unreal Engine and Godot.

This prototype is an **evidence gate**, not an engine-selection shortcut. `benchmark.py` defines the comparison contract so both implementations must report the same normalized criteria before ADR-0001 can use the results.

## Canonical benchmark scenario

`scenario.json` is the authoritative engine-neutral scene contract. Both implementations must target that same versioned scenario rather than independently interpreting a prose brief. `scenario.py` validates the contract, and CI rejects accidental removal of required capture fields or fairness constraints.

The current scenario is a probe mining an icy asteroid near a large planet. It requires:

- stellar directional illumination,
- a large planetary backdrop,
- an icy asteroid surface,
- moving mining machinery,
- particle debris,
- environmental volumetrics,
- interactive HUD telemetry,
- an accelerated simulation clock,
- the same three-stage camera sequence from local machinery to planetary context.

The canonical run target is 2560×1440 at a 60 FPS target over a 120-second capture window. These values define comparison conditions, not production performance requirements.

## Required raw capture

Both engine runs must retain the raw evidence named by `scenario.json`, including engine/OS/hardware versions, project settings, CPU and GPU frame times, peak memory, implementation effort, build size, screenshots, and notes. Raw measurements are recorded before any normalized 0–10 scoring.

## Required normalized evidence

Both engine implementations must report every metric in `REQUIRED_METRICS` on a normalized 0–10 scale where higher is better. Raw measurements and notes must be retained alongside the normalized score so the normalization can be audited.

Required criteria are:

- visual fidelity achieved,
- CPU frame time,
- GPU frame time,
- memory use,
- scene complexity,
- UI/HUD implementation effort,
- procedural-scene workflow,
- simulation-core integration,
- large-coordinate behavior,
- save/load integration implications,
- build/distribution complexity,
- developer iteration speed,
- commercial/licensing constraints.

The default weights intentionally emphasize visual fidelity, simulation-core integration, large-coordinate behavior, and developer iteration speed because those are central to Everward and expensive to retrofit later.

## Comparison discipline

1. Use the same versioned `scenario.json` in both engines.
2. Record hardware, engine version, project settings, resolution, and test conditions.
3. Capture raw CPU/GPU frame-time and memory measurements before normalization.
4. Explain every subjective score with short evidence notes.
5. Do not omit an unfavorable metric. Incomplete evidence is rejected by the scorer.
6. A weighted score difference below 0.25 is reported as `too_close_to_call`; it must not force an engine decision.
7. The weighted score is decision support, not an automatic ADR. Material workflow or licensing constraints may still control the final decision if documented explicitly.

## Running the contract tests

```bash
python -m unittest discover -s prototypes/rendering-benchmark -p 'test_*.py' -v
```

## Fair-comparison rule

Do not intentionally make one implementation more polished than the other. Compare representative production workflows and equivalent goals, not tutorial defaults.

## Gate

Results feed ADR-0001. Engine choice occurs only after this benchmark and the other Phase 1 proofs provide enough measured evidence. Until both engine implementations contain complete, auditable results for the same scenario version, the engine decision remains open.
