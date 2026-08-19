# Decision Log

Everward uses lightweight Architecture/Design Decision Records (ADRs) to prevent assumptions from silently hardening into permanent constraints.

## Status values

- **OPEN** — evidence still required.
- **PROPOSED** — candidate decision awaiting review/test.
- **ACCEPTED** — current project decision.
- **SUPERSEDED** — replaced by a later decision.
- **REJECTED** — deliberately not adopted.

## ADR-0001 — Production engine selection

**Status:** ACCEPTED

**Decision:** Unreal Engine is Everward's production engine direction.

**Why it matters:** Everward requires both visually ambitious space rendering and a deterministic, scalable simulation architecture. Its visual identity is not a secondary preference: the intended product is cinematic, immersive, high-fidelity 3D scientific realism in which the player physically inhabits the probe and the universe itself is part of the reward.

**Rationale:** The project has evolved beyond the original possibility of a primarily strategic 2D/2.5D presentation. Everward explicitly requires local cinematic scale, detailed machinery, planets, rings, volumetrics, stellar phenomena, extreme astronomy, cinematic camera work, and a long-term photo-mode/wallpaper-quality visual standard. That product requirement materially favors Unreal's high-end real-time 3D rendering and cinematic toolchain.

**Benchmark role:** The existing Godot/Unreal Phase 1 benchmark remains required as technical evidence. It now validates the Unreal choice and identifies material risks in performance, memory, large-coordinate handling, simulation integration, UI, procedural scene construction, save/load, packaging, and development workflow. It is no longer an unconstrained vote between two equally preferred artistic directions.

A Godot result that is lighter, faster, or easier does not automatically supersede this ADR. If the benchmark exposes a serious Unreal blocker, Phase 1 remains open until the blocker is resolved or a new explicit ADR supersedes this decision with documented consequences for the visual promise.

**Consequences:**

- Phase 2 production gameplay is authorized only by a decision-ready Phase 1 artifact validating Unreal.
- A decision-ready recommendation for Godot does not authorize Phase 2; it triggers Unreal blocker investigation or explicit reconsideration of this ADR.
- Future production architecture, asset planning, visual systems, UI integration, and automated roadmap work should assume Unreal unless this ADR is superseded.
- Simulation truth remains engine-independent in principle and must not be trapped inside presentation objects.
- Everward must not drift into a primarily 2D, 2.5D, low-fidelity, abstract-map, or deliberately quirky presentation merely because it is easier to implement.

See `ENGINE_DIRECTION.md`, `VISUAL_DIRECTION.md`, and `TECHNOLOGY_DECISIONS.md`.

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

## ADR-0010 — Repository licensing and visibility posture

**Status:** ACCEPTED

**Decision:** Everward remains proprietary, all-rights-reserved commercial software and creative work while the GitHub repository is intentionally public for operational reasons. Public visibility does not grant an open-source license.

**Consequences:**

- the root `LICENSE` remains a proprietary rights-reservation notice;
- documentation must not describe public visibility as permission to reuse, modify, redistribute, or commercialize Everward's original work;
- restricted third-party material, secrets, credentials, private data, and non-redistributable assets must never be committed;
- outside contributions require provenance/IP review before acceptance;
- any future open-source or alternative licensing decision requires a new explicit ADR.

## ADR-0011 — Scheduled autonomous development governance

**Status:** ACCEPTED

**Decision:** Scheduled Everward development follows `AGENT_DEVELOPMENT_POLICY.md` and uses `PROJECT_STATUS.md` as its durable operational continuation record.

**Consequences:**

- existing PRs and CI failures take priority over new roadmap work;
- fully GREEN merge-ready PRs may be merged automatically once fresh independent GitHub CI has succeeded, including within the same hourly run in which the PR was opened if CI has completed and every merge condition is satisfied;
- local tests alone never authorize a merge;
- at most one substantial new highest-value roadmap slice is started per hourly run;
- current roadmap phase/exit gate is preferred over later attractive work;
- substantive autonomous changes use branches and pull requests rather than direct commits to `main`;
- affected documentation must be kept current;
- significant failure memory belongs in `ERROR_RESOLUTION_LEDGER.md` and durable design/architecture rationale remains in this decision log.

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
