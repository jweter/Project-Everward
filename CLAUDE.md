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
5. `docs/QUANTUM_SCIENCE_CANON.md` when proposing quantum sensing, communication, materials, computation, or quantum-physics lore/mechanics.

Treat `docs/INDUSTRY_REALITY_CHECK.md` as a durable quality-gap baseline, not as a replacement for verified repository state, Product Reality evidence, or authorized game direction. Prefer phase-compatible work that closes a documented quality gap when priorities are otherwise comparable. Do not declare a gap closed merely because simulation code exists or CI passes when the report calls for Unreal build/package verification, controls/HUD usability, persistence, performance, gameplay-loop proof, or actual playtest evidence. Update the assessment when major verified Product Reality materially changes its conclusions.

`docs/QUANTUM_SCIENCE_CANON.md` is additive future-science canon, not permission to displace the current phase/milestone. Quantum-themed features must identify a real physical mechanism, measurable quantity, limitation/noise source, gameplay decision, classical fallback, and Product Reality test. Entanglement must not be used for faster-than-light messaging, quantum teleportation must not become macroscopic matter transport, and quantum computation must not be treated as infinite/general-purpose acceleration.

## Hard stop on future Aura testing until first-run repair gate passes

The owner explicitly requires **every** finding from 2026-11-08 fixed or conclusively disproven, and objectively verified in Unreal, *before he will consider another paid Aura playtest*. Use [issue #378](https://github.com/jweter/Project-Everward/issues/378) as the mandatory G01–G13 release gate, not merely closed #372. Prioritize implementation and non-Aura regression acceptance; don't suggest an additional Aura run or make it necessary for a PR. The merged #375 mining fix still requires true Unreal gameplay verification. Only the owner may ever decide to execute paid Aura after the gate clears.

## Owner-only Aura playtesting

**Do not invoke Aura Autopilot or connect its CLI/MCP/service to autonomous Claude or Iron God workflows.** Aura consumes owner-controlled credits; only Jeremy manually chooses and runs a session in Unreal. Account presence, prior successful onboarding, installed plugin and existing report are NOT consent for another run. Never schedule, retry, or auto-start Aura.

You *may* analyze completed owner-provided reports, create focused issues/PRs, debug the root causes, and perform standard unpaid simulation/Unreal tests to verify fixes. Do not make a new Aura run a prerequisite for finishing independent engineering work. Read `docs/integrations/OWNER_RUN_AURA_REPORT_TO_ROADMAP.md` and issue #372 before development driven by the first Aura run.

## Owner-run playtest: Unreal MCP before, evidence reconciliation after

Read `docs/integrations/UNREAL_MCP_AURA_PLAYTEST_EVIDENCE_WORKFLOW_2026-11-08.md` before advising or processing a future Aura playtest. The owner reports successful connection from a local terminal; this is an **owner-observed milestone**, not proof that every future session's editor/tool handshake works. Verify a real read-only Unreal MCP operation before the next chosen test and record actual map/version/SHA/dirty state and available initial state. Then **stop MCP control requests** while the owner runs Aura. Afterward, compare independent recorder/telemetry/state changes and the finished Aura report before assigning cause, classifying defects and generating bounded roadmap/repair PRs.

Never initiate Aura, run its MCP/CLI or spend Aura credits. Native local Unreal tests and conventional CI remain the free regression path; no new paid Aura run is required to finish a fix.

## Official Unreal MCP connection on Azathoth

Before using the official **Unreal MCP** connection, read `docs/integrations/CLAUDE_UNREAL_MCP_CONNECTION_HANDOFF_2026-11-08.md`. The project-scoped local `.mcp.json` in `C:\\Users\\jerem\\Documents\\Project-Everward` already points to `http://127.0.0.1:8000/mcp`, and UE5.8 plugin declarations are enabled **in the real local project's dirty checkout**. At the last check, `claude mcp get unreal-mcp` reported **Pending approval**, and no editor/server was running. An actual tool-call handshake is still REQUIRED before claiming the integration works. Start the server on demand from Unreal and obtain normal Claude project trust; do not enable auto-start or `--dangerously-skip-permissions`.

Keep `AllToolsets` disabled, prefer read-first toolset discovery, and serialize requests. Do not modify production maps/assets on the stale local `main` working tree; use an approved isolated worktree for write operations. **Unreal MCP is not Aura MCP**: Aura remains **owner-only** for paid sessions and must never be called through an agent, MCP, CLI, scheduler or worker. Current issue [#372](https://github.com/jweter/Project-Everward/issues/372) records the first Aura playtest findings for unpaid engineering follow-up.

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
