# Everward Roadmap

This is the master development roadmap. It preserves the core identity of physical travel, delayed information, resource-driven industry, probe genealogy, autonomous descendants, deterministic simulation, historical state, observation-driven research, and the transition from one machine to a distributed civilization.

The player is explicitly the probe. The roadmap is sequenced to prove the game from the inside out rather than beginning with late-game spectacle.

**Production engine direction:** Unreal Engine is accepted as the production target. Phase 1 technical work validated that direction, identified implementation risks, and proved the engine-independent simulation architecture before production gameplay began. Godot material may remain only as comparative benchmark evidence or historical prototype work unless a later accepted ADR explicitly supersedes ADR-0001.

## Phase 0 — Project Constitution

**Status:** complete.

**Objective:** prevent scope drift before implementation.

Deliverables:

- `VISION.md`
- `DESIGN_PILLARS.md`
- `GAMEPLAY_LOOP.md`
- `SIMULATION_PHILOSOPHY.md`
- `VISUAL_DIRECTION.md`
- `AUDIO_DIRECTION.md`
- `ENGINE_DIRECTION.md`
- `IP_AND_LICENSES.md`
- `TECHNOLOGY_DECISIONS.md`
- `GLOSSARY.md`
- `ARCHITECTURE.md`
- `TESTING_STRATEGY.md`
- `SAVE_FORMAT.md`
- `PERFORMANCE_BUDGETS.md`
- `DECISION_LOG.md`
- contribution/coding standards

**Gate:** satisfied. The project constitution and accepted technical/design boundaries are established.

## Phase 1 — Technical Proofs

**Status:** complete; exit gate satisfied.

Built isolated experiments rather than the production game.

### Prototype A — Simulation clock
Proved deterministic time, scheduled events, pause, and very high time acceleration.

### Prototype B — Procedural star system
Proved deterministic stars, planets, moons, belts, and resources.

### Prototype C — Space rendering
Built the representative Everward visual benchmark with Unreal as the production target. Comparative Godot evidence remains historical/comparative only.

### Prototype D — Massive coordinate handling
Proved local machinery, system scale, and interstellar coordinates can coexist without precision failure within the prototype acceptance criteria.

### Prototype E — Headless simulation
Proved long-duration headless deterministic simulation.

**Gate:** satisfied by the real `decision_ready` Unreal evidence artifact. Residual GPU/rendering risk remains tracked but does not block Phase 2.

## Phase 2 — One Probe

**Status:** **ACTIVE.** Production implementation began in PR #68 after the Phase 1 exit gate passed.

Current production foundation on `main`:

- top-level Unreal Engine 5.8 project under `unreal/`;
- engine-independent C++20 authoritative simulation core under `src/simulation/`;
- canonical first-probe state for identity, position/velocity, mass, energy, temperature, storage, and basic capabilities;
- deterministic fixed-step movement integration and domain-event delivery;
- `UProbeSimulationAdapter` as the single Unreal-side caller into authoritative simulation;
- Blueprint-visible simulation tick, position, and velocity-command access;
- CMake/CTest production-core coverage integrated into GitHub Actions.

Immediate development target: create the first visible embodied probe in Unreal, driven from authoritative simulation state through the adapter boundary. Then add minimal inspection telemetry, scanning, power allocation, and software-policy interaction until the complete One Probe interaction set is testable.

Phase 2 target state remains one embodied machine with mass, energy, storage, sensors, computation, propulsion, position, velocity, temperature, component capabilities, and software state.

Player must be able to observe, scan, move, inspect systems, manage power, and alter basic software policies.

The Unreal layer presents authoritative simulation state and submits commands; it must never become the owner of mechanical truth. See ADR-0002, ADR-0012, `PHASE2_KICKOFF_SCAFFOLD.md`, and `PROJECT_STATUS.md`.

**Gate:** simply existing as the probe is compelling.

## Phase 3 — One Star System

Add survey, spectroscopy, resource determination, travel between local objects, orbital context, and environmental conditions.

**Gate:** scanning is enjoyable and creates understanding rather than functioning as a progress bar.

## Phase 4 — Industrial Bootstrap

Add mining, power production, refining, storage, fabrication, and construction.

**Gate:** one probe can create a self-sustaining industrial foothold.

## Phase 5 — Research and Engineering

Research emerges from phenomena, experiments, operating experience, scanning, and material discovery. Build the first successor-design interface.

**Gate:** player can design something physically better than the current body.

## Phase 6 — First Replication

Manufacture the first successor. Choose inherited code and knowledge, instructions, architecture changes, and either consciousness transfer or independent-child instantiation.

**Gate:** replication feels consequential: “I made another intelligence.”

## Phase 7 — Interstellar Travel

Add nearby systems, trajectories, travel duration, propulsion differences, arrival events, and first extrasolar discovery.

**Gate:** arrival at another star feels enormous.

## Phase 8 — Communication Latency

Messages become physical delayed information. Children operate from local knowledge rather than omniscient player state.

**Gate:** delayed knowledge creates compelling consequences rather than only frustration.

## Phase 9 — Vertical Slice

Target: 30–60 minutes.

Player awakens, surveys, finds resources, mines, refines, manufactures, researches, designs a successor, replicates, departs for another system, arrives, and discovers something worth investigating.

Visual quality should already communicate the intended cinematic scientific-realism identity; the vertical slice is not a temporary low-fidelity 2D interpretation of the final game.

**Gate:** ready for external playtesting.

## Phase 10 — Autonomous Children

Children receive goals, priorities, constraints, code, behavior parameters, local state, knowledge, and communications. They must survive without constant micromanagement.

**Gate:** twenty autonomous probes are interesting rather than annoying.

## Phase 11 — Procedural Expansion

Implement deterministic region generation, stellar populations, planetary diversity, rare systems, extreme objects, spatial distribution, and region persistence.

**Gate:** player can travel continuously outward without authored-world boundaries.

## Phase 12 — Generational Progression

Deepen successor engineering across propulsion, sensors, computation, industry, defense, energy, thermal management, and structure. Resolve through playtesting whether modification limits use one major change, points, engineering budget, or a hybrid.

**Gate:** generation 20 is dramatically more capable than generation 1 while still presenting engineering tradeoffs.

## Phase 13 — Difficulty Framework

Implement Serenity, Explorer, Voyager, Survivor, and Abyss. Difficulty changes resource pressure, environmental lethality, recovery, hostility, and long-term threat rather than merely enemy hit points.

**Gate:** the same expedition feels meaningfully different across presets.

## Phase 14 — Machine Society

Add divergent descendants, inherited software, lineage identity, behavior/cultural drift, cooperation, independence, disputes, and specialization.

## Phase 15 — Extreme Astronomy

Prioritize black holes, neutron stars, pulsars, active stars, binaries, unusual planets, massive rings, rogue objects, and remnants.

Each major phenomenon needs:

1. visual identity,
2. physics behavior,
3. scientific value,
4. engineering challenge,
5. resource possibilities where appropriate.

## Phase 16 — Megastructures

After industry is already fun, add massive shipyards, telescope arrays, communication networks, orbital habitats, computation systems, stellar collectors, swarm-scale structures, and extreme resource engineering.

Megastructures must physically exist in space rather than act as abstract bonuses.

## Phase 17 — Other Intelligence

Only after the machine game works, introduce primitive life, technological civilizations, advanced civilizations, post-biological civilizations, and alien machines.

Observation precedes understanding.

## Phase 18 — Conflict

Develop combat from existing physics, sensor, propulsion, manufacturing, communication, and automation systems so it feels native to Everward rather than bolted on.

## Phase 19 — Infinite Progression Framework

Engineer intentionally for extremely large capability values, diminishing returns, continuing research, material escalation, unbounded upgrade identifiers, high-generation simulation, and numerical stability.

Test absurd cases such as engine levels 1, 1,000, and 1,000,000.

## Phase 20 — Endgame Without an Ending

Support player-created long-term objectives: distant exploration, impossible structures, enormous catalogs, extreme survivability, unusual civilizations, lineage perfection, and cosmological investigation.

On Abyss, late-game threats may scale far enough to challenge ancient, highly developed players.

## Phase 21 — Alpha

Required identity is complete: embodiment, scanning, resources, industry, replication, generations, interstellar expansion, descendants, communication latency, research, environments, difficulty, progression, saves, and substantial procedural space.

No missing feature should still be required to prove the fundamental game.

## Phase 22 — Visual Production

Aggressively improve Unreal-based stars, planets, atmospheres, rings, particles, structures, probes, lighting, shaders, transitions, camera, HUD, and photo mode.

Target the “wallpaper screenshot” standard.

## Phase 23 — Audio Production

Develop environmental sonification, machinery, communications, sensors, adaptive music, danger transitions, and discovery transitions.

## Phase 24 — UX and Accessibility

Scale interfaces from one probe to a vast lineage. Address readable telemetry, color accessibility, UI scaling, remapping, HUD customization, automation, descendant search, alerts, and navigation.

## Phase 25 — Optimization

Focus on the true scaling problem: simulation scale × persistence × time acceleration.

Optimize inactive-region simulation, event aggregation, distant agents, deterministic generation, save size, rendering LOD, pooling, parallelism, and data-oriented structures where appropriate.

## Phase 26 — Beta

Focus on balancing, defects, performance, onboarding, save migration, content distribution, procedural quality, and difficulty validation. Avoid major new systems.

## Phase 27 — Steam Demo

Polish awakening, scanning, mining, industrial bootstrap, first successor, and first departure. The demo must communicate the actual fantasy, not merely showcase technology.

## Phase 28 — Release Decision

Choose full 1.0 versus Early Access based on evidence. Do not commit prematurely.

## Phase 29 — 1.0

A credible 1.0 supports hundreds of hours and delivers a complete version of:

> Become an increasingly powerful immortal machine intelligence and explore an effectively unending universe.

## Post-1.0 possibilities

Potential expansions include deeper civilizations, alien ecosystems, machine cultures, stellar engineering, more megastructures, extreme astrophysics, speculative propulsion, richer programming, expanded child autonomy, modding, and cooperative multiplayer research.

## Explicitly not required for the first playable build

Do not initially build:

- aliens,
- civilizations,
- warfare,
- black-hole engineering,
- Dyson-style swarms,
- infinite-tech progression,
- multiplayer,
- millions of active systems,
- a full programming language,
- a complete soundtrack,
- Steam integration.

The first question remains:

> Is starting as one machine and bootstrapping the first self-sustaining interstellar lineage actually fun?
