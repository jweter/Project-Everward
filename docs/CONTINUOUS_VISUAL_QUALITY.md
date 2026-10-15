# Continuous Visual Quality and Production — Everward

**Status:** Accepted production direction, effective during Phase 2 and every later gameplay phase.
**Scope:** Art direction, content production, animation, lighting, UI, audio feedback, performance, and verifiable player experience.
**Companion authorities:** `ROADMAP.md`, `VISUAL_DIRECTION.md`, `PHASE2_VERTICAL_SLICE_PLAN.md`, `CURRENT_EXECUTION_PLAN.md`, `UNATTENDED_VERIFICATION.md`, `WORLD_BUILDING_CRAFT_PLAYBOOK.md`, `GRAPHICS_SCALABILITY_STANDARD.md`, and `ASSET_ACQUISITION_PIPELINE.md`.

## Why this policy exists

A technically correct simulation is not yet a convincing, enjoyable game. Passing code and tests without tangible progress in the playable experience is insufficient. An attractive concept image or a highly staged pre-rendered clip is likewise not proof of a working game.

**Everward must advance on two connected tracks from Phase 2 onward:**
1. **Mechanical/gameplay integrity:** the authoritative simulation, usable controls, progression, player choice, and deterministic correctness.
2. **Experiential production quality:** coherent art direction, credible physical materials, lighting, scale, motion, audio, readable HUD, atmosphere, and meaningful visual feedback.

These tracks converge in the **same playable Unreal build**. The project must not accumulate a polished cinematic demo that is disconnected from the actual simulated game, nor defer every visual improvement until Phase 22.

The game's permanent high-end visual promise remains: **"Pause. Hide HUD. Take screenshot. Use as wallpaper."** This is an aspiration tested against actual gameplay captures, not a claim that current builds already meet it.

## Visual identity and constraints

- Style: **cinematic scientific realism**; compelling astronomical spectacle with restraint and physical legibility. Not arbitrary decorative neon, fantasy machinery, or sterile data visualization.
- **You are the probe.** Art, camera, controls, animation, HUD, and audio must reinforce physical embodiment rather than make the player feel like an abstract strategy cursor.
- **Prime Generation-1 / Probe A — Scientific Explorer** remains the canonical original design. Preserve recognizable silhouette, body axis, functional systems, approximate proportions, manipulators, thermal hardware, instrumentation, and attachment logic. Marketplace assets and AI concepts must never quietly replace it.
- Gen-1's incomplete/neglected construction, damage, and imperfect repairs are intentional storytelling opportunities, not permission for low-quality production. Differentiation between *deliberate diegetic damage* and *unfinished game asset* must be clear.
- Component repair, replacement, redesign, added drones, and successor generations should produce legible physical changes. Visual evolution represents simulation-owned events, not cosmetic level-up illusions.
- Astronomy, local gravity, lighting, and physical materials should be credible within the game's rules. Visible impacts, manipulation, mining, repair, and motion must derive from authoritative state.
- Mood may be spacious, quiet, lonely, and sometimes almost motionless; cinematic quality does not require constant particles, bloom, or visual noise.

## A repeatable asset-to-game production pipeline

1. **Art brief and target:** identify the game mechanic, canonical reference, visual composition, required visual cues, camera distances, target platform/preset, and success/failure evidence.
2. **Concept/reference:** establish silhouette, function, proportions, materials, damage states, lighting and mood. Concept renders are references, *not* proof of an implemented asset.
3. **Geometry and asset authoring:** use Blender or an equivalent suitable DCC tool for hard-surface components, modular parts, articulation/rigging, correct pivots, UVs, normal/detail baking, and appropriate LOD strategies. Choose tools on measured workflow value; don't make any one DCC product a requirement for every asset.
4. **PBR/materials and environment:** create physically coherent roughness/metallic response, macro/meso/micro variation, scientifically plausible stellar lighting, authored landmarks, and scalable environment detail. Follow `WORLD_BUILDING_CRAFT_PLAYBOOK.md` and `VISUAL_DIRECTION.md`.
5. **Licensed acquisition:** external models, materials, textures and HDRIs must use `ASSET_ACQUISITION_PIPELINE.md`. Asset discovery and import do not substitute for artistic fit, scale, performance, or commercial provenance checks.
6. **Unreal integration:** use the production Unreal 5.8 project. Test pivots, scale, materials, rig/animation, collision match, camera behavior, integration with simulation state, quality presets, and packaging. The renderer never becomes the mechanical authority.
7. **Experiential feedback:** implement responsive manipulation, thrusters, impacts, mining, sensor presentation, HUD transitions, machine-sonification, and sound. Every important mechanical action must communicate cause and effect.
8. **Evidence and refinement:** capture gameplay, compare against approved references and last accepted build, diagnose shortcomings, improve one focused slice, and repeat.

## First production checkpoint: Visual Milestone 01 — A Beautiful, Believable Awakening

**Status:** planned; not accepted or claimed complete.

The target is a **real, interactable, reproducible Unreal gameplay sequence** that combines the canonical damaged-awakening progression with a compelling visual presentation.

**Scenario, without prematurely fixing unapproved world lore:**
- The player awakens as the damaged canonical Prime Gen-1 probe, stranded on a moon or planetary surface, sustained by limited working power and sensors.
- A deliberately composed view communicates an isolated astronomical environment. An icy moon with a huge nearby planet is a candidate *visual direction*, not mandatory world canon.
- Visible damage, imperfect machinery, lighting/material response, and partial HUD/sensor function convey machine identity.
- The player physically surveys a nearby reachable resource, selects/operates a manipulator/mining tool, and acquires real material.
- `Fix_It` recommends or performs an authorized staged self-repair using actual simulation-owned resources, power, and time.
- A damaged component visibly improves; restored capability, instrumentation or HUD feedback makes the consequence clear.
- The scene remains functional, readable, and atmospheric at gameplay camera distance, not just in a cinematic angle.

**Incremental build order:**
1. Approved gameplay-camera blockout and stable Prime silhouette with trustworthy bounds/collision.
2. One well-composed starter environment and lighting pass, with scalable PBR materials.
3. Probe damage/material/functional-component detail and rigged visible manipulators.
4. State-driven scanning, mining, contact, repair, HUD and sound feedback linked to the working gameplay loop.
5. Capture/compare/improve the same playable sequence repeatedly rather than building an unrelated marketing-only scene.

**Acceptance evidence:**
- an **unedited gameplay capture** from the production Unreal build showing the actual sequence, not a concept image or render-only trailer;
- exact tested commit/build identifier, Unreal version, machine/graphics preset, resolution, frame-time/memory results where measurable, scenario seed/state and capture metadata;
- several reproducible camera views including normal gameplay and a cinematic/HUD-hidden shot, plus short motion/audio evidence where relevant;
- correct Prime Gen-1 silhouette and clear relationship between art, manipulators, target, surface, visible collision, and authoritative mechanics;
- readable actions, selectable mining/scan targets, unmistakable cause/effect for one repair, and no critical missing meshes, clipping, floating/tiling defects, broken animation or HUD occlusion;
- the complete same mechanics work on Low/Emergency and higher presets, with cheaper effects replacing costly ones; **do not claim Ultra/Cinematic acceptance until run on qualifying hardware**;
- deterministic/Unreal checks and relevant visual evidence recorded as `PASS`, `FAIL`, `REVIEW_REQUIRED`, `PRODUCT_REALITY_REQUIRED`, or `ENVIRONMENT_FAILURE`, with unresolved blockers explicitly carried forward;
- subjective **visual quality, clarity, atmosphere, and gameplay feel** receive a milestone review; automated scores are advisory and never fabricate human aesthetic approval.

**Definition of done:** an integrated, genuine playable build presents an interesting and readable awakening with compelling visual/audio craft, verified authoritative mechanical consequences, and documented remaining gaps. Mechanical correctness alone, a series of screenshots alone, or an attractive mockup alone do not satisfy the milestone.

## Automated experiential quality loop

Default sequence:

`small gameplay + presentation slice -> fast deterministic checks -> Unreal build/smoke and unattended scenario -> screenshots/motion/audio/telemetry + perf -> compare to references and last accepted build -> diagnosis -> repair -> re-verify`

Capture artifacts may include screenshots from pinned cameras and known lighting/seed, short video, metadata, frame-time/memory statistics, mesh/material validation, HUD-safe-area checks, telemetry, and logs.

**Objective gates** should attempt to catch missing assets, shader/material failures, broken animation or actor attachment, collision/visible geometry mismatch, bad camera clipping, illegible/occluded mandatory HUD information, missing effect feedback, unexpected texture repetition, black or blank frames, crashes, frame-time or memory regressions, and tier-specific behavior drift.

Image-difference and AI critique tools can prioritize investigation, but should not be absolute pixel thresholds for dynamic space scenes or certify artistic quality by themselves. Record false positives, comparisons, evidence limits, and differences between **detected automatically**, **visually reviewed**, and **not verified**.

Follow `UNATTENDED_VERIFICATION.md`: the machine handles routine setup, builds, captures, diagnostics, regression and retries, without routing every increment through the owner. Only genuinely subjective/artistic product acceptance belongs in a periodic owner milestone review; do not turn that review into recurring manual test labor.

## Performance and graphics-access contract

Current development hardware is approximately 16 GB RAM with integrated graphics. That requires sensible local development settings, not a lower commercial art target.

- Keep the canonical full mechanic/state parity across **Emergency / Minimum -> Low -> Medium -> High -> Ultra / Cinematic -> Custom**.
- Low must be **playable and unmistakably Everward**, not merely able to boot. Capture on the actual machine before claiming it playable.
- Art packs, shader quality, particles, lighting, reflections, texture memory, volumetrics, and post effects scale; core controls, scan data, targeting, repairs, progression and history do not disappear.
- Expensive high-tier fidelity must be tested on appropriate hardware when available; until then mark that evidence pending.
- Prefer optimization of scene complexity, scalability and authored detail over deleting the graphical ambition or silently downgrading the art direction.

## Agent task-selection and reporting contract

For every relevant Phase-2+ gameplay slice:

1. Read this document alongside current phase status/roadmap before choosing presentation work.
2. State the **player-facing experience improvement** expected, not only the code file or test count.
3. Link new visual/audio components to an authoritative mechanic or explicitly label them environmental presentation.
4. Include the cheapest appropriate verifiable scenario and quality-preset considerations.
5. Capture before/after evidence if available; avoid claiming results from screenshots generated separately from the running game.
6. Record status honestly: **planned / implemented / verified in Unreal / artistically accepted** are distinct states.
7. Keep the existing priority rule: first repair broken/pending work; prefer the earliest incomplete gameplay slice; introduce tightly bounded experiential improvements whenever they directly strengthen that slice or provide a safe independent parallel lane.
8. Update this document, phase plan or acceptance ledger when verified evidence changes the production approach; do not invent new visual work merely to avoid a blocked gameplay gate.
9. Protect the owner's control of the laptop and unattended-work scheduling agreements; do not interrupt foreground play for capture jobs.

## Relationship to late phases

Phase 22 (Visual Production), Phase 23 (Audio Production), and Phase 24 (UX/Accessibility) remain the later **comprehensive final maturation** phases. They are not their respective systems' first appearance. From Phase 2 forward, visual, animation, audio and UX production must grow with actual gameplay, leading to a polished integrated vertical slice well before those late phases.

**Progress means every accepted build is more playable, visually convincing, understandable, and coherent than the previous accepted build — not merely more code passing tests.**
