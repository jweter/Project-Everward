# Everward Engine Direction

## Accepted production direction — November 27, 2026

**Godot is Everward's active production development engine**, as approved by the owner and recorded in [ADR-0014](DECISION_LOG.md). The prior Unreal-first decision (ADR-0001) is **superseded**, not erased. Unreal feature work is frozen; keep `unreal/` as historical/reference material until the G5 retirement gate is met. Do not schedule or implement new production Unreal features by default.

This is a product and engineering direction, not permission to reduce visual ambition or change the game's identity. The player is the probe, embodied in a scientifically credible, visually compelling physical 3D universe.

## Why the direction changed

Unreal native verification and iteration were constrained by RAM and toolchain demands on the available non-upgradable development laptop. The owner-approved Godot feasibility path has demonstrated a playable world spike on the laptop's Intel Iris Xe (reported 44–60 FPS in the scripted route, with a significant transient drilling dip), native C++ simulation binding, and deterministic headless test routes. These are meaningful iteration/verification advantages, **not** proof of final art quality, shipping performance, or every Phase 2 gameplay gate.

The exact migration status and limits live in [Godot Path Plan](GODOT_PATH_PLAN.md) and the `tools/godot/` READMEs. A dated report must not override current code, PRs, or exact-commit verification.

## Visual standard — unchanged

Everward must still pursue:

- physically convincing large-scale space environments;
- high-quality real-time lighting, PBR materials, atmospheres, volumetrics, particles and shadows;
- detailed probes, mining systems, infrastructure, and eventually megastructures;
- cinematic composition without sacrificing player controls or gameplay legibility;
- physical embodiment near asteroids, moons, planets, rings, stars and machinery;
- transitions across local, system and strategic views;
- HUD-off/photo-quality scenes that reward exploration.

The target remains **cinematic scientific realism**. Godot is not a rationale for defaulting to a 2D map game, intentionally crude low-poly identity, or a feature-incomplete low-spec build. Achieve the target through appropriate scene craft, shaders, assets, procedural tools, and measured graphics tiers, not by assuming parity with Unreal's renderer.

## Responsibilities and hard boundaries

- **`src/simulation/`**: authoritative deterministic mechanics, timing, physical state, inventory, repairs, save rules, and outcomes; remains testable without an engine.
- **Godot/GDExtension**: validated player commands and simulation snapshots/events; Godot scenes and UI render state but do not create independent mechanical truth.
- **Native + Godot headless tests**: objective correctness and parity on the exact source commit.
- **Godot in-engine captures and runs**: practical performance, interaction legibility and presentation evidence.
- **Human visual/game-feel review**: only genuinely subjective acceptance, separately identified from machine-verifiable PASS.

Do not imply that a green GitHub check, a simulation unit test, an illustrative screenshot, or a high frame-rate scene by itself proves the full game is fun or finished.

## Current migration stages

The [Godot Path Plan](GODOT_PATH_PLAN.md) is authoritative for G0–G5:

- **G0**: feasibility world spike, merged (#433).
- **G1**: simulation GDExtension and native/Godot parity foundation, merged (#437).
- **G2**: simulation-driven Godot movement/contact experiments with acknowledged approximation limits.
- **G3**: the integrated scan/mining/Fix_It first loop is progressing (#443, #444); all legacy #378 equivalence gates are not yet accepted.
- **G4**: complete visual, gameplay capture, performance-tier and owner-art acceptance evidence remains pending.
- **G5**: archive the historical Unreal project only after required parity/acceptance evidence and a normal reviewed PR.

The historical Phase 1 Unreal exit gate does not have to be relitigated to continue Phase 2 in Godot. Earlier Unreal-specific runtime tests do not become `GODOT_VERIFIED` automatically.

## Development rule

All new Phase 2 production gameplay, HUD, rendering, audio, asset integration, interaction, autonomous QA, and runtime tooling should target **Godot**, subject to the existing review/CI, worker, IP, payment and owner-approval restrictions. Keep core simulation development engine-independent.

Any future switch away from Godot requires another explicit accepted ADR. Historical Unreal reports are evidence, not current development instructions.
