# Procedural Mechanical Modeling & Teardown — Everward

**Status:** Authorized development methodology and isolated prototype track, effective during Phase 2. The first pilot is **not** an accepted production mesh, functioning Unreal gameplay feature, or a replacement for the Unreal Engine 5.8 project.

**Companion authority:** [CONTINUOUS_VISUAL_QUALITY.md](CONTINUOUS_VISUAL_QUALITY.md), [CURRENT_EXECUTION_PLAN.md](CURRENT_EXECUTION_PLAN.md), [PHASE2_VERTICAL_SLICE_PLAN.md](PHASE2_VERTICAL_SLICE_PLAN.md), [ASSET_ACQUISITION_PIPELINE.md](ASSET_ACQUISITION_PIPELINE.md), [UNATTENDED_VERIFICATION.md](UNATTENDED_VERIFICATION.md), canonical Prime Gen-1 reference assets and manifests.

## Why this exists

Reference-driven procedural geometry, joint-driven kinematics, interactive cutaways, exploded views, staged assembly, and screenshot-feedback correction can accelerate the process of creating convincing mechanical assets. The inspiration is [elvarofficial/teardown](https://github.com/elvarofficial/teardown), an MIT-licensed Claude Code skill built around Three.js and scripted machine simulators, including robotic-creature demonstrations. **Use the transferable method broadly**, not the Three.js browser runtime as a replacement for the game's authoritative simulation or Unreal renderer.

Pin any isolated upstream checkout to commit `a91262ac1109051e014662d42ecb0b7bc1f113b1` before reviewing dependencies, scripts, network behavior and licensing. The upstream package uses Three.js, esbuild, Playwright, fonts, and optionally Chromium/ffmpeg; installing and capturing can consume laptop RAM, disk and GPU time. Upstream's MIT LICENSE names elvarofficial and whaleyxbt. Preserve all applicable notices if reusing substantial source. Do not assume unrelated samples, sound recordings or generated images have the same license.

The Prime Gen-1 Probe A — Scientific Explorer remains the canonical starting body. A generated output is **a design candidate** until it passes provenance, multi-view reference, mechanics, Unreal, low-spec, and game-feel checks.

## The new production chain

1. **Select player-facing requirement.** Start with the earliest incomplete gameplay slice. First target: a credible, selectable, articulated mining manipulator with accurate reach and collision. Identify the relevant controls, simulation-owned data and visible payoff.
2. **Establish reference and dimensions.** Use approved Prime Gen-1 views and actual measurements where available. Label assumed dimensions as preliminary. Define assembly IDs, body axes, joint limits, attachment sockets, mounting hardware, material families and intended damage states.
3. **Generate parametric mechanism.** Separate pure mechanism equations/kinematics from Three.js geometry, lighting and camera. Geometry, animation and readouts derive from one parameter contract. Measure joint/end-effector positions against the actual control/physics adapter; never invent what the runtime has proven.
4. **Build an isolated interactive engineering studio.** Orbit, isolate, disassemble, section, repair and reassemble the model. Capture locked side/top/front/three-quarter views, tool-tip trajectories and performance metadata. The studio is a development aid and can be distributed as offline explanatory media; it is not a release feature by itself.
5. **Render → compare → correct.** Compare reference silhouette, proportion, part relationships, hinges, material response, collision and contact. Fix numerical geometry, not just the camera angle. The automated reviewer can propose fixes; visual critiques cannot certify artistic or gameplay quality.
6. **Translate to production assets.** Investigate a Blender scripting/glTF/FBX handoff or Unreal Geometry Scripting implementation. Maintain stable pivots, sockets, names, sensible triangles/material slots, LODs and collision primitives; confirm the format actually transfers these. Do not call Three.js output “Unreal-ready” before this succeeds.
7. **Admit through provenance and Product Reality.** Apply `ASSET_ACQUISITION_PIPELINE.md` to externally sourced asset components and record generation/source provenance for first-party procedural geometry. Integrate through the current Unreal adapter; keep simulation truth, damage, repair resources, player input and gameplay collision authoritative. Capture unedited real gameplay, frame/memory metrics and cross-tier behavior.
8. **Learn without endless busywork.** Record known-good camera presets, component schemas, failures, frame comparisons and accepted parts. Reuse modular pieces and rigging standards across generations. Keep AI inference for asset creation/review, not a live LLM per machine.

## Strategic uses — exploit the method creatively

| Area | What to build | Truth/acceptance boundary |
|---|---|---|
| **Mining arm — FIRST** | Parametric upper/forearm links, shoulder/elbow/wrist axes, tool head, drill bits, manipulators, visible contact points, arm swap previews | True reach and collision in Unreal; exact tip in gameplay units |
| **Prime Gen-1 body** | Modular scientific probe: central tube, radiators, antennae, thermal/control pods, sensor apertures, instruments and thrusters | Preserve canonical silhouette and properly attached systems |
| **Fix_It** | Damage x-ray, isolate failed subsystem, deploy service access, show repair/replace, reassemble; mechanical “this should not work” lore | Resource/power/time and restored function remain simulation-owned |
| **Repair → Replacement → Evolution** | Variant part catalog, constraints, manufacturing preview, generational successor comparisons, visual genealogy | Visually meaningful engineering tradeoffs, not magic morphing |
| **Worker drones** | Shared chassis/hardpoints and specialized mining, hauling, scanning, repair, defense, construction and recycling kits | Functional equipment and agent roles; no decorative substitutions |
| **Fabrication, recycling and storage** | Assembly line choreography, cutting/extraction, sorting, salvage, bins, conveyors, refinement and drone assembly | Material accounting, actual production throughput and logistics match the simulation |
| **Tractor beam, grapples, José docking** | Mechanism/force-path diagrams, grapple anchors, thruster arrays, joint constraints and docking alignment guides | Real physics/navigation still owns forces and movement |
| **Scientific instruments** | Sensor cutaways, scanning beam alignment, sample handling and spectroscopy interfaces | Unlock only knowledge actually discovered by the probe |
| **Player engineering HUD** | Exploded subsystem map; toggle visibility, choose arms/tools; inspect failure reason and service options | Works at gameplay scale and Emergency/Low settings; accessibility considered |
| **First awakening** | One signature visual sequence: damaged machinery activates, mine material, Fix_It makes visible repair | Must be an unedited integrated Unreal gameplay capture |
| **Autonomous design pipeline** | Procedural kit generator, camera-aware art review, mesh regression/collision checks, 3D assembly diff, artifact provenance | CI evidence with exact-head verification, no green-by-illustration |
| **Tutorials and outreach** | Interactive schematics, technical manuals, in-game maintenance lessons, brief reveal clips and educational trailers | Label developer renders and illustrative simulations honestly |
| **Long-term scalability** | Parameterized equipment archetypes and aggregation/LOD for distant machine populations | Do not render all drones at full per-part fidelity at all distances |

### Additional creative design possibilities, without promoting them ahead of gameplay

- **Scanner-controlled cutaways:** a player sees only systems its instruments can resolve; improved sensing reveals hidden mechanisms or design flaws.
- **Mechanical archaeological record:** the Gen-1 probe visibly exposes the neglected engineer's shortcuts and the intern's unauthorized Fix_It repairs, with each physical repair teaching a fragment of the origin story.
- **Successor “design laboratory”:** compare a generation's mass, power, manipulators, heat, clearance and serviceability alongside a working assembly preview.
- **Manufacturing camera:** a probe can watch its first child assembled part by part; visuals consume actual world inventory and build time.
- **Recycling forensic mode:** recover components and inspect failure history or provenance rather than instantly converting a unit into generic materials.
- **Drone/arm maker toolkit:** after the basic game loop exists, an engineering UI can recombine approved mechanical kits under real constraints and preview motion/clearance before fabrication.
- **Automated asset regression oracle:** test articulation poses, camera silhouette, tool-tip offset, collision envelopes, clipping, materials and quality-tier frame times before accepting visual updates.

## Proof-of-concept delivered with this document

`tools/procedural_mechanics/prime-arm/` includes:
- `model.mjs`: parameterized pure, deterministic two-link mechanism and tool-tip geometry (no Three.js dependency).
- `model.test.mjs`: deterministic mathematics, physical bounds and repeatable timeline tests using Node's built-in test runner.
- `main.js`: isolated Three.js mechanism studio using upstream teardown's runtime contract; animated joint poses, an exploded-view interval and a simple orbiting camera. **This is a provisional engineering blockout, not the canonical Prime model.**

Test the pure model with:
```sh
node --test tools/procedural_mechanics/prime-arm/model.test.mjs
node --check tools/procedural_mechanics/prime-arm/main.js
```

To build a local self-contained HTML page **after upstream source, dependencies and resource budget have been approved**, on an isolated working copy:
```sh
# Review and explicitly install the pinned teardown checkout inside .tools/ first.
node .tools/teardown/sims/build.mjs ./tools/procedural_mechanics/prime-arm
# Build output is in teardown-out/dist/prime-arm.html or TEARDOWN_OUT/dist/.
```

No automatic cloning, `npm install`, browser download, network calls or process launches occur in the tracked pilot source. Do not make an unreviewed upstream installation part of normal Everward startup or CI. Build artifacts belong in ignored local output and are not automatically licensed, imported, or promoted.

### Pilot gates

1. Security and licenses: pinned upstream, lockfile/dependency review, local-only default, attribution and provenance accepted. No secrets or live owner controls accessed.
2. Mechanism: math tests pass at the exact PR head; stable named joints, consistent tip positions, state-independent reproducible rendering and valid numeric dimensions.
3. Visual: locally built offline demo opens and captures on the laptop's integrated graphics without disrupting active work; truthful before/after and GPU/performance data collected.
4. Asset bridge: isolated Blender/Unreal transfer demonstrably preserves part hierarchy, transforms, materials and usable collision/socket layout.
5. Product Reality: actual existing Everward Unreal mining arm uses an accepted assembly, maintains authoritative targeting and mining contact, and functions in Low/Emergency as well as higher presets.
6. Quality/cost: compare model quality and total labor/Claude tokens/CPU/memory against the pre-existing manual/Blender approach. If cost is higher with no quality gain, keep as a visualization-only aid.

**Status vocabulary:** `documented` → `code prototype` → `tests verified` → `teardown capture verified` → `Unreal bridge verified` → `Product Reality verified` → `artistically accepted`. No gate is implied by a later aspiration.

## Governance and prioritization

This is a reversible **parallel-safe visual tooling lane**. Fixes to failing CI, outstanding PRs, mining contact, Fix_It correctness, Product Reality and owner-controlled execution take precedence. Never update the installed Iron God release, owner-control switch, scheduled jobs, desktop resources or existing production branches as a side effect of this work. Do not auto-run memory-intensive browser capture while the laptop is occupied.

Agents proposing new procedural assets should link this document, specify **one gameplay benefit and one verification scenario**, and distinguish isolated kit output from a shipped game asset. Owner subjective review is reserved for meaningful visual milestones; automated engineers own regression, build, conversion and diagnostics.
