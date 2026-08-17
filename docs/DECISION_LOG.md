# Decision Log

Everward uses lightweight Architecture/Design Decision Records (ADRs) to prevent assumptions from silently hardening into permanent constraints.

## Status values

- **OPEN** — evidence still required.
- **PROPOSED** — candidate decision awaiting review/test.
- **ACCEPTED** — current project decision.
- **SUPERSEDED** — replaced by a later decision.
- **REJECTED** — deliberately not adopted.

## ADR-0001 — Production engine selection

**Status:** OPEN

**Question:** Unreal Engine or Godot for the production game?

**Why it matters:** Everward requires both visually ambitious space rendering and a deterministic, scalable simulation architecture.

**Evidence required:**

- representative asteroid-mining benchmark in both engines,
- UI/HUD implementation effort,
- large-coordinate proof,
- deterministic/headless simulation integration,
- procedural scene generation,
- save architecture,
- performance/memory measurements,
- development complexity,
- licensing/commercial implications.

**Decision:** Not yet made.

## ADR-0002 — Simulation owns mechanical truth

**Status:** ACCEPTED

**Decision:** Mechanical outcomes are determined by the simulation core. Presentation layers consume state/events but do not independently author mechanical outcomes.

**Consequences:**

- core simulation must be runnable without graphics,
- presentation code cannot become the only source of authoritative state transitions,
- future AI-assisted narration may describe structured events but does not decide what happened.

## ADR-0003 — Deterministic procedural generation

**Status:** ACCEPTED

**Decision:** Procedural space is generated from explicit deterministic inputs including universe seed, spatial coordinate, and generation algorithm version.

**Consequences:**

- unvisited space need not be persisted,
- bug reports can identify reproducible seeds/coordinates,
- algorithm changes require versioning,
- modifications/observations must be persisted separately from base generation.

## ADR-0004 — Save data is a versioned schema

**Status:** ACCEPTED

**Decision:** Do not use blind engine-object serialization as the long-term canonical save format.

**Consequences:**

- save versions are explicit,
- migrations are required,
- persistent IDs are stable,
- migration tests become part of compatibility work.

## ADR-0005 — Communication latency remains a core mechanic

**Status:** ACCEPTED

**Decision:** Remote communication is delayed by distance. Descendants operate from local information rather than global omniscient player state.

**Consequences:**

- orders and reports are simulation objects with send/receive times,
- autonomous agents require local decision capability,
- instantaneous empire-wide command cannot be the default abstraction.

## ADR-0006 — Multiplayer target

**Status:** ACCEPTED

**Decision:** Single-player 1.0. Cooperative multiplayer may be investigated post-release but is not promised.

**Consequences:**

- do not distort early architecture around networking,
- avoid gratuitous decisions that make later co-op impossible if an equally good single-player design exists,
- time acceleration and simulation ownership remain single-player-first.

## ADR-0007 — Runtime LLM agency

**Status:** ACCEPTED

**Decision:** LLMs are not the authoritative decision engine for every probe.

**Consequences:**

- autonomous mechanics remain deterministic/testable,
- any future LLM feature is presentation or tightly bounded assistance unless a later ADR explicitly changes this rule.

## ADR-0008 — FTL

**Status:** OPEN

**Question:** Should extremely late-game travel include speculative faster-than-light or spacetime technologies?

**Current constraint:** early and midgame architecture assumes travel time and distance matter. No design may require instantaneous communication.

**Evidence required:** late-game playtesting demonstrating whether non-FTL scale remains awe-inspiring or becomes tedious.

## ADR-0009 — Successor upgrade limits

**Status:** OPEN

**Question:** How should physical changes per generation be constrained?

**Candidates:**

1. exactly one major modification,
2. modification points,
3. engineering budget,
4. hybrid model.

**Evidence required:** prototype playtesting after research/engineering systems exist.

## ADR-0010 — Repository licensing posture

**Status:** ACCEPTED

**Decision:** Private proprietary commercial development by default. No open-source license grant is implied unless a future explicit decision changes the root license.

## ADR template

Copy this section for future decisions:

```markdown
## ADR-XXXX — Title

**Status:** OPEN | PROPOSED | ACCEPTED | SUPERSEDED | REJECTED

**Question / context:**

**Options:**

**Evidence required:**

**Decision:**

**Consequences:**

**Supersedes / superseded by:**
```
