# Unreal Rendering Handoff Contract

## Purpose

This contract is the renderer-neutral simulation boundary consumed by Everward's Unreal Engine presentation layer. It prevents Unreal-specific rendering code from inventing or owning authoritative simulation truth.

The benchmark simulation layer owns object placement, camera timing, accelerated simulation time, animation phases, HUD telemetry, and scenario identity. Unreal consumes those values and decides how to render them.

## Generate the handoff bundle

From the repository root:

```bash
python prototypes/rendering-benchmark/export_handoff.py \
  --output prototypes/rendering-benchmark/handoff.json
```

The exporter reads the canonical `scenario.json` and `scene_manifest.json`. Do not hand-edit generated frame truth to change runtime behavior.

Handoff version 2 is self-contained for Unreal playback. It carries canonical object/camera definitions, camera sequence and stage durations, animation periods, feature bindings, target resolution/FPS, simulation time scale, and conformance sample frames.

## Required Unreal behavior

The Unreal implementation must:

1. load or faithfully reproduce the handoff bundle values;
2. place canonical scene objects from supplied object definitions;
3. use the supplied camera sequence, stage durations, transforms, and FOV values;
4. drive moving machinery, debris pulses, and asteroid rotation from supplied animation periods;
5. display HUD values from supplied benchmark truth rather than an independent renderer timer;
6. preserve scenario name/version in captured evidence;
7. implement all required scene features listed in the handoff bundle;
8. keep authoritative benchmark/simulation state outside presentation code.

## Sample frames

The bundle includes samples immediately before and at both 40-second camera transitions, plus intermediate points throughout the 120-second capture. These are conformance fixtures. Continuous playback is reconstructed from the same camera timing, simulation-time scale, and animation-period rules.

## Completion condition

The handoff layer is complete when the exporter is deterministic, CI validates its scenario binding and continuous-playback inputs, and the Unreal adapter reproduces the canonical contract without moving authoritative state into renderer-specific code.
