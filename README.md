# Everward

**There is always farther.**

Everward is a commercial single-player-first space exploration, automation, engineering, survival, and emergent-narrative game about inhabiting an effectively immortal self-replicating machine intelligence and expanding into a practically inexhaustible universe.

The player is not an external empire controller. **The player is the probe.** Progress is earned by observing, scanning, mining, manufacturing, researching, rewriting software, designing successor bodies, replicating, and traveling physically into the unknown.

## Current project stage

**Phase 2 — One Probe: ACTIVE.** Phase 1 technical proofs are complete. **Godot is the owner-approved production development direction as of November 27, 2026**, superseding the earlier Unreal-first decision (see ADR-0014 in `docs/DECISION_LOG.md`). Unreal feature development is frozen; the existing Unreal project remains historical/reference work until Godot acceptance gates justify its retirement.

The authoritative C++20 simulation under `src/simulation/` continues to own mechanics. Godot handles input, rendering, HUD, audio, and presentation through a GDExtension boundary. Reusing an existing simulation algorithm does not by itself establish that the equivalent Godot gameplay is accepted.

**Current evidence, not a finished-game claim:** the Godot world feasibility spike (#433), GDExtension/native simulation parity foundation (#437), the first scan → survey → approach → arm-path → mine loop (#443), and resource-funded Fix_It repair (#444) are merged. Godot headless evidence exists for these bounded paths; the complete game, all legacy acceptance gates, packaged builds, visual/artistic acceptance, and long-running player experience are **not** yet verified.

The first playable proof is intentionally small:

> One probe → one star system → industrial bootstrap → one successor → first interstellar departure.

The immediate playable priority is narrower: **wake damaged → scan → select/approach → mine → pay material and energy for Fix_It → restore capability → repeat an understandable objective.** This loop must be mechanically correct, visually legible, and enjoyable before expansion.

Aliens, warfare, megastructures, multiplayer, and infinite late-game progression are outside the first playable build.

For the active continuation point, see `docs/PROJECT_STATUS.md`, `docs/GODOT_PATH_PLAN.md`, and `docs/ENGINE_DIRECTION.md`.

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
├── .github/                  # CI, ownership, issue/PR templates
├── .editorconfig
├── .gitattributes
├── .gitignore
├── README.md
├── LICENSE                   # proprietary project notice
├── CONTRIBUTING.md
├── SECURITY.md
├── THIRD_PARTY_NOTICES.md
├── docs/
│   ├── VISION.md
│   ├── DESIGN_PILLARS.md
│   ├── ENGINE_DIRECTION.md
│   ├── GODOT_PATH_PLAN.md    # current migration stages and evidence gates
│   ├── DECISION_LOG.md       # ADR-0014 supersedes Unreal-first ADR-0001
│   ├── ROADMAP.md
│   ├── PROJECT_STATUS.md
│   ├── AGENT_DEVELOPMENT_POLICY.md
│   ├── ARCHITECTURE.md
│   ├── TESTING_STRATEGY.md
│   ├── VISUAL_DIRECTION.md
│   └── ...
├── src/
│   └── simulation/           # engine-independent authoritative C++20 rules
├── tests/                    # deterministic native simulation coverage
├── tools/
│   └── godot/
│       ├── world-spike/      # current Godot world/first-loop prototype and tests
│       ├── sim-binding/      # GDExtension, native-vs-Godot parity harness
│       └── ...               # Godot proofs and tools
├── unreal/                   # frozen former production shell; retained for reference
├── prototypes/               # historical engine-independent technical proofs
└── assets/                   # proprietary source/reference assets
```

The current Godot projects live under `tools/godot/`; this is not a claim that the final distributable Godot project layout or packaging pipeline has been accepted.

## Near-term development sequence

1. Close and verify the first-loop Godot G3 gaps, including the remaining #378 G01–G13 equivalents, with exact-commit test evidence.
2. Preserve deterministic simulation truth through the GDExtension while connecting controls, scanned targets, arms, mining, material inventory, energy, Fix_It, and persistence.
3. Run native C++ tests and automated headless Godot parity/first-loop tests; independently verify actual runtime behavior, including save/load and refusal paths.
4. Improve Godot's player-visible Prime Generation-1 probe, terrain, lighting, readable HUD, interaction feedback, and sound **alongside** mechanical correctness.
5. Meet the G4 integrated visual/performance acceptance gate on available laptop hardware; measure FPS spikes as well as averages and distinguish automated evidence from subjective art acceptance.
6. Preserve the old Unreal project until the G5 evidence and archival decision are satisfied. Then continue toward Phase 3 — One Star System.

Do not infer that a passing simulated/headless test proves game feel, appearance, or commercial readiness.

## Architecture rule

**The simulation owns truth. Presentation renders truth.**

The renderer, UI, narration, or AI-assisted presentation must never decide whether a mechanical event occurred. Simulation state and deterministic rules determine outcomes; presentation visualizes those outcomes and submits validated commands.

**Godot is the active presentation and input runtime.** The simulation core remains engine-independent and must retain headless deterministic execution for tests, replay, long-duration runs, balancing, persistence, and eventual large-scale simulation. The Godot adapter must not create parallel authoritative mechanics.

## Autonomous development policy

Scheduled and autonomous development is governed by `docs/AGENT_DEVELOPMENT_POLICY.md` and `AGENTS.md`.

- Inspect existing PRs, reviews, unresolved comments and exact-head CI before starting new work.
- Repair failed or blocked existing work before expanding the roadmap.
- Use small reviewable branches and PRs; never treat local test success alone as merge authority.
- Prefer Godot-native gameplay, parity, screenshot, visual and performance evidence for the active production path.
- Preserve the owner-control, resource/worker-lease, payment and security gates. **Do not activate paid Aura/Claude workflows automatically.**
- Treat legacy Unreal-only instructions as historical unless an explicitly owner-authorized maintenance task requires them.
- Update affected documentation when project truth changes.

## Local foundation check

Before opening or merging a project-foundation change, run:

```text
python tools/check_foundation.py
```

Use the applicable native C++ and Godot tests for gameplay changes as described in `docs/TESTING_STRATEGY.md` and the Godot tool READMEs.

## Repository and IP status

This repository is intentionally **public** for operational reasons. Everward remains **proprietary, all rights reserved**. Public source visibility does not grant an open-source license or permission to copy, modify, redistribute, commercialize, or create derivative works from Everward's original material.

See `LICENSE` and `docs/IP_AND_LICENSES.md` for the authoritative IP posture.
