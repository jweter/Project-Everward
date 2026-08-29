# CLAUDE.md

## Repository Role

This repository contains Everward, the Von Neumann probe simulation/game project.

Follow the current repository roadmap, architecture, design documents, tests, issues, PRs, and documented project direction.

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
