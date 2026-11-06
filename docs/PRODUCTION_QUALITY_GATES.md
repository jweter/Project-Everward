# Everward — Cross-Disciplinary Production Quality Gates

**Status:** Owner-authorized production standard proposal (2026-11-05). Effective for review when this PR is merged; not proof that any gameplay milestone passed.
**Authority:** Additive to ROADMAP.md, PHASE2_VERTICAL_SLICE_PLAN.md, CONTINUOUS_VISUAL_QUALITY.md, UNATTENDED_VERIFICATION.md, DESIGN_PILLARS.md, and AGENTS.md. Earlier incomplete player outcomes and existing owner-control rules still govern priority.

## Why this exists

A merged code feature, a passing simulation test, a 3D asset, and a compelling game experience are four different deliverables. Everward must make each accepted playable slice a **complete player outcome** rather than a subsystem or PR count. Never count speculative systems as shipped gameplay, and never count screenshots or synthetic audio as a verified player interaction.

## Evidence status vocabulary

| State | Meaning | Required evidence |
|---|---|---|
| PLANNED | Reviewed design and dependency only | Player outcome, constraints, ownership, intended acceptance |
| IMPLEMENTED | Code/assets and relevant deterministic/contract tests exist | Exact source commit, tests and data contracts; not an Unreal pass |
| UNREAL_VERIFIED | Reproducible exact-build game run demonstrates behavior | Build SHA, map/seed, machine/preset, input sequence, logs, screenshots/video or captured signals |
| PRODUCT_ACCEPTED | Unreal proof plus applicable player comprehension, art, sound, accessibility and performance reviews passed | Signed-off milestone evidence and failures/limits disclosed |
| BLOCKED / REVIEW_REQUIRED | Critical evidence is missing or failed | Explicit failure, dependency and next safe test; never silently promote |

An acceptance gate may have separate IMPLEMENTED and UNREAL_VERIFIED markers for mechanics, art, audio, UI and persistence. Do not collapse these into one optimistic label. Older August 2026 playtest scores are historical baselines, not November performance measures.

## Complete player-facing slice contract

Each player-visible milestone must record:

1. **Player intent:** what the player tries to achieve, why it matters and the visible next opportunity.
2. **Game design:** rules, meaningful choice, failure and recovery, pacing and difficulty consequences.
3. **Mechanical authority:** engine-independent simulation state and deterministic/event tests. Unreal renders truth and submits commands, never fabricates results.
4. **Interaction design:** contextual discovery, true spatial reach, feedback, readable HUD, control override/recovery and input mapping. Do not rely on undocumented keyboard shortcuts.
5. **Experience production:** canonical Prime A and setting, coherent lighting, real animation/pose, VFX, haptics where applicable, and sound from the same authoritative event.
6. **Accessibility:** alternatives for sound-only or color-only information, readable text, remapping plan, mix options, subtitles/captions where appropriate; safety cues available from the start.
7. **Operational quality:** save/load during the action, controlled-error behavior, memory/frame-time and low-spec parity on the declared target.
8. **Verification:** exact-head unattended CI/local Unreal test and evidence; human judgment only for genuinely subjective quality and player comprehension.
9. **Review/closure:** what changed from the previous accepted build, what remains missing, linked issues, and next dependency.

A slice can be merged under the current reversible parallel-safe lane with IMPLEMENTED status when its Product Reality dependency is pending. **That does not close the milestone** or authorize a phase transition.

## Four concurrent production lanes

| Lane | Always operating | Gate examples |
|---|---|---|
| Gameplay and simulation | Movement, scan, extraction, repair, fabrication, industry, successors | Accurate, deterministic state and real cause/effect |
| Visual / player experience | Model, animation, VFX, camera, world composition, controls/HUD, onboarding | Unedited in-game capture and novice understanding |
| Quality and Product Reality | C++ tests, Unreal build, packaged smoke, input replay, saves, performance, reports | Exact SHA and explicit PASS/FAIL/REVIEW_REQUIRED evidence |
| Audio / evolving sensorium | Event cues, machine contact, generative layers, mix/accessibility | Perception-bound sound, correct event origins and fatigue/CPU checks |

Audio and visuals are not end-of-project polish. They begin with the same small canonical opening, while their ambitious future systems remain separately gated.

## Player-experience gate for the opening

**R0 — Recover trust:** physical mining/contact, named material, Fix_It action and an authoritative repair objective; a seeded, repeatable Unreal run with appropriate sensory feedback.

**R1 — Believable awakening:** recognizable damaged Prime A, restrained visual and audio language, comprehensible first-10-minute interaction, and visible restoration of a capability. The proposed first-time study is six independent sessions, with candidate targets of five completing a first repair, five identifying the newly available capability and a median desire-to-continue rating of at least 4/5. These are design targets, not observed outcomes or statistical proof.

**R2 — First industry:** processing/fabrication, storage, one useful worker, actual bottlenecks and save/restore through a 30–60-minute session.

**R3 — First successor:** distinct built body, inherited state/instructions, transfer or independent child, genealogy/history and an emotional audiovisual moment.

**R4 — Interstellar proof:** meaningful physical travel/time, arrival/discovery, delayed report, accepted start-to-finish vertical slice.

**R5 — Scalable continuation:** industry/lineage/doctrine UX, deterministic procedural variety, simulation LOD and sustained campaigns.

**R6 — Commercial alpha/beta:** packaged, compatible, accessible, balanced, localized as required, and legally reviewed release candidate.

These R gates are **cross-phase experience gates**; they do not renumber or replace the established Phase 0–29 roadmap.

## Mandatory evidence record for each R gate

- Repo and exact tested Git SHA, branch/build manifest and test timestamp.
- OS/device hardware, graphics preset, engine version, map, scenario seed, test duration.
- Declared input automation or script and expected authoritative events.
- Mechanic assertions, HUD observations and captured screenshots/video/audio where applicable.
- Save/reload and low-spec simulation equivalence.
- Runtime CPU/GPU frame time, audio performance and memory when relevant; record missing measurements honestly.
- Human creative/novice review only for subjective questions; separate from objective PASS.
- Defects, known limitations, evidence locations, final state and reviewer decision.

The idle Windows worker currently supplies only a subset of this evidence; startup survival is not a packaged playable pass. Do not put the owner back into repetitive routine testing. Only escalate subjective or safety-sensitive acceptance.

## Constraints protected during implementation

- All seven canonical design pillars and the player-as-probe identity.
- Canonical scientific Prime Probe A and visibly meaningful Repair → Replacement → Upgrade → Redesign → Evolution via Fix_It.
- Unreal Engine 5.8 production direction; C++ simulation owns game truth.
- Direct → Task → Operational → Doctrine control without arbitrary machine-count caps, using LOD not unlimited physical simulation.
- Single-player PC-first, offline-capable game; native mobile remains an independent future project.
- Existing asset provenance, owner-control gates, protected branches and Iron God scheduling.
- Existing phase order, earliest incomplete playable objective, independent parallel-safe work and no false Product Reality claims.

## Production decision rule

For each candidate work packet, ask: **Which incomplete player outcome will this demonstrably improve?** Prefer the earliest broken playable path, then the smallest independent change that advances visual/audio/UX or evidence for that same path. A future-system idea is not permission to postpone the damaged awakening.
