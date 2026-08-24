# Everward

**There is always farther.**

Everward is a commercial single-player-first space exploration, automation, engineering, survival, and emergent-narrative game about inhabiting an effectively immortal self-replicating machine intelligence and expanding into a practically inexhaustible universe.

The player is not an external empire controller. **The player is the probe.** Progress is earned by observing, scanning, mining, manufacturing, researching, rewriting software, designing successor bodies, replicating, and traveling physically into the unknown.

## Current project stage

Everward is in **Phase 2 — One Probe**. Phase 1 technical proofs and the executable exit gate are complete, and **Unreal Engine 5.8 is the accepted production engine direction**. Production gameplay work now focuses on making one authoritative simulated probe physically present, readable, and controllable through Unreal without allowing presentation code to own mechanical truth.

The first playable proof is intentionally small:

> One probe → one star system → industrial bootstrap → one successor → first interstellar departure.

Aliens, warfare, megastructures, multiplayer, and infinite late-game progression are explicitly outside the first playable build.

See `docs/PROJECT_STATUS.md` for the durable current continuation point and `docs/ENGINE_DIRECTION.md` for the authoritative engine direction.

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
│   ├── ENGINE_DIRECTION.md
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
├── unreal/                 # production Unreal Engine project
└── assets/
```

## Near-term development sequence

1. Complete visible One Probe embodiment by driving Unreal presentation from authoritative simulation state.
2. Add a minimal inspect/HUD read model for mass, energy, temperature, storage, velocity, and simulation time.
3. Expose scanning and power-management commands through the Unreal adapter/Blueprint boundary.
4. Implement the remaining basic software-policy interaction required by the Phase 2 gate.
5. Playtest movement, observation, telemetry, scanning, and systems management until simply existing as the probe is compelling.
6. Only then advance to Phase 3 — One Star System.

## Architecture rule

The simulation owns truth. Presentation renders truth.

The renderer, UI, narration, or future AI-assisted presentation layer must never decide whether a mechanical event occurred. Simulation state and deterministic rules determine outcomes; presentation explains or visualizes them.

Unreal Engine is the production presentation/runtime integration layer. The simulation core remains engine-independent in principle and must retain headless deterministic execution for tests, long-duration runs, balancing, persistence verification, and large-scale simulation work.

## Autonomous development policy

Scheduled and autonomous development is governed by `docs/AGENT_DEVELOPMENT_POLICY.md`.

In summary:

- inspect existing PRs and CI before new work;
- repair failing existing work before starting new roadmap work;
- merge only independently verified GREEN, fully merge-ready PRs;
- local tests alone never authorize a merge;
- when unblocked, advance one small highest-value slice of the current authorized roadmap phase;
- keep project documentation current in the same change when project truth changes;
- use branches and PRs for substantive autonomous work rather than direct commits to `main`;
- assume Unreal Engine for future production-facing architecture, visual systems, asset planning, and gameplay implementation unless a later accepted ADR explicitly supersedes ADR-0001.

## Local foundation check

Before opening or merging a project-foundation change, run:

```text
python tools/check_foundation.py
```

This is the same portable validation entry point used by GitHub Actions.

## Repository and IP status

This repository is intentionally **public** for operational reasons. Everward remains **proprietary, all rights reserved**. Public source visibility does not grant an open-source license or permission to copy, modify, redistribute, commercialize, or create derivative works from Everward's original material.

See `LICENSE` and `docs/IP_AND_LICENSES.md` for the authoritative IP posture.
