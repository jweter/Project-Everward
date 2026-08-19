# Everward Engine Direction

## Product decision

Everward is an **Unreal Engine-first** production project.

This is a product-direction decision, not merely a tool preference.

Everward must not drift into a primarily 2D, 2.5D, abstract-map, deliberately quirky, or visually lightweight interpretation simply because that path is easier to implement. The intended experience is cinematic, immersive, high-fidelity 3D scientific realism.

The player is the probe. They should feel physically present in a universe that is worth stopping to look at.

## Visual standard

Everward should pursue:

- physically convincing large-scale space environments;
- high-end real-time lighting and materials;
- volumetrics, particles, shadows, atmospheric and stellar effects;
- detailed probes, mining systems, stations, infrastructure, and eventually megastructures;
- cinematic camera composition without sacrificing gameplay legibility;
- strong local-scale embodiment near machinery, asteroids, moons, planets, rings, stars, and extreme astronomical phenomena;
- smooth transitions between local, system, and strategic scales;
- photo/HUD-off views capable of producing wallpaper-quality imagery.

The visual target remains **cinematic scientific realism**: grounded enough to feel scientifically credible, spectacular enough that exploration itself is a reward.

## What Everward is not

The production game is not intended to become:

- a 2D star-map game with occasional 3D decoration;
- a primarily icon-driven strategy interface;
- a deliberately low-poly or quirky visual experience;
- a flat presentation where astronomical phenomena are mostly abstract UI;
- a game whose core visual ambition is sacrificed simply to reduce engine complexity.

Strategic maps, telemetry, overlays, charts, lineage views, engineering interfaces, and schematic modes will still exist where they serve gameplay. They complement the physical 3D universe; they do not replace it.

## Unreal Engine role

Unreal Engine is the accepted production direction because its toolset aligns with the project's visual and presentation requirements, including high-end 3D rendering, lighting, material systems, particles, volumetrics, cinematic tools, and large-environment workflows.

The simulation architecture remains renderer-independent in principle. Unreal must consume authoritative simulation state rather than becoming the only place where mechanical truth exists.

## Phase 1 benchmark interpretation

The existing Godot/Unreal benchmark should still be completed because it provides valuable evidence about performance, integration complexity, large-coordinate behavior, memory, UI workflow, save/load implications, and development friction.

However, the benchmark is now a **validation and risk-discovery gate for Unreal**, not an unconstrained vote between two equally preferred artistic directions.

A result that shows Godot is lighter or easier does not automatically change the production engine. A result that exposes a serious Unreal blocker must trigger investigation and, if necessary, a new explicit ADR. The visual promise cannot be silently weakened to make a technical problem disappear.

## Phase 2 authorization

Phase 2 — One Probe should begin only after the Phase 1 hardware evidence is decision-ready and validates Unreal as the production engine under the current project constraints.

If the evidence does not support Unreal, Phase 1 remains open until the blocker is resolved or the product decision is explicitly reconsidered.

## Development rule

Future roadmap work, automated hourly development, architecture choices, asset planning, UI decisions, and prototype priorities should assume Unreal Engine as the production target unless a later accepted ADR explicitly supersedes this direction.