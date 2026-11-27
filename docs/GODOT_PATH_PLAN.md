# Godot path plan

**Status (2026-11-27):** **Godot is the owner-approved active production development path** (ADR-0014). New Unreal feature work is frozen; the existing Unreal source is retained as historical/reference material until the G5 archival gate. Engine migration does not close any #378 G01–G13 item without its own evidence. See `docs/ENGINE_DIRECTION.md` and `docs/DECISION_LOG.md`.

**Current implemented evidence:** world-feasibility spike #433; palette/looks and refined procedural trees #436/#439; simulation GDExtension/native parity #437; simulation-driven flight/contact #440/#442; scan → survey → approach → collision-aware arm path → mine #443; resource-funded Fix_It repair #444. These were merged. They do not prove the entire first playable, save/restore of every world state, final sound/visual quality, or packaged distribution.

**Why Godot:** Unreal native verification exceeded the feasible free-memory budget on the non-upgradable laptop. Godot's initial scripted world route was 44–60 FPS on its integrated GPU, with a transient dip to approximately 13 FPS during drilling (later tree work reported a higher route FPS and a still-significant drilling dip). C++ simulation + headless Godot now allow bounded automated behavioral verification. A successful headless run does not constitute owner-approved game feel or high-end visual acceptance.

**Invariant:** `src/simulation/` alone owns authoritative mechanical state. Godot presents and submits validated commands through the GDExtension; it must not fork simulation truth. Owner-approved mood/reference images remain outside this **public** repository; commit only appropriately licensed or derived assets and values under the existing provenance gate.

## What stays and what is replaced

| Stays (engine independent) | Replaced (Unreal only, about 10,600 lines) |
|---|---|
| `src/simulation`: 49 headers, 50 test files, about 23,000 lines, including the collision-free arm planner (#431) | `ProbeSimulationAdapter` (about 2,000 lines): the object that wraps the simulation for the engine |
| `tools/procedural_mechanics` (C++ oracle, GLB exporter) and `tools/godot/prime-rig-proof` | Bridges: mining, tractor field, target selection, surface descent (about 1,450 lines) |
| `assets/reference/**` and their manifest validator | Pawn and player controller, mining controls, autopilot, hover, descent (about 2,000 lines) |
| All C++ and Python tests that do not need Unreal | HUD (`EverwardHUD`, about 1,300 lines; its read models `ProbeHUDReadModel` and `PeripheralHUDLayout` are already plain C++ and are reused) |
| | Test environments and the Fix-It actor; the 12 native Unreal automation tests |

The paid Aura playtest tooling was Unreal-specific and owner-only. It remains historical, and no paid Godot equivalent or automated paid use is authorized.

## Mapping

- **Adapter** becomes a GDExtension class (godot-cpp, C++20) that owns the same simulation objects and ticks them at the fixed 16,667 us step from `_physics_process`. It exposes the same commands and read-only snapshots the adapter exposes today.
- **Bridges** are small C++ helpers moved into the extension.
- **Pawn, controller, controls** become Godot scripts on a flight scene. Joint control uses the simulation's own path planner, so the "arm stalls at the hull" defect fixed in #431/#432 cannot return.
- **HUD** becomes Godot `Control` scenes fed by the existing C++ read models.
- **Native tests** become headless Godot runs (a script runner) plus the existing C++ tests, which keep running unchanged on the laptop.

## Phases and the evidence each one must produce

| Stage | Present status | Required acceptance evidence |
|---|---|---|
| **G0 — Feasibility** | Implemented, #433 merged | Reproducible world spike, `--autotest` route, captures/metrics and documented laptop limits |
| **G1 — C++ binding** | Implemented, #437 merged | `EverwardSim` GDExtension, native-vs-Godot exact-source headless parity of canonical simulation and tested save/load behavior |
| **G2 — Flight/world** | Implemented for bounded pathways, #440/#442 merged | Sim-driven position/orientation, input and contact checks; the three ground-following proxy spheres remain a *terrain approximation* |
| **G3 — First loop** | In progress, #443/#444 merged | Damaged awakening → scan → survey → physically approach/align/mine → spend stored material and energy on Fix_It → visibly restore capability → repeatable objective, plus persisted world state and remaining #378 G01–G13 equivalent checks |
| **G4 — Integrated visual/game-feel** | Not accepted | Real Godot captures/flythrough; 30+ FPS target on the current laptop with measured frame-time spikes; readable HUD, credible machine/world art, feedback and bounded owner review; high-end desktop GPU results remain unverified |
| **G5 — Retire Unreal** | Not started | Required Godot parity and product acceptance, preserved historical evidence and a separate reviewed owner-authorized archival PR |

**Status meanings:** `GODOT_VERIFIED` means an automated headless Godot run on the **exact source commit**, with captured evidence. It is not a synonym for `UNREAL_VERIFIED`, human art/game-feel acceptance, full gameplay acceptance, or commercial readiness. Keep incomplete gate items open even if their subcomponents merge.

## Risks

- **Look.** Godot lacks Unreal's Nanite/Lumen pipeline; cinematic quality must be built through purposeful lighting, PBR materials, shaders, art, VFX and measured graphics tiers. G4 is still open; laptop evidence is not a substitute for a future desktop GPU assessment.
- **GDExtension build.** godot-cpp plus the existing MSVC toolchain must build C++20 headers cleanly; G1 exists to find out early.
- **Voxel Tools.** The spike uses a third-party prebuilt Godot with the voxel module compiled in. A shipping path likely wants the GDExtension build or a build from source; building Godot from source is memory-heavy on this laptop.
- **Destruction.** The spike's crumble is scripted ceiling removal; physically correct collapse (disconnected rock falls) needs a connectivity pass.
- **Art pipeline.** The GLB exporter makes a low-detail engineering mesh; production models are still to be made from the canonical references.

## Owner decisions (2026-11-27)

- **godot-cpp and SCons:** permission granted. G1 uses the godot-cpp 4.5 branch (forward-compatible with Godot 4.7).
- **Desktop GPU:** unavailable at the November 27 decision. For now, G4 targets the dev laptop's Intel Iris Xe; desktop GPU quality/performance claims stay out of scope until measured. Do not interpret laptop feasibility as final visual acceptance.
- **`GODOT_VERIFIED`:** approved. It means verified by an automated headless Godot run on the exact source commit with recorded evidence. It does not imply `UNREAL_VERIFIED` or a human playtest.
