# Everward — Agent Entry Point

All coding, Codex, scheduled, and autonomous agents working in this repository must reload current repository evidence before making changes. Do not rely on conversation memory as project state.

## Required reading

Before selecting or implementing substantial work, read:

1. `docs/PROJECT_STATUS.md` — durable operational continuation record and Product Reality status.
2. The current phase/vertical-slice plan and design documents referenced by `docs/PROJECT_STATUS.md`, especially `docs/PHASE2_VERTICAL_SLICE_PLAN.md` while Phase 2 is active.
3. `docs/INDUSTRY_REALITY_CHECK.md` — the current repo-specific gap analysis versus commercial game-development expectations.
4. Relevant playtest evidence, issues, PRs, tests, and architecture documents for the system being changed.

## Authority and use of the reality check

Use this order when sources disagree:

1. Verified current repository state: code, tests, CI, open PRs/issues, and actual Unreal/Product Reality evidence.
2. Current project direction and phase plans referenced by `docs/PROJECT_STATUS.md`.
3. `docs/PROJECT_STATUS.md` as the continuity record.
4. `docs/INDUSTRY_REALITY_CHECK.md` as the industry-quality gap baseline.
5. Other current design/architecture documents relevant to the task.
6. Historical documents and conversation memory.

`docs/INDUSTRY_REALITY_CHECK.md` does not override verified state or authorized game direction. It exists to prevent technically impressive infrastructure from being mistaken for a commercially ready game.

Use it to:

- prefer phase-compatible work that closes documented Product Reality, gameplay-loop, Unreal verification, controls/HUD, persistence, performance, or production-readiness gaps when priorities are otherwise comparable;
- require actual Unreal/playtest evidence when the finding is user-facing rather than treating simulation tests as sufficient proof;
- keep scanning, manipulation/mining, inventory/resources, self-repair, and restored-capability progression connected as a coherent player loop rather than isolated systems;
- update the assessment when major verified Product Reality materially changes its conclusions or score.

## Core engineering invariants

Preserve deterministic simulation ownership of game truth, player agency, evolution/probe extensibility, documented design pillars, and the separation between simulation correctness and Unreal presentation.

Do not simplify away core systems merely for implementation convenience. Do not invent major mechanics outside documented direction.

## Execution rule

Existing broken, pending, or merge-ready work takes priority over new scope. Never fabricate repository state, tests, Unreal behavior, Product Reality, PR state, or progress. Never merge failed, pending, conflicted, blocked, or materially uncertain work.
