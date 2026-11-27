# Everward Godot world spike (feasibility test, not game code)

Owner question (2026-11-27): can Godot deliver the Everward world feel if Unreal is unusable on the dev laptop? This is a working test of four things, built so it can be re-run and judged:

1. **Clouds that part around the ship.** A raymarched volumetric cloud layer (`cloud.gdshader`, drawn as a full-screen quad, depth-aware so mountains occlude it). The ship's recent positions are passed in as a trail; each point opens a cavity and pushes the noise pattern outward, and the radius shrinks as the point ages so the cloud recovers.
2. **Trees that shake when the ship flies near.** `tree.gdshader` adds ambient wind sway plus a proximity shake and lean-away, scaled by height so roots stay planted. Trees are a `MultiMesh` placed by raycasting the terrain.
3. **A mountain you can drill, hollow out and watch crumble.** Smooth voxel terrain from the Voxel Tools module (`VoxelTerrain` + `VoxelGeneratorNoise2D` + Transvoxel mesher). Drilling and hollowing are real `VoxelTool.do_sphere` removals with live remeshing and collision. "Crumble" is progressive: the cavity ceiling lets go in pieces, each a real voxel removal plus Jolt rigid-body rubble and a dust puff.
4. **Frame rate on weak hardware.** Everything runs on the dev laptop's Intel Iris Xe integrated GPU.

## Run it

Needs a Godot 4.7.x **with the Voxel Tools module compiled in**, for example `godot.windows.editor.x86_64.exe` from https://github.com/Zylann/godot_voxel/releases (v1.7 is Godot 4.7.2). Nothing is downloaded or installed by this project.

    # scripted flight: writes screenshots and metrics.txt, then quits (~30 s)
    godot.windows.editor.x86_64.exe --path tools/godot/world-spike --resolution 960x540 -- --autotest --out=D:\some\folder

    # fly it yourself
    godot.windows.editor.x86_64.exe --path tools/godot/world-spike
    #   W/S thrust, mouse look, A/D strafe, Q/E down/up, Shift boost,
    #   left mouse or F drill, H hollow out ahead, P screenshot, Esc release mouse

Keep the output folder outside the repository; the first launch also writes a `.godot/` import cache that is git-ignored.

## Measured on the dev laptop (Intel Iris Xe, 960x540, Vulkan Forward+, Godot 4.7.2)

| Item | Result |
|---|---|
| Scripted route | 13 screenshots, 0 script errors, about 30 s |
| Frame rate | 44-60 FPS for the whole route; **dips to about 13 FPS for a moment when drilling starts** (remeshing plus a burst of rubble) |
| Memory | about 1.2 GiB static |
| Terrain | 289/289 sampled columns streamed; peak 204 m, cloud layer 110-240 m so the peak breaks through |
| Voxel edit call | 47 edits, 0.05 ms average (the edit only; meshing happens asynchronously) |
| Tree shake | with the shake on, 844 of 8100 sampled pixels changed in a window around the nearest tree against 0 in a control window; off/on frames differ visibly |
| Interactive mode | 900 frames with no input ran with no errors |

## Not verified / known limits

- **How it looks on a real GPU.** Only the integrated GPU was available. Quality settings (cloud steps, shadows, resolution) are untuned for a desktop.
- **Manual controls.** Keyboard and mouse flight, left-click drilling and H hollowing were not exercised by a person; only the scripted path and a no-input smoke run were.
- **Crumble is scripted, not structural.** Pieces of ceiling are removed in sequence; disconnected rock does not detect itself and fall. True structural collapse needs a connectivity pass (flood fill) that is not built.
- **Cloud displacement is a visual trick** (trail cavity plus noise push), not a fluid simulation.
- **Art is placeholder**: cone trees, a capsule ship, a procedural terrain shader, hazy hollow interiors (fog and sky ambient need tuning), and no sound.
- The Voxel Tools binary is a third-party prebuilt Godot build; its SHA-256 was recorded at download (`e2292adb0b671508ab220fce5bcd052b1c20674a3385d89c6b38601e6e1f956b`) but the upstream project publishes no checksum to compare against.
- No connection to `src/simulation`. This project does not read or change the game's rules; an engine move would bind them via GDExtension.
