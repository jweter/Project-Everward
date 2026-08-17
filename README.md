# Everward

**There is always farther.**

Everward is a commercial single-player-first space exploration, automation, engineering, survival, and emergent-narrative game about inhabiting an effectively immortal self-replicating machine intelligence and expanding into a practically inexhaustible universe.

The player is not an external empire controller. **The player is the probe.** Progress is earned by observing, scanning, mining, manufacturing, researching, rewriting software, designing successor bodies, replicating, and traveling physically into the unknown.

## Current project stage

Everward is in **Phase 0 — Project Constitution / Phase 1 — Technical Proofs**. The production engine is deliberately undecided until evidence from technical benchmarks supports a choice.

The first playable proof is intentionally small:

> One probe → one star system → industrial bootstrap → one successor → first interstellar departure.

Aliens, warfare, megastructures, multiplayer, and infinite late-game progression are explicitly outside the first playable build.

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
├── README.md
├── LICENSE
├── CONTRIBUTING.md
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
│   ├── ARCHITECTURE.md
│   ├── TESTING_STRATEGY.md
│   ├── SAVE_FORMAT.md
│   ├── PERFORMANCE_BUDGETS.md
│   └── DECISION_LOG.md
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

1. Lock the project constitution and terminology.
2. Establish deterministic simulation time and event scheduling.
3. Prove headless time acceleration over very long simulated periods.
4. Prove deterministic procedural astronomy from seed + coordinates + algorithm version.
5. Prove large-coordinate handling from local machinery to interstellar scale.
6. Build the same representative Everward visual benchmark in Unreal Engine and Godot.
7. Select the production engine from measured evidence.
8. Begin the One Probe implementation.

## Architecture rule

The simulation owns truth. Presentation renders truth.

The renderer, UI, narration, or future AI-assisted presentation layer must never decide whether a mechanical event occurred. Simulation state and deterministic rules determine outcomes; presentation explains or visualizes them.

## Status

Private pre-production repository. Commercial release decisions, public branding, Steam presence, and final engine selection remain future gates.
