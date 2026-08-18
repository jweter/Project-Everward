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

Handoff version 2 is intentionally self-contained for engine playback. In addition to conformance sample frames, it carries the canonical object/camera definitions, camera sequence and stage durations, animation periods, feature bindings, target resolution/FPS, and simulation time scale. An engine adapter therefore does not need to independently reopen or reinterpret the source scenario/manifest files to reproduce continuous benchmark truth.

## Required engine behavior

Both engine implementations must:

1. load or faithfully reproduce the handoff bundle values;
2. place canonical scene objects from the supplied object definitions;
3. use the supplied camera sequence, stage durations, transforms, and FOV values;
4. drive moving machinery, debris pulses, and asteroid rotation from the supplied animation periods while matching the conformance-frame phases;
5. display HUD values from supplied benchmark truth rather than an independent engine timer;
6. preserve the scenario name/version in captured evidence;
7. implement all required scene features listed in the handoff bundle;
8. keep authoritative benchmark/simulation state outside renderer-specific presentation code.

## Sample frames

The bundle includes samples immediately before and at both 40-second camera transitions, plus intermediate points throughout the 120-second capture. These samples are conformance fixtures, not the only times the engine may update.

An engine implementation should demonstrate that its rendered state matches the canonical fixture at these sample times. Continuous playback between samples is reconstructed from the same explicit camera timing, simulation-time scale, and animation-period rules carried in the bundle.

## What may differ

The engines may use their native rendering systems, materials, lighting implementation, particles, UI framework, asset pipeline, profiler, and packaging workflow. Those differences are part of the evidence being measured.

They may not simplify or omit a canonical requirement merely because one engine makes that requirement harder.

## Completion condition

The handoff layer is complete when the exporter is deterministic, CI validates its scenario binding and continuous-playback inputs, and both engine prototypes can use the same generated contract without changing simulation truth.

The next Prototype C work after this contract is engine-side scene construction and measured capture. Additional renderer-neutral changes should only be made when an engine implementation exposes another concrete contract gap.
