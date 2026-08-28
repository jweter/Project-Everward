# Phase 2 Prime Functional Material Pass

## Purpose

Move the current Prime Generation-1 blockout one step closer to the canonical Probe A / Scientific Explorer visual target without pretending the blockout is final production art.

This slice is deliberately presentation-only. It does not change simulation truth, collision/contact resolution, damage, component integrity, flight behavior, manipulator mechanics, or planetary-body architecture.

## Player-visible result

The Prime probe no longer reads as one undifferentiated gray primitive assembly. Major hardware families receive restrained, physically motivated material values so the player can visually distinguish:

- load-bearing structural alloy;
- protected computation/core housing;
- reactor/power hardware;
- refractory propulsion hardware;
- dark optical/sensor surfaces;
- thermal radiator surfaces;
- maneuvering hardware;
- sensor mast hardware;
- manipulator joint/shoulder hardware.

The existing Phase-2 physical scan/contact target receives a rough, low-metallic regolith/rock treatment. Spatial reference markers remain visually distinct.

## Architecture boundary

Dynamic material instances are created in Unreal at `BeginPlay` and attached only to presentation meshes. They do not author gameplay state.

The current engine basic-shape material is used as the parent. The code writes conventional `Color` / `BaseColor`, `Metallic`, and `Roughness` parameters so the same call pattern can survive replacement with future Everward master materials.

If a parent material does not expose a parameter, Unreal safely ignores that parameter assignment; the presentation contract remains isolated from simulation mechanics.

## Why this is not the final skin

This is **Skin Pass 1 — functional materials**.

It intentionally does not include:

- final authored PBR textures;
- UV unwraps or trim sheets;
- normal-map machining detail;
- decals, fasteners, warning markings, or provenance labels;
- micrometeorite pitting;
- controlled engine heat discoloration maps;
- radiator microstructure;
- optical coating shaders;
- damage-state material blending;
- final production meshes or Nanite/LOD decisions.

Those belong to later geometry/material integration and production-art passes.

## Environment constraint

The regolith target is **not** a replacement for the planned spherical planetary body.

Everward must still progress toward:

`space -> spherical moon/planet approach -> altitude/local horizon -> solid surface collision -> descent/contact -> surface operations`

Do not expand this temporary Phase-2 scene into an infinite flat-ground architecture. The material family is reusable; the temporary geometry is not authoritative future terrain.

## Local Unreal 5.8 Product Reality test

Run the exact branch build in the normal Phase-2 test environment and verify:

1. EV-0001 still spawns, moves, rotates, scans, and contacts exactly as before.
2. Structural spine reads as metallic aerospace structure rather than the same value as every subsystem.
3. Computation/core housing is visibly darker/protected.
4. Reactor/power hardware is visually separable from the structural spine.
5. Main engine reads as a darker/refractory propulsion material family.
6. Forward sensor reads as a dark low-roughness optical/sensor surface.
7. Both thermal radiators match one another and read as a distinct high-roughness thermal surface.
8. Maneuvering pods remain paired and visually distinct from radiators and optics.
9. Manipulator shoulder mounts read as precision joint hardware.
10. The physical scan/contact target reads as rough rock/regolith rather than generic engine gray.
11. Contact/damage behavior is unchanged; visual materials do not create or remove collision.
12. No material unexpectedly renders neon, emissive, transparent, or physically implausible.
13. Record screenshots from front three-quarter, side, rear three-quarter, and close sensor/radiator views for comparison with the canonical Probe A references.

## Acceptance rule

This slice is **implemented, Product Reality pending** until the local Unreal test above passes.

A visual failure is repaired in presentation code/material setup only. Any collision, orientation, control, or damage failure still outranks this cosmetic pass and must be treated as the higher-priority defect.

## Next visual/physical-world lane

After acceptance, the intended non-conflicting sequence remains:

1. finish visible manipulator geometry/articulation;
2. retain this functional material differentiation on new arm geometry;
3. object selection and physical interaction;
4. dedicated zero-g test scene;
5. spherical planetary-body foundation;
6. reusable moon/regolith/ice material family on real spherical terrain;
7. controlled descent, solid surface contact, and near-surface operations;
8. sample/resource/mining interaction.
