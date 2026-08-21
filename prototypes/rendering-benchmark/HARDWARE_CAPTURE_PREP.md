# Hardware Capture Preparation

## Purpose

Prepare the Phase 1 Prototype C **Unreal Engine** benchmark workspace from canonical repository state without inventing or overwriting measurements.

The preparation command:

1. exports the self-contained renderer handoff from `scenario.json` and `scene_manifest.json`;
2. creates the protected Unreal evidence scaffold bound to that scenario identity;
3. runs the repository/handoff readiness audit before Unreal is launched.

## Command

From the repository root:

```bash
python prototypes/rendering-benchmark/prepare_hardware_capture.py \
  --output-dir prototypes/rendering-benchmark/captures
```

The command creates:

- `handoff.json` — canonical renderer-neutral benchmark truth;
- `unreal-run-record.json` — incomplete Unreal evidence scaffold;
- `capture-preparation-summary.json` — scenario identity, readiness result, blockers, and remaining manual evidence.

A successful exit code means the repository is structurally ready for the real Unreal hardware benchmark. It does **not** mean Unreal has been executed and it does not make the evidence record decision-grade.

## Evidence preservation

By default the command refuses to overwrite an existing handoff or Unreal evidence record. `--overwrite` exists only for an explicitly disposable pre-capture workspace. Do not use it after real measurements or screenshots have been collected.

## Next step after preparation

Copy the generated handoff to `unreal/Content/benchmark_handoff.json`, build the UE 5.8 project, run the canonical 5-second warmup plus 120-second capture, collect engine-native GPU timing and screenshots, assemble the complete Unreal run record, then finalize the Phase 1 Unreal validation artifact.
