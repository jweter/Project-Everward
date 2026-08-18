# Hardware Capture Preparation

## Purpose

Prepare the Phase 1 Prototype C Godot/Unreal benchmark workspace from canonical repository state without inventing or overwriting benchmark measurements.

The preparation command composes three existing contracts:

1. export the self-contained renderer handoff from `scenario.json` and `scene_manifest.json`;
2. create protected Godot and Unreal evidence scaffolds bound to the same scenario identity;
3. run the repository/handoff readiness audit before either engine is launched.

## Command

From the repository root:

```bash
python prototypes/rendering-benchmark/prepare_hardware_capture.py \
  --output-dir prototypes/rendering-benchmark/captures
```

The command creates:

- `handoff.json` — canonical renderer-neutral benchmark truth;
- `godot-run-record.json` — incomplete Godot evidence scaffold;
- `unreal-run-record.json` — incomplete Unreal evidence scaffold;
- `capture-preparation-summary.json` — scenario identity, readiness result, blockers, and the manual evidence still required after each engine capture.

A successful exit code means the repository is structurally ready for the real hardware benchmark. It does **not** mean either engine has been executed and it does not make the evidence records decision-grade.

## Evidence preservation

By default the command refuses to overwrite an existing handoff or engine evidence record. This is intentional: once a capture workspace contains real measurements, rerunning preparation must not silently destroy them.

`--overwrite` exists only for an explicitly disposable pre-capture workspace. Do not use it after real measurements or screenshots have been collected.

## Next step after preparation

Run the canonical Godot and Unreal benchmark scenes on the same physical hardware and equivalent test conditions. Use each engine's capture session plus engine-native profiling to collect only real evidence. Then assemble validated run records with `assemble_run_record.py`, run the decision pipeline, and finalize the engine decision artifact.
