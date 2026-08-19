# Technology Decisions

This document records current technical constraints and open gates. It is not a place to turn assumptions into permanent architecture without an explicit project decision.

## TD-001 — Production engine

**Status:** ACCEPTED — UNREAL ENGINE

**Decision:** Everward is being developed toward **Unreal Engine** as its production presentation/runtime engine.

**Product rationale:** Everward is not intended to become a primarily 2D, 2.5D, abstract-map, or deliberately quirky-looking strategy game. Its visual identity is a first-class product requirement: cinematic, immersive, high-fidelity 3D scientific realism across local machinery, planetary environments, stellar phenomena, and large-scale space scenes.

The player should feel physically present as the probe. Space itself is part of the reward. The project therefore prioritizes an engine/toolchain capable of supporting high-end real-time 3D rendering, lighting, materials, particles, volumetrics, cinematic cameras, large environments, and a long-term path toward wallpaper-quality astronomical scenes.

**Phase 1 benchmark role:** the existing Godot/Unreal benchmark remains useful, but its purpose is now technical validation and risk discovery rather than an unconstrained product-direction vote. It should establish:

- whether the current Unreal prototype runs correctly on target development hardware;
- performance and memory characteristics of the representative Everward workload;
- large-coordinate behavior;
- simulation-core integration boundaries;
- procedural scene workflow;
- UI/HUD workflow;
- save/load implications;
- development/package friction;
- and any technical blocker serious enough to require a new explicit ADR.

A Godot benchmark result may remain as comparative evidence, but a numerically better lightweight result does **not** by itself supersede the accepted visual/product direction. Replacing Unreal requires a later explicit decision documenting a material blocker and the consequences for Everward's visual promise.

**Benchmark scene:** probe mining an icy asteroid near a large planet with stellar lighting, volumetric effects, particles, moving machinery, HUD telemetry, and accelerated time.

**Phase 1 exit constraint:** production gameplay may proceed only after a real, decision-ready hardware artifact validates **Unreal** for the current Phase 1 gate. A decision packet recommending Godot does not authorize Phase 2; it indicates that Unreal-specific blockers must be understood or the engine decision must be explicitly reconsidered.

See `ENGINE_DIRECTION.md`, `VISUAL_DIRECTION.md`, and ADR-0001 in `DECISION_LOG.md`.

## TD-002 — Simulation/presentation separation

**Status:** ACCEPTED

The simulation core owns mechanical truth. Rendering, UI, audio, narrative presentation, and future AI-assisted presentation consume simulation state but do not determine outcomes.

This decision survives the Unreal selection: Unreal is the presentation/runtime integration layer, not the owner of independent simulation truth.

## TD-003 — Headless simulation

**Status:** ACCEPTED AS REQUIREMENT

Everward must support non-rendered simulation suitable for deterministic tests, long-duration runs, balance experiments, save verification, and performance analysis.

The production Unreal integration must preserve this capability rather than forcing core simulation tests through the renderer.

## TD-004 — Deterministic procedural generation

**Status:** ACCEPTED

Unobserved space is generated deterministically from stable inputs conceptually equivalent to:

```text
universe_seed + spatial_coordinate + generation_algorithm_version
```

Persistent state stores meaningful observations and modifications, not a fully materialized infinite universe.

## TD-005 — Runtime Python

**Status:** NOT PLANNED

Python may be used for offline research, analysis, generation, conversion, or developer tooling. It should not be introduced into the shipping runtime unless a later requirement provides a compelling benefit that outweighs distribution and integration complexity.

## TD-006 — FTL travel / FTL communication

**Status:** OPEN DESIGN GATE

Early architecture assumes distance and travel time matter. Conventional instantaneous empire-wide communication is incompatible with a central design pillar.

Whether extremely advanced propulsion eventually includes speculative spacetime technologies remains a late-game design decision. The early simulation must not depend on FTL.

## TD-007 — Multiplayer

**Status:** POST-1.0 FEASIBILITY ONLY

Single-player is the 1.0 target. Architecture should avoid gratuitously making cooperative play impossible, but no multiplayer feature should distort the initial simulation, time-control, persistence, or networking design.

## TD-008 — LLMs in probe agency

**Status:** NOT CORE SIMULATION

Autonomous machine behavior should be deterministic/rule-based or otherwise mechanically testable. Do not place an LLM inside every probe as the source of game truth.

A future model may transform structured simulation events into optional narrative presentation, provided the simulation already determined the facts.

## Decision process

For unresolved decisions:

1. State the question.
2. Define measurable evidence required.
3. Build the smallest representative prototype.
4. Record results.
5. Make the decision.
6. Record consequences in `DECISION_LOG.md`.

Accepted decisions may be revisited only through another explicit decision record with evidence and consequences; they should not drift because a later tool or benchmark happens to be more convenient.