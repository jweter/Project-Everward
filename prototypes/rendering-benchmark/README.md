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

## Capture execution

`CAPTURE_RUNBOOK.md` is the operator procedure for producing comparable Godot and Unreal evidence on real hardware. It fixes the fairness controls, scene-acceptance checklist, measurement sequence, evidence-recording rules, and pair-comparison gate.

`evidence_template.py` creates a scenario-bound scaffold with exactly the fields required by the canonical scenario. The scaffold is intentionally incomplete and is **not** valid benchmark evidence until real measurements replace its placeholders and `run_record.py` accepts the completed record.

## Required raw capture

Both engine runs must retain the raw evidence named by `scenario.json`, including engine/OS/hardware versions, project settings, CPU and GPU frame times, peak memory, implementation effort, build size, screenshots, and notes. Raw measurements are recorded before any normalized 0–10 scoring.

`run_record.py` is the audit boundary for that evidence. Each captured engine run must identify its scenario version and name, candidate engine, capture timestamp, and the complete raw evidence object. The validator rejects scenario drift, missing or unexpected capture fields, invalid measurements, empty screenshot evidence, and unsupported engine labels before those measurements can be normalized or compared.

Raw run records are evidence, not scores. A run record must remain independently inspectable even if the normalization model or benchmark weights later change.

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
4. Validate each raw run record before producing normalized scores.
5. Explain every subjective score with short evidence notes.
6. Do not omit an unfavorable metric. Incomplete evidence is rejected by the scorer.
7. A weighted score difference below 0.25 is reported as `too_close_to_call`; it must not force an engine decision.
8. The weighted score is decision support, not an automatic ADR. Material workflow or licensing constraints may still control the final decision if documented explicitly.

## End-to-end decision pipeline

Once both engine implementations have complete raw run records and explicit qualitative evidence, `pipeline.py` turns those files into a reproducible decision packet without inventing missing measurements or judgments.

Each qualitative JSON file must contain every metric listed in `QUALITATIVE_METRICS`, with exactly:

```json
{
  "visual_fidelity": {
    "score": 8.5,
    "evidence": "Side-by-side captures at the canonical camera positions."
  }
}
```

Run the pipeline with:

```bash
python prototypes/rendering-benchmark/pipeline.py \
  --scenario prototypes/rendering-benchmark/scenario.json \
  --left-record evidence/godot-run.json \
  --right-record evidence/unreal-run.json \
  --left-qualitative evidence/godot-qualitative.json \
  --right-qualitative evidence/unreal-qualitative.json \
  --output evidence/engine-decision-packet.json
```

The command validates the scenario, both raw records, and both qualitative evidence sets before normalization. Its output contains the weighted comparison, top differentiators, normalization audit, and ADR-ready decision fields. A near-tie still produces `additional_evidence_required`; the CLI cannot override the benchmark's decision threshold.

## Running the contract tests

```bash
python -m unittest discover -s prototypes/rendering-benchmark -p 'test_*.py' -v
```

## Fair-comparison rule

Do not intentionally make one implementation more polished than the other. Compare representative production workflows and equivalent goals, not tutorial defaults.

## Gate

Results feed ADR-0001. Engine choice occurs only after this benchmark and the other Phase 1 proofs provide enough measured evidence. Until both engine implementations contain complete, auditable results for the same scenario version, the engine decision remains open.
