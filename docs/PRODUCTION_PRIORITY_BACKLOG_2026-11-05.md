# Everward — P0/P1 Production Execution Backlog (2026-11-05)

**Status:** Owner-authorized, dependency-ordered planning baseline. PRs #353 (production gates) and #354 (generative audio) must be reviewed/merged in order before this doc becomes authoritative. No gameplay milestone, local Unreal check, asset or sound engine is claimed completed.
**Governing sequence:** Current Phase 2 and PHASE2_VERTICAL_SLICE_PLAN.md; this queue is a *quality/production complement*, not a replacement for the earliest incomplete slice. Each implementation must independently re-read current main/CI/PRs/issue state because Iron God advances the project.

## Current reconciliation (verified at planning time)

- Main advanced to commit **65206b1**, merging PR #351 for a **starter-restoration objective status**. The 2026-11-04 first-playable inventory marked that runtime objective missing; treat it as **implemented at source level and in need of actual Unreal acceptance**, not an empty task to re-implement.
- FIRST_PLAYABLE_LOOP_INVENTORY.md recorded movement, scan, manipulation, named-material collection, Fix_It repair and save/load as source-implemented; **fabrication was not implemented** at its recorded baseline.
- Issue #167 (initial save/load implementation) is **closed**; the new requirement is testing saves through the integrated opening, repair, fabrication and future lineage/migration, not reopening #167 on assumption.
- Existing open Product Reality / UX / pipeline issues remain: #159, #160, #161, #162, #163, #165, #166, #168, #228, #240, #243, #345.
- First-run scores from August 2026 are stale baselines. The current full player journey has **not** been proven by this documentation.

## Queue priority semantics

**P0-R0:** the earliest broken gameplay/recovery journey takes precedence over unrelated work. Fix a failed exact-head Unreal acceptance before promoting a dependent later slice.
**P0-support:** A0/A1 sound and cross-disciplinary polish may progress in reversible, independent branches only if the established parallel-safe-lane criteria hold. They do not preempt the first gameplay blocker.
**P1:** after or alongside independent P0 work, finish first industry and production-quality vertical-slice systems. An exploratory audio composer cannot block Phase 2.

## Dependency-ready packets

| Order | Work and outcome | Existing/new owner | Dependency / completion evidence |
|---|---|---|---|
| P0-00 | Reconcile latest main and verify #351 restored-objective output; keep exact SHA and evidence statuses accurate | #159, #168, PR #351 | Source already merged; current Unreal goal/HUD save status still requires test |
| P0-01 | Unattended actual Unreal scan → tool reach → contact → named material → Fix_It repair → restored capability and actionable next goal | #159, #161, #162, #163, #166, #168, #240/#243 | Real same-build event/HUD/video proof, no invented credits, failures readable, no manual owner test loop |
| P0-02 | Canonical Prime A mining-arm art, collision, actual tool tip, low-spec parity and coherent awakening composition | #345, #165, #228 | Implemented bridge is not Unreal/artist accepted; compare game capture vs reference |
| P0-03 | Gen-1 silence with safe UI/alerts and immutable source-event audio contract | **#355 (A0)**, PR #354 | Source-event fixture, real Unreal audible/silent conditions, accessibility & mix parity |
| P0-04 | Tactile true-contact mining and Fix_It repair sounds with interruption-safe routing | **#360 (A1)** after #355 and actual mechanics | Audio/render state traces match contact/inventory/integrity; no clipping or phantom cues |
| P0-05 | Named-matter processing and one functional fabricated item, with energy/time/storage economy | **#356**; PHASE2_VERTICAL_SLICE_PLAN.md Slice 13 | Authoritative tests + end-to-end Unreal + save/reload; must not assert successor complete |
| P1-01 | Packaged Windows build/asset/startup QA lane beyond existing UBT/NullRHI | #160, #240/#243 | Exact artifact + smoke/controls/assets and declared tested machine |
| P1-02 | Contextual action discoverability, Enhanced Input/remapping, pause/recovery and readable telemetry | #165 + active slices | First repair reachable without developer shortcut list; visual/audio alternatives |
| P1-03 | Integrated save/replay through ongoing repair, fabricated objects and first departure; schema migration | #168; preserve closed #167 | Interrupted/reloaded same events/objective/inventory and durable history |
| P1-04 | First-time-player study and revised first-hour beat sheet, following actual accepted micro-loop | **#357** | Six independent novice sessions proposed; target 5/6 first repair, 5/6 restored-capability understanding, median continue ≥4/5; report failures and fixes |
| P1-05 | Measured gameplay frame-time, audio CPU, memory and hitches on Low/High reference tiers | **#358**, #228 | Hardware-anchored budgets and exact-head regression, parity of mechanics |
| P1-06 | One sensory unlock and bounded offline algorithmic composer, never a runtime LLM dependency | **#359 (A2/A3)** after #355/#360 | Before/after valid sensorium, stable generated motif, smooth transitions, accessible mute, CPU/fatigue evaluation |

## R gates and release significance

- **R0 — Recover trust:** P0-00 to P0-02 + appropriate A0/A1 cues prove the 20–30-minute meaningful first repair, not simply a green code suite. Processing/fabrication extends later R2 while keeping P0 architectural priority.
- **R1 — Believable awakening:** visual/animation/HUD/audio cohere and first-time players understand the recovery without coaching.
- **R2 — First industry:** processing, fabrication, storage, useful worker, throughput and reliable saves.
- **R3 — First successor:** designed successor + transfer/independent child + lineage/records and rare musical event.
- **R4 — First interstellar proof:** full departure/travel/arrival/delayed information vertical slice.
- **R5/R6** remain later large-scale simulation/alpha/beta gates; cannot be entered by checking off documentation tasks.

Keep the original roadmap Phase 0–29 unchanged. R gates measure quality **across** phases; P labels drive priority but do not supersede the actual dependency graph.

## Per-packet closeout checklist

Every packet needs its own link to: player outcome and fail/recover case; simulation owner/test; Unreal build/seed and repeatable input; contextual UI+Prime animation/VFX/sound; save/settings/low-spec/accessibility implications; exact SHA CI and unattended Product Reality; documented performance evidence where relevant; subjective acceptance only when warranted; and remaining integration debt.

Acceptable states: PLANNED → IMPLEMENTED → UNREAL_VERIFIED → PRODUCT_ACCEPTED. A merge changes code, not automatically those status labels.

## Explicit non-scope

No early alien faction/combat/megascale expansion, full soundtrack, neural AI music, iOS execution, paid API, Unreal/Godot reevaluation, global owner-control changes, implicit AutoMerge authorization, or claim that #351 closed full first playable. Future R&D can continue only inside existing safe governance.

## Recommended next work selection

First inspect active PRs and exact-head CI and the latest local Product Reality evidence; repair the earliest failure. If waiting on a genuinely environmental dependency, take the highest-value **independent reversible P0 sub-slice** instead of marking the entire project blocked. Automate objective tests; owner accepts only genuine creative milestones.
