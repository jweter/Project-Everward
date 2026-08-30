# CLAUDE.md

## Repository Role

This repository contains Everward, the Von Neumann probe simulation/game project.

Follow the current repository roadmap, architecture, design documents, tests, issues, PRs, and documented project direction.

## Required Agent Context

Before selecting or implementing substantial work, read:

1. `AGENTS.md` — repository agent entry point and authority order.
2. `docs/PROJECT_STATUS.md` — durable operational continuation record and Product Reality state.
3. The current phase/vertical-slice plan and design documents referenced by `docs/PROJECT_STATUS.md`, especially `docs/PHASE2_VERTICAL_SLICE_PLAN.md` while Phase 2 is active.
4. `docs/INDUSTRY_REALITY_CHECK.md` — the current repo-specific gap analysis versus commercial game-development expectations.

Treat `docs/INDUSTRY_REALITY_CHECK.md` as a durable quality-gap baseline, not as a replacement for verified repository state, Product Reality evidence, or authorized game direction. Prefer phase-compatible work that closes a documented quality gap when priorities are otherwise comparable. Do not declare a gap closed merely because simulation code exists or CI passes when the report calls for Unreal build/package verification, controls/HUD usability, persistence, performance, gameplay-loop proof, or actual playtest evidence. Update the assessment when major verified Product Reality materially changes its conclusions.

## Engineering Priorities

Prefer, in order:

1. Fix failing existing PRs or tests.
2. Complete unfinished work.
3. Advance the current milestone.
4. Implement the highest-value authorized roadmap slice.
5. Refactor or clean up only when it supports current work.

Prefer small, coherent, reversible changes.

## Core Design Requirements

Preserve documented:
- design pillars
- deterministic simulation architecture
- player agency
- difficulty semantics
- procedural systems
- evolution systems
- probe and child-probe extensibility
- testing standards
- commercial-project quality

Treat simulation correctness separately from visual presentation.

Do not simplify away core simulation systems merely for implementation convenience.

Do not invent major mechanics outside the documented roadmap or design direction.

## Verification

Never claim a test, fix, PR, merge, simulation behavior, or gameplay system succeeded unless verified.

Do not merge failed, pending, conflicted, blocked, or materially uncertain work.

Run targeted tests appropriate to the change.

## Scope

Work only on files relevant to the selected task.

Do not perform unrelated cleanup.

Read only the documentation, code, logs, and tests required for the current task.

## Human Decisions

Make routine engineering decisions autonomously.

Ask Jeremy only for:
- fundamental product-direction changes
- major unauthorized architecture changes
- paid services
- license changes
- destructive migrations
- security/privacy boundary changes
- credential problems
- publishing/release authorization
- irreversible actions
- materially ambiguous product outcomes
