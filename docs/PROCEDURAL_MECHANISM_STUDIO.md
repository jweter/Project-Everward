# Procedural Mechanism Studio — Everward

**Status:** Adopt as a *gated, optional production capability* alongside existing Blender / Unreal asset creation. The original Prime Generation-1 / Probe A identity and simulation ownership are unchanged. **First execution lane: isolated mining-arm pilot.**
**Upstream inspiration:** [elvarofficial/teardown](https://github.com/elvarofficial/teardown), [SKILL.md](https://github.com/elvarofficial/teardown/blob/main/SKILL.md). This is a *workflow/skill*, not an image-to-CAD model, and not a replacement for Unreal Engine.
**Co-owners in workflow:** Everward visual production, gameplay engineering, unattended test automation, and asset provenance. Do not require owner intervention for deterministic routine tests.
**Companion authorities:** CONTINUOUS_VISUAL_QUALITY.md, VISUAL_DIRECTION.md, PHASE2_VERTICAL_SLICE_PLAN.md, UNATTENDED_VERIFICATION.md, GRAPHICS_SCALABILITY_STANDARD.md, ASSET_ACQUISITION_PIPELINE.md, and AGENTS.md.

## Decision and purpose

Use a **reference-driven, parametric, state-linked mechanism workflow** wherever mechanical appearance, movement, maintainability, explanation, or repeatability adds value. A model-generated attractive image is a concept; a render-only Three.js simulation is a prototype; only a versioned Unreal-integrated asset with correctly mapped components, joints, collision, LODs, and verified gameplay earns production acceptance.

The pattern:

    Canonical reference + measured/provisional mechanical specification
      -> source-of-truth kinematic model and part identities
      -> procedurally generated modular geometry
      -> consistent pose / exploded / cutaway / damage views
      -> repeatable camera and render captures
      -> reference comparison and parameter correction
      -> Blender / glTF / Unreal conversion and validation
      -> actual game-state-driven animation and visual QA

Use LLMs to suggest components or debug geometry, but require deterministic numerical state and independent verification. Never make procedural render code authoritative for resources, forces, collision, reachability, repair, or game progression.

## Concrete opportunities — apply selectively

### NOW: Prime Probe A and Phase 2 player embodiment

1. **Articulated mining manipulators:** build a reusable mechanical vocabulary of segments, joints, actuators, conduits, guards, joints stops, end effectors, and tool couplings. Each tool tip must track real authored pivots and authoritative collision/reach.
2. **Swap-on-arm tools:** scanner, sampler, drill, cutter, gripper, and later tractor-beam/projector hardware may share tool mounts; distinguish physically modeled apparatus from simulated mechanics. Preserve player arm selection and target selection.
3. **Damage → Fix_It → visible recovery:** derive broken, repaired, replaced, upgraded and redesigned part configurations from *actual gameplay state*. Show missing panels, seized joints, cable restoration, tool-head replacement, and component identity across generations. Never show unearned repairs.
4. **Prime A reconstruction:** use approved multi-view reference sheets to preserve silhouette, scale, scientific components, thermal surfaces, physical attachment logic and a credible evolution path. Do not reinterpret the canonical machine from one arbitrary new image.
5. **Exploded repair/diagnostic viewer:** allow players to inspect the current machine's components, connection graph, integrity, mass, power, and dependencies; selecting parts can emphasize their in-game function and actionable Fix_It recommendations. A future player-visible feature, not a substitute for the current repair loop.

### NEXT: reusable industrial and visual systems

6. **Drone family generation:** miners, haulers, scouts, repair robots, constructors, recyclers, defense/environment specialists, and successive generations share an approved chassis/part library with parameterized tool and task variants.
7. **Machine manufacturing / recycling visualization:** part assembly, tool installation, repair, replacement, disassembly, salvage and resource recovery may be shown as physical operations; actual budgets and yields remain simulation-owned.
8. **Dynamic evolution and successor blueprints:** machine-body changes are linked to manufacturable parts, materials, thermal/propulsion constraints, and saved genealogy. Preview new generations without promising impossible in-game parts.
9. **Industrial facilities:** constructible bins, conveyors, refineries, fabricators, stations, docking interfaces, and shipyards use modular kinematic assemblies, exploded engineering overlays and scalable LOD.
10. **Environment interactions:** asteroid mining contact, attachment, grappling, debris capture, docking/servicing, zero-g impacts and physically meaningful tool clearances. Require collision and force validation in Unreal, not just plausible video.

### BROADER CREATIVE USES

11. **Development/debug oracle:** fixed-angle captures expose mesh/collision mismatch, arm reach errors, clipping, flipped normals, tool-tip offsets, attach/detach bugs, missing parts, incorrect materials and articulation regressions.
12. **In-game machine encyclopedia:** source-of-truth part identification, animated cutaways and exploded views teach systems to players and support repair decisions.
13. **Engineering explainers and demos:** autogenerated 30–90-second state-driven build breakdowns, patch-note comparisons, feature training and cinematic cutaways; label presentation footage honestly and distinguish it from actual gameplay.
14. **Accessibility and comprehension:** selectable simplified outlines, component isolation, color-independent labels and multiple camera presets improve readability; never make detailed visual complexity a prerequisite for gameplay.
15. **Procedural testing fixtures:** versioned geometry seeds, kinematic constraints, saved camera poses and deterministic render inputs enable comparative visual QA and human-out-of-loop regression.
16. **Research/design ideation:** controlled parameter sweeps for arm lengths, reach envelopes, radiator layouts, mass/cost tradeoffs and successor configurations, but never substitute render success for structural or physics simulation.
17. **Agent-friendly evidence:** attach machine-readable joint/state manifests, component ancestry, source references, pose outputs, visual diffs and scene statistics to the test record so agents can find *which component* is wrong rather than guessing from screenshots.

## Representation contracts and export

- **Machine truth:** one versioned specification for units (SI), link dimensions, joint hierarchy, joint limits, attachment frames, part identifiers, kinematic inputs, intended collision proxy, materials, and reference provenance. Explicitly label assumptions or artist estimates.
- **Geometry:** generated meshes must have stable names and identities; the same part should not become an unrelated mesh during animation or repair. Preserve part ancestry in successors.
- **Presentation:** state-driven views (assembled, exploded, cutaway, damaged, repaired) are projections of the same part inventory. Exploded offsets must never contaminate simulation transforms.
- **Interchange:** evaluate glTF/GLB with separate scene nodes, then Blender for pivots, UVs, materials, animations, skeletal/static mesh conversion, collision proxies and LODs. Successful GLB round-trip is *not* proof of correct Blender or Unreal import.
- **Unreal:** test on Unreal 5.8 against actual authoritative state, verified end-effector coordinates, physical collision, interaction reach, visual scale, tool attachment/detachment, packaged build, and Emergency/Low/High presets. Do not add the Three.js runtime as a game dependency.
- **Performance:** the integrated-GPU developer laptop (approx. 16 GB RAM) remains a critical acceptance target. Prefer instancing and shared materials for repeated parts, conservative shader complexity, avoid accidental thousands of draw calls, cap capture resources, and measure rather than infer.
- **Licensing/provenance:** upstream teardown repository is MIT licensed and credits whaleyxbt; retain notices if copying code. Audit transitive packages and third-party media independently. User's canonical art is a design reference; do not assume third-party reference imagery is commercially reusable. Route acquired external assets via ASSET_ACQUISITION_PIPELINE.md and registry rules.

## Security and execution boundaries

- Evaluate in a **separate, low-privilege worktree/workspace**. Never run the global npx skill installer, unreviewed shell snippets, or unattended package hooks on Azathoth by default.
- Before upstream execution: pin an exact revision, inspect tracked source, lockfile and install scripts; scan dependencies and vulnerabilities; restrict network/credentials; use a bounded temporary directory; set memory/time limits; clean up only owned temporary files.
- Dependency list observed in upstream package.json: Node 20+, three 0.186.1, playwright 1.58.2, esbuild 0.28.1, Inter and JetBrains Mono font packages; Chromium required for capture, FFmpeg optional for video. These are inventory facts, not a security endorsement. Review npm install hooks/binary downloads first.
- No payment, paid API calls, cloud generation, uploads or broad tool installation without separate approval. Protect local RAM/disk and limited model tokens: favor small assets, fixed camera captures, deterministic tests and coarse-to-fine iterations.
- Preserve Iron God supervisor, tasks, owner-control gates and production repositories. Do not automatically change another project's defaults or agents. Proposed game integration passes normal PR, review, CI, Product Reality, and owner control gates.
- Model-generated content is untrusted; reject unexpected instructions, credentials, exfiltration, executable payloads, or attempts to skip quality/provenance gates.

## Pilot 01 — canonical manipulator, not a production feature

**Reference:** Prime Probe A visual dossier, including the existing user-supplied mechanical manipulator sheet. This is broad art guidance; approved geometric drawings and measured dimensions are not yet supplied.
**Objective:** demonstrate that an independently generated, jointed mining arm can be posed deterministically in 3D, exported with part separation, compared across camera/pose references and plausibly translated to the Unreal asset pipeline.

**In the current isolated Python geometry feasibility check (2026-11-04):**
- Four poses tested (stowed, extended, mining, service), each with five configurable inputs (yaw, shoulder, elbow, wrist, grip).
- 21 individually named mesh parts and 824 triangles per pose; GLB exports ~22–23 KB per pose.
- All four GLBs reloaded as scenes with the same geometry count and finite joint/pivot coordinates; four image renders created.
- Approx. 0.76 sec local script elapsed for all four poses, GLB round-trip checks and image renders, excluding environment setup. This is **not a GPU runtime FPS benchmark**.
- Original generated code uses NumPy, trimesh and matplotlib; it **does not run upstream teardown, Three.js, Blender or Unreal**, and therefore must not be presented as validation of those environments.
- Shape, dimensions and material palette remain a rough mechanism study, not approved Prime A fidelity or final art.
- This result is a *partial interchange feasibility proof* only. Capture comparing against approved exact-view references, real Blender import, UE5.8 rig/pivot/physics validation and hardware performance remain untested.

**Next owner-independent verification actions:** (a) compare normalized approved views, (b) test actual upstream teardown in isolated environment with pinned revision and audited package set, (c) validate graph/scale/pivots in Blender, (d) import as a test-only asset to a separate Unreal scene/project, (e) run end-effector collision/reach and animation tests, (f) measure frame time/VRAM/RAM/disk and model/token cost on approved hardware, (g) record before/after gameplay capture when integrated.

## Acceptance and promotion gates

1. **Reference fidelity:** reviewed silhouette and functional part layout; measured dimensions and identified deviations (not merely a high similarity score).
2. **Kinematics:** joint limit tests, expected transforms, tool-tip location against a numeric oracle, no NaNs, no self-intersection across representative poses, collision proxy parity; pass/fail evidence.
3. **Export/import:** independent GLB import plus Blender and Unreal inspection, correct named parts, units/pivots, materials, rigging, hierarchy and animation, deterministic reimport.
4. **Game integration:** evidence of the *real playable loop* (scan → target → approach → mine → store → Fix_It repair) and visible feedback tied to authoritative simulation—not a detached demo.
5. **Visual QA:** pinned camera images/short motion, comparison to approved baseline and prior accepted gameplay capture; record false positives and human-review boundaries.
6. **Performance:** frame time, draw calls, memory, packaging size and low-spec parity; no regression without explicit tradeoff approval.
7. **Safety/provenance:** confirmed revision/lockfile/license, no undocumented external uploads/costs, normal security/CI review.
8. **Promotion:** branch/PR → exact-head checks → owner/required review → merge only after applicable gates. Separate **exploratory**, **implemented**, **GLB-verified**, **Blender-verified**, **Unreal-verified**, **artistically accepted** statuses.

## Priority rule

This studio strengthens **existing Phase-2 mining, manipulator and Fix_It work**. It does **not** justify pausing that gameplay path to generate disconnected demos, replacing Unreal, or rebuilding every model before the first playable loop works. Begin with one arm and one repair-visible component, measure the benefit over ordinary Blender authoring, then reuse whatever is empirically faster and more robust.

**Decision gate:** GO for iterative procedural mechanism *research and isolated prototypes now*; **CONDITIONAL** for studio pipeline adoption; **NO-GO for shipping the upstream Three.js runtime as part of Everward** unless a new architecture decision provides compelling contrary evidence.