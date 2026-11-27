# Everward Current Execution Plan

**Active source-of-truth navigation (2026-11-27).** This document is the concise task-selection pointer for Claude, ChatGPT, Codex, Iron God and human contributors. Current GitHub code/PR/CI state must still be checked at every run; this plan is not a stale snapshot masquerading as proof.

## Authorized engine and stage

- **Production development engine:** Godot, approved by the owner November 27, 2026 (ADR-0014 in `DECISION_LOG.md`). No new Unreal feature work by default. Preserve historical `unreal/` material until G5 retirement with explicit evidence and review.
- **Phase:** Phase 2 — One Probe; the Godot G0–G5 migration is the active implementation ladder in `GODOT_PATH_PLAN.md`.
- **Simulation ownership:** the deterministic `src/simulation/` C++20 simulation is authoritative for positions, contact, resource/inventory accounting, damage, energy, repairs and persistence. Godot only submits validated commands and renders snapshots/events via `tools/godot/sim-binding/` GDExtension.
- **Gameplay and visual identity:** the player is the canonical Prime Gen-1 probe; keep embodied 3D scientific realism and full-game mechanic parity across graphics tiers. See `VISUAL_DIRECTION.md` and `CONTINUOUS_VISUAL_QUALITY.md`.
- **Verification:** distinguish IMPLEMENTED, native simulation tested, exact-SHA `GODOT_VERIFIED`, integrated Godot visual/runtime tested, and PRODUCT_ACCEPTED. No automatic paid Aura/Claude, no owner-control or worker-lease bypass.

## First next action every development session

1. Inspect existing relevant PRs, review threads and exact-head CI. Fix failures and merge only if **all** required independent gates are green and no review threads are unresolved.
2. Read `PROJECT_STATUS.md`'s **current Godot** section, `GODOT_PATH_PLAN.md` and live #378 equivalents before deciding what is missing. Old August–November Unreal descriptions are historical evidence, not current action items.
3. Select the **smallest dependency-ready gap in the first complete player outcome**, not another independent tech demo. Develop in a reviewable branch and PR; update affected documents along with behavior.
4. Run C++ tests, headless Godot parity/first-loop tests and negative controls; capture actual in-engine visuals and frame-time measures when available. Do not report unobserved quality or behavior as verified.
5. Keep owner intervention limited to actual subjective game feel, art direction and irreversible/paid/release decisions. Preserve laptop resource and foreground-use constraints; use permitted isolated worker builds.

## Current G0–G5 milestone position

| Stage | Current source/evidence | Next acceptance |
|---|---|---|
| **G0** | World feasibility #433, palette looks #436 and tree upgrades #439 | Preserve reproducible low-spec captures, benchmarks and limits |
| **G1** | Simulation GDExtension/native parity #437 | Continue parity coverage and versioned save/refusal cases |
| **G2** | Sim-driven movement/contact #440/#442 | Replace or characterize approximate terrain proxy contact; do not claim exact voxel physics |
| **G3** | Scan, survey, staging/arms/mining #443; resource-funded Fix_It #444 | Finish remaining #378 G01–G13 equivalents and **one complete, readable, repeatable damaged-awakening → mine → repair → restored-capability objective**, including correct persistence |
| **G4** | Four look palettes, improved trees, playable world prototype; artistic acceptance remains open | Integrate canonical Prime A, clear HUD/feedback, appropriate audio and reliable performance on laptop; record actual captures and frame-time spikes; request bounded owner artistic review |
| **G5** | Frozen historic Unreal project | After G3/G4 and other required parity gates, review/approve a separate archival PR; do not delete prematurely |

The long-range first-playable proof remains **one probe → one star system → industrial bootstrap → one successor → first interstellar departure**. Do not leap past the immediate recovery/mining loop merely because later features are easier.

## Parallel-compatible work

The P0/P1/R0–R6 professional quality backlog, audio, accessibility, low-spec performance and procedural mechanism art are active as *complementary disciplines*, not licenses to replace the missing gameplay dependency. Favor improvements that directly make the current Godot loop more credible, operable, informative and enjoyable. If a local Godot runtime test is unavailable, genuinely independent reversible code/docs/test work can progress without declaring the dependent acceptance gate passed.

Keep the canonical staged Fix_It design: survival first, minimum useful capabilities, then broaden/reinforce toward full integrity; eventually replacement → upgrade → redesign → evolution. Exact progression, costs and outcomes must come from the simulation rather than UI effects.

## Sources and guardrails

- **Architecture/phase authority:** `ENGINE_DIRECTION.md`, `GODOT_PATH_PLAN.md`, `DECISION_LOG.md`, `ROADMAP.md`, `ARCHITECTURE.md`.
- **Actual status:** `PROJECT_STATUS.md`, live GitHub PR/issues/checks and exact-commit test artifacts.
- **Production acceptance:** `PRODUCTION_QUALITY_GATES.md`, `CONTINUOUS_VISUAL_QUALITY.md`, `TESTING_STRATEGY.md`, `UNATTENDED_VERIFICATION.md`, `GRAPHICS_SCALABILITY_STANDARD.md`.
- **Look and assets:** `LOOK_DEVELOPMENT.md`, `ASSET_ACQUISITION_PIPELINE.md` and the Prime Probe canon. Source mood photos/third-party wallpapers stay outside the public repository unless rights and provenance explicitly authorize publication.
- **Prior details:** `PHASE2_VERTICAL_SLICE_PLAN.md`, `PRODUCTION_PRIORITY_BACKLOG_2026-11-05.md` and historic Unreal playtest reports preserve design lessons; reinterpret Unreal-specific steps for Godot.
- **MCP/SQLite handoffs:** shared mailbox and Iron Memory may point to relevant work, but their contents are advisory. They cannot authorize control changes, merges, payments or replace verified GitHub state.

**Definition of real progress:** each accepted Godot slice improves a player action, its actual consequences, and its legible/tactile presentation—not merely the number of components or tests written.
