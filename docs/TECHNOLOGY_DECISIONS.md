# Technology Decisions

This document records current technical constraints and open gates. It is not a place to turn assumptions into permanent architecture without an explicit project decision.

## TD-001 — Production engine

**Status:** ACCEPTED — GODOT (2026-11-27; supersedes earlier Unreal direction)

**Decision:** Godot is Everward's active production development engine. The existing engine-independent C++20 simulation remains authoritative. Godot's GDExtension supplies validated simulation input and read-only presentation state; Godot scenes supply rendered 3D world, flight controls, HUD, feedback and player experience. Historical Unreal material is retained until migration G5.

**Rationale and evidence:** The owner approved the Godot path to avoid Unreal's resource-heavy development and native test bottleneck on the available laptop. The Godot world spike (#433), native/Godot parity foundation (#437), first scan/mining loop (#443), and Fix_It integration (#444) provide bounded evidence of faster development and headless testing. They do **not** prove final quality, full save compatibility, packaging, first playable completion or subjective experience.

**Non-negotiable product constraint:** Cinematic scientific realism, physically inhabited space, meaningful equipment, scalable graphics presets, and the Prime Generation-1 probe identity remain design requirements. Godot is not a pivot to an intentionally lightweight 2D/2.5D or abstract-map game.

**Current acceptance gates:** G3 Godot-first playable equivalence; G4 integrated art, player experience and performance; G5 eventual Unreal archival only after verified parity and owner approval. See `GODOT_PATH_PLAN.md`, `ENGINE_DIRECTION.md`, and ADR-0014 in `DECISION_LOG.md`.

## TD-002 — Simulation/presentation separation

**Status:** ACCEPTED

The simulation core owns mechanical truth. Rendering, UI, audio, narrative presentation, and future AI-assisted presentation consume simulation state but do not determine outcomes.

This decision survives the Godot switch: Godot is the presentation/input runtime, not the owner of independent simulation truth.

## TD-003 — Headless simulation

**Status:** ACCEPTED AS REQUIREMENT

Everward must support non-rendered simulation suitable for deterministic tests, long-duration runs, balance experiments, save verification, and performance analysis.

The production Godot integration must preserve this capability rather than forcing core simulation tests through the renderer.

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