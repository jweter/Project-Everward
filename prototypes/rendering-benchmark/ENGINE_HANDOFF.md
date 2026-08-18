# Rendering Engine Handoff Contract

## Purpose

This contract is the final renderer-neutral boundary before the Godot and Unreal benchmark implementations. It prevents either engine prototype from silently inventing different benchmark truth.

The benchmark simulation layer owns object placement, camera timing, accelerated simulation time, animation phases, HUD telemetry, and scenario identity. Engine projects consume those values and decide only how to render them.

## Generate the handoff bundle

From the repository root:

```bash
python prototypes/rendering-benchmark/export_handoff.py \
  --output prototypes/rendering-benchmark/handoff.json
```

The exporter reads the canonical `scenario.json` and `scene_manifest.json`. Do not hand-edit generated frame truth to make one engine easier to implement.

## Required engine behavior

Both engine implementations must:

1. load or faithfully reproduce the handoff bundle values;
2. place canonical scene objects from the manifest-derived state;
3. use the specified camera stage and transform at the same benchmark times;
4. drive moving machinery, debris pulses, and asteroid rotation from the supplied normalized animation phases;
5. display HUD values from supplied benchmark truth rather than an independent engine timer;
6. preserve the scenario name/version in captured evidence;
7. implement all required scene features listed in the handoff bundle;
8. keep authoritative benchmark/simulation state outside renderer-specific presentation code.

## Sample frames

The bundle includes samples immediately before and at both 40-second camera transitions, plus intermediate points throughout the 120-second capture. These samples are conformance fixtures, not the only times the engine may update.

An engine implementation should be able to demonstrate that its rendered state matches the canonical fixture at these sample times. Continuous playback between samples remains driven by the same canonical timing rules.

## What may differ

The engines may use their native rendering systems, materials, lighting implementation, particles, UI framework, asset pipeline, profiler, and packaging workflow. Those differences are part of the evidence being measured.

They may not simplify or omit a canonical requirement merely because one engine makes that requirement harder.

## Completion condition

The handoff layer is complete when the exporter is deterministic, CI validates its scenario binding and camera-transition coverage, and both engine prototypes can use the same generated contract without changing simulation truth.

After this point, the next Prototype C work should be engine-side scene construction and measured capture, not additional renderer-neutral scoring infrastructure unless a real implementation gap is discovered.
