# Everward

**There is always farther.**

Everward is a commercial single-player-first space exploration, automation, engineering, survival, and emergent-narrative game about inhabiting an effectively immortal self-replicating machine intelligence and expanding into a practically inexhaustible universe.

The player is not an external empire controller. **The player is the probe.** Progress is earned by observing, scanning, mining, manufacturing, researching, rewriting software, designing successor bodies, replicating, and traveling physically into the unknown.

## Current project stage

Everward is in **Phase 1 — Technical Proofs**. The production engine is deliberately undecided until evidence from technical benchmarks supports a choice. Phase 2 — One Probe is not authorized until the Phase 1 exit gate passes.

The first playable proof is intentionally small:

> One probe → one star system → industrial bootstrap → one successor → first interstellar departure.

Aliens, warfare, megastructures, multiplayer, and infinite late-game progression are explicitly outside the first playable build.

See `docs/PROJECT_STATUS.md` for the durable current continuation point.

## Governing design principles

1. You are the probe.
2. Hardware evolution occurs through construction and replication.
3. Progression can continue without a hard level ceiling, but power must be earned.
4. Discovery is gameplay; better instruments reveal genuinely new information.
5. Space must be worth looking at.
6. Difficulty changes the universe, not merely enemy statistics.
7. Time and distance matter, including delayed communication.

## Repository layout

```text
Project-Everward/
├── .github/                # CI, ownership, issue/PR templates
├── .editorconfig
├── .gitattributes
├── .gitignore
├── README.md
├── LICENSE                 # proprietary project notice
├── CONTRIBUTING.md
├── SECURITY.md
├── THIRD_PARTY_NOTICES.md
├── docs/
│   ├── VISION.md
│   ├── DESIGN_PILLARS.md
│   ├── GAMEPLAY_LOOP.md
│   ├── SIMULATION_PHILOSOPHY.md
│   ├── VISUAL_DIRECTION.md
│   ├── AUDIO_DIRECTION.md
│   ├── TECHNOLOGY_DECISIONS.md
│   ├── IP_AND_LICENSES.md
│   ├── GLOSSARY.md
│   ├── ROADMAP.md
│   ├── PROJECT_STATUS.md
│   ├── AGENT_DEVELOPMENT_POLICY.md
│   ├── ARCHITECTURE.md
│   ├── TESTING_STRATEGY.md
│   ├── SAVE_FORMAT.md
│   ├── PERFORMANCE_BUDGETS.md
│   ├── DECISION_LOG.md
│   └── REPOSITORY_SETTINGS.md
├── prototypes/
│   ├── simulation-clock/
│   ├── procedural-system/
│   ├── coordinate-scale/
│   ├── headless-simulation/
│   └── rendering-benchmark/
├── src/
├── tests/
├── tools/
└── assets/
```

## Near-term development sequence

1. Complete the Phase 1 technical-proof evidence set.
2. Prove deterministic simulation time and event scheduling.
3. Prove headless time acceleration over very long simulated periods.
4. Prove deterministic procedural astronomy from seed + coordinates + algorithm version.
5. Prove large-coordinate handling from local machinery to interstellar scale.
6. Build/evaluate the representative Everward visual benchmark in Unreal Engine.
7. Select the production engine from measured evidence and satisfy the Phase 1 exit gate.
8. Begin the One Probe implementation only after that gate passes.

## Architecture rule

The simulation owns truth. Presentation renders truth.

The renderer, UI, narration, or future AI-assisted presentation layer must never decide whether a mechanical event occurred. Simulation state and deterministic rules determine outcomes; presentation explains or visualizes them.

## Autonomous development policy

Scheduled and autonomous development is governed by `docs/AGENT_DEVELOPMENT_POLICY.md`.

In summary:

- inspect existing PRs and CI before new work;
- repair failing existing work before starting new roadmap work;
- merge only independently verified GREEN, fully merge-ready PRs;
- local tests alone never authorize a merge;
- when unblocked, advance one small highest-value slice of the current authorized roadmap phase;
- keep project documentation current in the same change when project truth changes;
- use branches and PRs for substantive autonomous work rather than direct commits to `main`.

## Local foundation check

Before opening or merging a project-foundation change, run:

```text
python tools/check_foundation.py
```

This is the same portable validation entry point used by GitHub Actions.

## Repository and IP status

This repository is intentionally **public** for operational reasons. Everward remains **proprietary, all rights reserved**. Public source visibility does not grant an open-source license or permission to copy, modify, redistribute, commercialize, or create derivative works from Everward's original material.

See `LICENSE` and `docs/IP_AND_LICENSES.md` for the authoritative IP posture.
