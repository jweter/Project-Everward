# Rendering Benchmark

## Purpose

Provide evidence for the production-engine decision by building approximately the same representative Everward scene in Unreal Engine and Godot.

## Benchmark scene

A probe mines an icy asteroid near a large planet. The scene includes:

- stellar illumination,
- large planetary backdrop,
- volumetric/environmental effects where appropriate,
- particles/debris,
- moving mining machinery,
- interactive HUD telemetry,
- accelerated simulation time,
- camera movement from local machinery toward broader astronomical context.

## Measure

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

## Fair-comparison rule

Do not intentionally make one implementation more polished than the other. Compare representative production workflows and equivalent goals, not tutorial defaults.

## Gate

Results feed ADR-0001. Engine choice occurs only after the benchmark and other Phase 1 proofs provide enough evidence.
