# Everward — Agent Entry Point

All coding, Codex, scheduled, and autonomous agents working in this repository must reload current repository evidence before making changes. Do not rely on conversation memory as project state.

## Required reading

Before selecting or implementing substantial work, read:

1. `docs/PROJECT_STATUS.md` — durable operational continuation record and Product Reality status.
2. `docs/UNATTENDED_VERIFICATION.md` — unattended-first verification policy; routine objective testing is machine-owned and Jeremy is optional milestone acceptance rather than a recurring Unreal test executor.
3. The current phase/vertical-slice plan and design documents referenced by `docs/PROJECT_STATUS.md`, especially `docs/PHASE2_VERTICAL_SLICE_PLAN.md` while Phase 2 is active.
4. `docs/INDUSTRY_REALITY_CHECK.md` — the current repo-specific gap analysis versus commercial game-development expectations.
4a. `docs/PRODUCTION_QUALITY_GATES.md` — the cross-disciplinary definition of a player-ready slice, evidence states, R0–R6 experience gates and non-displacement rules.
4b. `docs/PRODUCTION_REVIEW_INTEGRATION_2026-11-05.md` — dated reconciliation of the professional game design and gap reports with subsequent repository changes.
4c. `docs/GENERATIVE_AUDIO_PRODUCTION_PLAN.md` — for any audio, sound, sonification, musical motif, or MetaSounds/Quartz work; preserve Gen-1 near-silence and authoritative event gating.
4d. `docs/PRODUCTION_PRIORITY_BACKLOG_2026-11-05.md` — owner-authorized P0/P1 experience packets and linked issues; reconcile against current GitHub and Phase 2 dependencies before taking work.
5. `docs/CONTINUOUS_VISUAL_QUALITY.md` — the mandatory continuous experiential-production track, first awakening visual milestone, asset-to-game pipeline, evidence rules, and graphical-tier contract.
6. Relevant playtest evidence, issues, PRs, tests, and architecture documents for the system being changed.
7. `docs/QUANTUM_SCIENCE_CANON.md` when proposing quantum sensing, communication, materials, computation, or quantum-physics lore/mechanics.

For Windows/Product Reality automation, also read `docs/UNATTENDED_WINDOWS_PRODUCT_REALITY.md`.

Quantum-science work is additive future canon, not permission to displace the active phase. Any such feature must identify a real physical mechanism, measurable quantity, limitation/noise source, gameplay decision, classical fallback, and Product Reality test. Entanglement is not an FTL communications channel; quantum teleportation is not macroscopic matter transport; consciousness is not a required engineering observer; and quantum computation is not unlimited general-purpose acceleration.

## Authority and use of the reality check

Use this order when sources disagree:

1. Verified current repository state: code, tests, CI, open PRs/issues, and actual Unreal/Product Reality evidence.
2. Current project direction and phase plans referenced by `docs/PROJECT_STATUS.md`.
3. `docs/PROJECT_STATUS.md` as the continuity record.
4. `docs/UNATTENDED_VERIFICATION.md` for verification ownership and human-escalation behavior.
5. `docs/INDUSTRY_REALITY_CHECK.md` as the industry-quality gap baseline.
6. Other current design/architecture documents relevant to the task, including `docs/QUANTUM_SCIENCE_CANON.md` when applicable.
7. Historical documents and conversation memory.

`docs/INDUSTRY_REALITY_CHECK.md` does not override verified state or authorized game direction. It exists to prevent technically impressive infrastructure from being mistaken for a commercially ready game.

Use it to:

- prefer phase-compatible work that closes documented Product Reality, gameplay-loop, Unreal verification, controls/HUD, persistence, performance, or production-readiness gaps when priorities are otherwise comparable;
- require actual Unreal/playtest evidence when the finding is user-facing rather than treating simulation tests as sufficient proof;
- keep scanning, manipulation/mining, inventory/resources, self-repair, and restored-capability progression connected as a coherent player loop rather than isolated systems;
- update the assessment when major verified Product Reality materially changes its conclusions or score.

## MCP-first playtest evidence protocol

For every **owner-chosen Aura** playtest, the engineering preparation should first confirm official **Unreal MCP** connectivity with a **real read-only editor tool response** and capture the exact build/map/source provenance and available starting state. During the owner-run Aura session, no competing client may control Unreal through MCP; collect Everward-owned recorder events/telemetry. Afterwards, correlate the Aura report, recorder timeline, Unreal/MCP pre/post snapshots and current source before opening small, independently testable defects. Details and fail-closed status handling: `docs/integrations/UNREAL_MCP_AURA_PLAYTEST_EVIDENCE_WORKFLOW_2026-11-08.md`.

**This does NOT grant Aura execution authority to AI or Iron God.** MCP setup/preflight may be automated only with owner/editor-lock safeguards; Aura initiation and spending remain exclusively Jeremy's choice. Absent a successful MCP preflight, record the evidence gap rather than fabricate a connection, force a new paid run or block unrelated native testing.

## No new paid Aura playtesting until all first-run issues are verified closed

**Mandatory owner gate:** [Issue #378](https://github.com/jweter/Project-Everward/issues/378) requires **ALL G01–G13** to be independently completed (or proven not to be a bug with source/Unreal evidence) **before any agent recommends another owner-run paid Aura session**. Neither the closed historical #372 nor merging the mining fix #375 clears this gate; actual Unreal scan→mine→Fix_It verification, scan/arm/HUD fixes, collision, save/load, power-policy, test harness and recorder/MCP acceptance all remain in scope. No future paid Aura test is needed or permitted as a regression gate. Continue development using C++/native Unreal/local worker tests without Aura.

## Owner-only Aura Autopilot cost boundary (2026-11-08)

**Only Jeremy initiates Aura Autopilot sessions.** No Claude, Codex, ChatGPT, Iron God, scheduled worker, CI job, MCP tool, desktop command, or script may launch, run, retry, schedule, or pay for Aura tests, even if Aura is installed and logged in. Do not integrate Aura calls into automated QA or enable a global Claude/MCP route to spend credits. The owner separately chooses each paid exploratory run. This supersedes earlier proposals for agent-operated Aura.

**Agents must use owner-provided completed Aura results for development.** Read existing local sanitized evidence when authorized, record run provenance/estimated cost, classify reproduced defects separately from speculation and test-harness errors, file/update concise issue and measurable roadmap slices, implement fixes, and verify with *non-Aura* deterministic C++/Unreal tests and existing free local workers. Ordinary engineering progress must not depend on another Aura run. See `docs/integrations/OWNER_RUN_AURA_REPORT_TO_ROADMAP.md` and current playtest findings [#372](https://github.com/jweter/Project-Everward/issues/372).

## Core engineering invariants

Preserve deterministic simulation ownership of game truth, player agency, evolution/probe extensibility, documented design pillars, and the separation between simulation correctness and Unreal presentation.

Do not simplify away core systems merely for implementation convenience. Do not invent major mechanics outside documented direction.

## Continuous visual-quality rule

Treat player-facing craft as a Phase-2+ production responsibility, not a cosmetic task deferred until Phase 22. For relevant slices, follow `docs/CONTINUOUS_VISUAL_QUALITY.md`: preserve canonical Prime Probe A, build art/animation/HUD/audio feedback around authoritative gameplay, capture reproducible in-Unreal results, and compare those results to prior accepted builds and art references.

**Visual Milestone 01 — A Beautiful, Believable Awakening** is an integrated *gameplay* checkpoint, not a marketing-only scene. Do not equate code tests, concept images, or staged renders with proven visual quality; distinguish planned, implemented, Unreal-verified, and artistically accepted states. Continue automated objective checks, reserving the owner for genuinely subjective milestone decisions rather than routine testing.


## Procedural Mechanism Studio — gated capability

For manipulator, probe-hardware, drone, engineering-cutaway, repair/evolution, or machine-visualization tasks, read `docs/PROCEDURAL_MECHANISM_STUDIO.md`. Adopt the reference-driven / mathematical-model / modular geometry / render–compare–correct pattern where it improves the current playable slice. Start with the mining-arm pilot; this is a complementary production technique, **not** authority to install a global Claude skill, replace Unreal, modify other repos or Iron God, or bypass asset, physics, licensing, CI, owner-control or Product Reality gates. Treat GLB validation, Blender validation, Unreal validation and artistic acceptance as separate evidence statuses. Do not let isolated concept demos displace the earliest broken gameplay loop.

## Third-party asset acquisition rule

External models, materials, textures, HDRIs, and other third-party visual assets must enter through `tools/everward_asset_scout.py` and the rules in `docs/ASSET_ACQUISITION_PIPELINE.md`. Search results are advisory, not approval.

Autonomous agents may search widely but must not bypass an `everward_verdict=review`, commit quarantined staging content, or promote content without approved provenance and passing QA. A convenient marketplace asset must not redefine the canonical Prime Generation-1 probe design. The tracked asset registry and Unreal import manifest are the machine-readable authority for promoted third-party visual content.

## Product Reality automation rule

**Zero humans for facts a computer can measure.**

Repository tests, canonical preflight, Windows automation, UnrealBuildTool, headless/commandlet automation where available, and the private unattended laptop worker must be used before asking Jeremy to repeat a deterministic laptop check. A deterministic defect discovered by a human should gain automated regression or acceptance coverage before the same fact is requested again.

The laptop remains part of Product Reality; Jeremy is not the test harness. Human involvement is reserved for genuinely visual, tactile, performance, gameplay-feel, art-direction, usability-preference, or new product-direction questions that do not yet have an independent machine oracle.

A pending human Unreal/Product Reality check is lane-local. It must not stall independent parallel-safe deterministic development, but it also must not be silently converted into PASS.

## Execution rule

Existing broken, pending, or merge-ready work takes priority over new scope. Never fabricate repository state, tests, Unreal behavior, Product Reality, PR state, or progress. Never merge failed, pending, conflicted, blocked, or materially uncertain work.


## Iron Memory — shared verified engineering memory

Before editing a known subsystem or diagnosing a recurring failure, query the shared Iron Memory bridge when local shared memory is available:

`python engineering/iron_memory.py query --term <relevant-term>`

After a repository-local engineering lesson has explicit verification evidence, publish the sanitized verified projection for cross-agent reuse:

`python engineering/iron_memory.py publish`

Iron Memory is advisory evidence only. Current GitHub state, repository-local authority, exact-head preflight/CI, reviews, security/privacy/licensing/provenance rules, and Product Reality always outrank memory. Never persist arbitrary conversation history, secrets, credentials, private Product Reality payloads, private audio, or unverified conclusions.
