# Procedural World Generation Feasibility — EVERWARD-PCG-01

**Status:** Roadmap proposal / research and isolated prototypes **not implemented, not Unreal-verified, not accepted**.
**Origin:** Owner request, 2026-11-09, following an independent procedural-game-engine development post: https://www.reddit.com/r/gameenginedevs/s/U3jfHERmRm
**Authority:** `ROADMAP.md`, `CURRENT_EXECUTION_PLAN.md`, `AGENTS.md`, `WORLD_BUILDING_CRAFT_PLAYBOOK.md`, `CONTINUOUS_VISUAL_QUALITY.md`, `SAVE_FORMAT.md`, `PERFORMANCE_BUDGETS.md`, and `integrations/UNREAL_58_AI_CAPABILITIES_PILOT.md`.

## Purpose

Learn and prove techniques for a virtually inexhaustible, **visually distinctive and physically meaningful** Everward universe without hand-authoring every asteroid, moon, star system or resource deposit. A compelling environment must reward looking, scanning, moving, mining, building, and discovery. Procedural generation is a means of producing *gameplay worlds*, not just dramatic screenshots.

This packet is **additive**. Unreal Engine 5.8 remains the accepted rendering/gameplay integration engine; C++20 simulation remains the authority for physics, resources, entity identity, damage, extraction and saves. A third-party hobby engine is a research reference **only**. Do not assume the Reddit post provides downloadable code, permission to reuse source/assets, reliable technical documentation, a production-ready editor, or proven integration methods. Confirm code availability and licensing before evaluating or reusing anything.

## Scope and sequencing (non-displacement)

1. **P0 stays P0:** resolve the earliest incomplete, real-Unreal damaged awakening -> scan -> target/tool contact -> mine -> stored material -> Fix_It -> restored capability -> repeatable objective journey. Any procedural work must improve that same canonical scene, not create a disconnected demo.
2. **Reconcile existing work first:** the UE 5.8 Unreal MCP + PCG pilot already proposes a reversible fixed-seed mineral-deposit/rock/debris patch on an isolated worktree. Inspect current issue/PR/test state and reuse its verified results before scheduling new overlap. Its editor safety, sandbox, read-only preflight, human ownership, costs and licensing boundaries carry forward.
3. **A — deterministic mineable asteroid:** construct one irregular, genuinely 3D asteroid (not merely a 2D heightfield or decorated sphere), with optional fractures/cavities only if collision/scale testing supports them. Material and resource features derive from simulation-owned data; visible geology, the scan target, contact surface, mining reach, collected mass and altered geometry must agree. Preserve canonical Prime A and the actual mining/repair gameplay seam.
4. **B — deterministic small icy-moon region:** generate a bounded local test area with craters, ridges, ice fractures, rock/debris distribution, mineral patches and coherent material variation. Atmospheric/astronomical context and composition must follow world facts; an icy moon and giant planet are candidates for the starter scene, **not fixed lore**. Confirm readable navigation, reachability, mining, surface collision, and plausible scale.
5. **C — distance-sensitive environmental detail:** compare nearby full-detail geometry/collision with far-field impostors, instance density, virtualized meshes, hierarchical PCG, spatial streaming and GPU-driven techniques *where supported and measured*. Test approach/retreat transitions, no visible popping beyond accepted thresholds, no target-ID/resource discontinuity, and no simulation changes caused by visual LOD.
6. **Later Phase 11 scale-up, dependent on acceptance:** deterministic planetary surfaces, orbital regions, asteroid fields, rings, moons, star-system placement and visually differentiated astronomical phenomena, generated lazily as needed and persisted after observation or modification. These are **not** deliverables of the initial Phase-2 experiment.

Do not add a major runtime dependency, non-Unreal engine, paid service, external API, autonomous editor permissions, production plugin, or continuously running worker merely to explore this idea. No new owner playtesting burden: automated captures and verification first; owner review only at meaningful subjective milestones. Never launch paid Aura tests.

## Proposed generation and presentation architecture

```text
(universe seed, coordinates, algorithm version)
             + established physical world state
                         |
         Engine-independent authoritative C++ simulation
       stars/orbits/composition/mineral stocks/identities
                         |
              approved read-only projection
                         |
        procedural geometry + material/placement recipe
                         |
               Unreal rendering / PCG / LOD
                         |
             visuals, collision proxies, HUD
```

- Seed + spatial coordinate + algorithm version produce stable baseline geometry/region identity; saved observed/modified deltas are reapplied on reload. Do not overwrite the existing `generated_region.hpp` or `SAVE_FORMAT.md` contracts with a second incompatible generator.
- Simulation/state owns mining quantities, probe interaction/contact and progress. Presentation may derive visuals, collision proxies and visual damage, but cannot invent or destroy conserved material or silently replace authoritative reach checks.
- Support multiple visual scales: star-system composition -> orbital context -> local cinematic terrain -> nearby tools and mineral surfaces. Ensure procedural objects are visually readable, navigable and useful, not arbitrary noise.
- Preserve the same resource locations, actionable collision logic, target identifiers, persistence, simulation outcomes and accessibility across Emergency/Minimum, Low, and higher settings. Visual density, material/shader and decorative complexity may vary by preset.
- Python, if used, is **offline tooling** for seed validation, batch generation, measurement or research; do not add Python as a shipping runtime language without a new architecture/packaging gate. C++ compute/shader or GPU generation methods require actual hardware measurements and driver compatibility.
- Separate future **visual block-programming** research for player-authored drone/probe rules; link to autonomy/doctrine UI after the current playable loop is proven. A Scratch-style editor is not a world-generation dependency.

## Validation and acceptance (evidence, not screenshot claims)

For each isolated experiment record: exact Git SHA, Unreal build and map, seed, generator version, source asset/license inventory, graphical preset, GPU/CPU/RAM, before/after capture and reproduction instructions.

1. **Determinism:** same seed + coordinate + version + state reproduce stable IDs, object placement and critical geometry/material/resource outputs; a different seed changes appropriate features without breaking invariants.
2. **Mining/physics agreement:** live Unreal probe scanner/target selection, mining-tool tip, reachable surface, collisions, material transfers and excavated/damaged visual states match authoritative state. No false-contact mining or inaccessible resource spawned by decorative placement.
3. **Persistence:** save/load reconstructs the baseline plus observed/extracted/modified-region deltas; no respawning extracted material, identity loss or save incompatible with documented versioning.
4. **Quality and gameplay:** capture normal gameplay, HUD-hidden composition and motion at key distances; check silhouette, geological coherence, scale, landmarks, visible cause/effect, terrain navigation, and lack of clipping/floating artefacts. Human artistic acceptance remains distinct from automated tests.
5. **Performance and scalable detail:** measure wall-clock generation time, CPU/GPU frame time, peak RAM/VRAM/shared memory, draw calls, spawn count, load/streaming latency and LOD transitions on **Azathoth's Emergency/Low** path plus higher-end hardware when available. Compare against `PERFORMANCE_BUDGETS.md` and existing accepted scene; choose explicit measured thresholds before claiming PASS. Do not infer high-end acceptance from low-end hardware or vice versa.
6. **Isolation:** disposable test map and reversible feature branch/worktree; no shipping dependency introduced, no changes to Iron God controls, production repo execution lanes, unrelated projects, or paid API/tool configuration.

Use status labels `PROPOSED`, `IMPLEMENTED`, `VERIFIED_IN_UNREAL`, `ARTISTICALLY_ACCEPTED`, or `BLOCKED` as supported by evidence. Passing source tests is insufficient for `VERIFIED_IN_UNREAL`.

## Longer-term scope after prototype validation

- **Asteroids and mining fields:** 3D shape families, fractures, cavities, geological layering, ore/mineral concentrations and actual mining state.
- **Planetary/mmoon environment families:** cratered rock, ice, fractured surfaces, volcanic features, atmospheric/thermal variation and rare formations with science-informed material rules.
- **System-scale environments:** stars, moons, planets, belts, rings, eccentric orbits and anomalies where the visual presentation follows existing astronomy mechanics.
- **Unique discovery:** seed-consistent landmarks and material signatures; progressively improved scanning reveals information, rather than a decorative random landscape immediately revealing all facts.
- **Streaming and persistence:** lazy region generation, spatial/temporal level of detail, asset pooling and compact save deltas for very long campaigns.

**Decision gate:** retain or promote a technique only when it measurably improves the actual Everward experience while preserving determinism, gameplay truth, the approved Unreal architecture, low-spec accessibility, licensing and unattended verification. Reject attractive but unmeasurable or mechanically disconnected demos.

## Research references and follow-up

- Independent developer example: https://www.reddit.com/r/gameenginedevs/s/U3jfHERmRm — idea source, **not** verified reusable code.
- Epic official procedural content generation overview: https://dev.epicgames.com/documentation/unreal-engine/procedural-content-generation-overview
- Epic geometry scripting overview: https://dev.epicgames.com/documentation/en-us/unreal-engine/introduction-to-geometry-scripting-in-unreal-engine
- Local Unreal PCG/Unreal MCP plan: `integrations/UNREAL_58_AI_CAPABILITIES_PILOT.md`.
- Existing craft and quality guidance: `WORLD_BUILDING_CRAFT_PLAYBOOK.md`, `CONTINUOUS_VISUAL_QUALITY.md`.

Do not substitute external claims, AI-generated visuals, or an editor-only sandbox pass for verified in-game Everward behavior.
