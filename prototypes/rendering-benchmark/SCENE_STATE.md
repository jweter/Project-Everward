# Canonical Rendering Scene State

## Purpose

The Phase 1 Godot and Unreal prototypes must render the same benchmark truth rather than two independently interpreted scenes. `scene_manifest.json` and `scene_state.py` define the engine-neutral object placement, camera anchors, animation phases, HUD telemetry, and scenario bindings used during the canonical 120-second benchmark.

This is intentionally not a production gameplay system. It is a parity contract for Prototype C.

## Ownership boundary

The benchmark/simulation layer owns:

- canonical object identities and positions;
- canonical camera anchors and field of view;
- camera-stage timing from `playback.py`;
- accelerated simulation time;
- deterministic mining, debris, and asteroid animation phase;
- HUD telemetry values;
- proof that every required scene feature has an explicit implementation binding.

The engine prototype owns only presentation of that truth: meshes, materials, lighting implementation, particles, volumetrics, UI widgets, shaders, camera interpolation, and rendering-specific optimization.

An engine implementation must not move authoritative benchmark state into a Godot node, Unreal actor, animation timeline, or renderer-only script when the value already exists in the canonical state.

## Engine integration

For any elapsed real time `t` inside the benchmark window:

1. Load `scenario.json`.
2. Load and validate `scene_manifest.json`.
3. Obtain `scene_state(scenario, manifest, t)`.
4. Apply the returned object, camera, animation, and HUD values to the engine representation.
5. Render and measure the result.

Both engine candidates should produce the same canonical state for the same `t`. Differences in visual quality, performance, workflow complexity, or engine integration are legitimate benchmark evidence. Differences caused by using different scene truth are not.

## Drift protection

Validation fails if:

- the manifest scenario name/version differs from `scenario.json`;
- camera definitions do not exactly cover the canonical camera sequence;
- feature bindings do not exactly cover the scenario's required scene features;
- animation periods are invalid.

Tests also verify deterministic replay, camera boundary transitions, normalized animation phases, and HUD simulation-time truth.

## Scope discipline

This contract does not choose Godot or Unreal, fabricate measurements, add gameplay logic, or begin Phase 2. It exists solely to make the Phase 1 rendering comparison reproducible enough to support the engine decision gate.
