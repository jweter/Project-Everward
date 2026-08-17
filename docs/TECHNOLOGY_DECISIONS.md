# Technology Decisions

This document records current technical constraints and open gates. It is not a place to turn assumptions into permanent architecture.

## TD-001 — Production engine

**Status:** OPEN

**Candidates:**

- Unreal Engine
- Godot

**Reason decision is open:** Everward evolved from a primarily strategic concept into an embodied, visually ambitious space simulation. Rendering quality now matters enough that engine choice must be evidence-based.

**Required benchmark evidence:**

- representative space-rendering scene,
- interactive HUD,
- procedural scene creation,
- large-coordinate handling,
- simulation integration,
- deterministic/headless execution strategy,
- save/load architecture,
- performance and memory profile,
- development complexity,
- commercial/licensing implications.

**Benchmark scene:** probe mining an icy asteroid near a large planet with stellar lighting, volumetric effects, particles, moving machinery, HUD telemetry, and accelerated time.

**Decision gate:** Phase 1 technical proofs complete.

## TD-002 — Simulation/presentation separation

**Status:** ACCEPTED

The simulation core owns mechanical truth. Rendering, UI, audio, narrative presentation, and future AI-assisted presentation consume simulation state but do not determine outcomes.

This decision must survive whichever production engine is chosen.

## TD-003 — Headless simulation

**Status:** ACCEPTED AS REQUIREMENT

Everward must support non-rendered simulation suitable for deterministic tests, long-duration runs, balance experiments, save verification, and performance analysis.

Exact implementation depends on the final architecture and engine.

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
