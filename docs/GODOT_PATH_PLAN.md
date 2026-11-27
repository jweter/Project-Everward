# Godot path plan

**Status:** direction approved by the owner on 2026-11-27 ("Godot is the next development path"). Unreal is frozen: no new Unreal feature work, and every #378 gate item that needs the native Unreal run stays **unverified** (nothing is ticked). The visual quality of Godot is **not yet judged**; the only Godot evidence is the feasibility spike in `tools/godot/world-spike`, merged in #433 (see its README for how to run it, the measurements and the limits), run on the dev laptop's integrated GPU.

**Why:** the native Unreal verification run needs more RAM than the dev laptop has free, and the laptop is not upgradable. Godot runs the same spike at 44-60 FPS on that laptop and runs headless for automated tests.

**Rule that does not change:** `src/simulation` owns authoritative state. The engine is presentation and input only.

## What stays and what is replaced

| Stays (engine independent) | Replaced (Unreal only, about 10,600 lines) |
|---|---|
| `src/simulation`: 49 headers, 50 test files, about 23,000 lines, including the collision-free arm planner (#431) | `ProbeSimulationAdapter` (about 2,000 lines): the object that wraps the simulation for the engine |
| `tools/procedural_mechanics` (C++ oracle, GLB exporter) and `tools/godot/prime-rig-proof` | Bridges: mining, tractor field, target selection, surface descent (about 1,450 lines) |
| `assets/reference/**` and their manifest validator | Pawn and player controller, mining controls, autopilot, hover, descent (about 2,000 lines) |
| All C++ and Python tests that do not need Unreal | HUD (`EverwardHUD`, about 1,300 lines; its read models `ProbeHUDReadModel` and `PeripheralHUDLayout` are already plain C++ and are reused) |
| | Test environments and the Fix-It actor; the 12 native Unreal automation tests |

The paid Aura playtest tooling was Unreal-specific and owner-only; no replacement is planned.

## Mapping

- **Adapter** becomes a GDExtension class (godot-cpp, C++20) that owns the same simulation objects and ticks them at the fixed 16,667 us step from `_physics_process`. It exposes the same commands and read-only snapshots the adapter exposes today.
- **Bridges** are small C++ helpers moved into the extension.
- **Pawn, controller, controls** become Godot scripts on a flight scene. Joint control uses the simulation's own path planner, so the "arm stalls at the hull" defect fixed in #431/#432 cannot return.
- **HUD** becomes Godot `Control` scenes fed by the existing C++ read models.
- **Native tests** become headless Godot runs (a script runner) plus the existing C++ tests, which keep running unchanged on the laptop.

## Phases and the evidence each one must produce

| Phase | Scope | Exit evidence |
|---|---|---|
| G0 | Feasibility spike (done: merged in #433 as `tools/godot/world-spike`; run it per its README) | `-- --autotest` route, screenshots, `metrics.txt`, README limits table |
| G1 | Binding spike: a GDExtension that ticks the canonical probe and returns a snapshot | A headless Godot script ticks N steps, runs one mining cycle and a save/load round trip, and its numbers equal the C++ test's |
| G2 | Flight and world: the spike's terrain, clouds and trees driven by the simulated probe, contact events from the simulation | Scripted flight where simulated position, not engine physics, moves the ship; contact reported once per impact |
| G3 | First-loop parity with #378 G01-G13, re-expressed for Godot, including the planned arm path and the HUD | Headless test per gate item with the same pass criteria; the status vocabulary gains `GODOT_VERIFIED` (proposed, needs owner approval) |
| G4 | Visual pass on the desktop GPU | The owner's judgment on screenshots and a flythrough; tuning list |
| G5 | Retire Unreal | Parity evidence for every gate item, then archive `unreal/` in a normal PR |

## Risks

- **Look.** Godot has no Nanite or Lumen equivalent; "epic" must come from custom shaders and art. Unproven until G4 on a real GPU.
- **GDExtension build.** godot-cpp plus the existing MSVC toolchain must build C++20 headers cleanly; G1 exists to find out early.
- **Voxel Tools.** The spike uses a third-party prebuilt Godot with the voxel module compiled in. A shipping path likely wants the GDExtension build or a build from source; building Godot from source is memory-heavy on this laptop.
- **Destruction.** The spike's crumble is scripted ceiling removal; physically correct collapse (disconnected rock falls) needs a connectivity pass.
- **Art pipeline.** The GLB exporter makes a low-detail engineering mesh; production models are still to be made from the canonical references.

## Needs from the owner (not started)

- Permission to download the official godot-cpp source and install SCons for G1.
- The desktop GPU model, and a date when it is available for G4.
- Approval of the `GODOT_VERIFIED` status term before any gate is marked with it.
