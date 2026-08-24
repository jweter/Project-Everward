# Everward Generation-1 Prime Probe — Production Reference Package

## Status

**Canonical visual family:** Everward Generation-1 Prime Probe / Probe A — Scientific Explorer.

This directory is the visual source-of-truth package for building the first player probe in 3D. The images are production references, not disposable mood-board material. They define the intended silhouette, proportions, hardware language, subsystem placement, articulation, surface treatment, and material character unless a later accepted design decision explicitly supersedes them.

## Canonical references

1. `canonical/01_master_design_reference.jpeg` — primary visual identity and overall form.
2. `canonical/02_orthographic_six_view.jpeg` — left/right/top/bottom/front/rear modeling reference.
3. `canonical/03_dimensions_and_scale.jpeg` — approximate physical dimensions and human-scale comparison.
4. `canonical/04_component_system_callouts.jpeg` — subsystem identification and placement reference.
5. `canonical/05_deployment_configurations.jpeg` — cruise, scanning, mining, material-handling, and maintenance/fabrication states.
6. `canonical/06_manipulator_and_tools.jpeg` — manipulator articulation, end-effector, tool, clamp, material-collection, and mounting details.
7. `canonical/07_materials_and_surface_detail.jpeg` — structural metal, protective panels, thermal/ceramic surfaces, composite structure, radiator, optics, engine materials, robotic joints, and wear language.

## Exploratory references

`exploratory/` contains alternate Generation-1 concepts. These are preserved because they may contain useful engineering ideas, but they are **not** allowed to override the canonical Prime Probe silhouette or system layout without an explicit design decision.

- `design_b_industrial_alternate.jpeg` — heavier industrial/mining interpretation.
- `design_c_advanced_alternate.jpeg` — more integrated advanced-machine interpretation.

## Modeling authority order

When references disagree, use this order:

1. master design reference;
2. orthographic views;
3. dimensions and scale;
4. component/system callouts;
5. deployment configurations;
6. manipulator/tool detail;
7. materials/surface detail.

Do not infer hidden geometry from a single perspective image when another canonical sheet provides a clearer view. Where the images remain ambiguous, preserve the ambiguity as an explicit modeling decision rather than silently inventing a major subsystem change.

## Intended Unreal production pipeline

Reference package → 3D blockout → silhouette/scale review → articulated subsystem layout → production mesh → UV/material authoring → Unreal materials → collision/attachment points → animation/articulation → LOD/Nanite/performance validation → in-game visual QA against these references.

The initial blockout may simplify small details, but it should already preserve the canonical length, major masses, radiator placement, propulsion axis, manipulator envelope, sensor orientation, and recognizable Prime Probe silhouette.

## Provenance and handling

These files are project-generated concept/reference images supplied by the project owner. Preserve originals. Do not crop, paint over, resample, or recompress the canonical originals in place. Derived working images should live in a separate `derived/` or DCC-specific working directory and retain a link back to the source filename.

See `docs/asset_manifest.json` for hashes and dimensions.
