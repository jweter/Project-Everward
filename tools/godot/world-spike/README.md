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
    #   left mouse or F drill, H hollow out ahead, P screenshot,
    #   Esc releases the mouse (Esc again, or a click in the window, recaptures it)

Looks: add `--look=ember_dusk|moonrise|storm_ridge|autumn_mist` to any run to apply a palette from the owner's references (`looks.gd`; see `docs/LOOK_DEVELOPMENT.md`). `-- --lookshots --look=NAME --out=FOLDER` renders the same three framings for comparison and exits 0.

Simulation-driven flight (Godot path G2): `-- --simflight` makes the Everward simulation move the ship through the `EverwardSim` extension from `tools/godot/sim-binding` (build it first; `everward_sim.gdextension` here points at its DLL, and a fresh checkout needs one `--headless --import` to register it, which hits the known first-import crash after finishing). Rocks are registered in both Godot and the simulation; contact comes only from the simulation. The run passes (exit 0) only if exactly one contact is reported on the boulder flown into, the ship's Godot position equals the simulated position (mapped from the simulation's x-forward, y-right, z-up frame), and after a 90-degree turn the ship moves where its nose points, and after a +90-degree roll, thrust toward the probe's right moves the ship along its displayed right; otherwise it exits 6 (5 if the extension did not load). Measured headless on the dev laptop: 1 contact at 14 m/s, 0.000000 m position error, heading match 1.0000; with the yaw sign deliberately flipped the run fails (heading -1.0000, exit 6). **Terrain contact.** The simulation only knows spheres, so three large proxy spheres follow the voxel ground around the ship: straight down, along the flight direction and along the nose. Each is placed so its surface touches the ground where that ray hits it, and the simulation decides terrain contact like any other. The sim-flight run adds a nose-down dive that must produce a terrain contact and leave the ship above the ground. Measured: contact after 8.75 s, 3.26 m clearance. With the proxies disabled as a negative control, the ship ended 98.79 m inside the mountain and the run failed. This is an approximation (three spheres of 40 m radius, refreshed every frame), not true terrain collision.

**Fly it yourself on the simulation:** `-- --sim` (needs the extension). W/S/A/D/Q/E become probe-frame thrust, the mouse turns the probe through the simulation, Space stops, Shift boosts. Two rocks are placed ahead of the start. `-- --selftest-sim` drives the real controls with synthetic input, headless: holding W for 1 s gave 18.04 m/s straight along the nose, and a 100 px mouse move turned the probe exactly 12 degrees. `-- --selftest-input` (mouse capture) needs a window and reports SKIP when headless.

Every mode also runs `--headless` (no window): screenshots and the image-based tree-shake check are then skipped and say so in `metrics.txt`.

Exit codes for `--autotest`: 0 route completed; 2 terrain did not stream in time; 3 no mountain face found (drilling, hollowing and crumbling skipped); 4 no trees could be placed. `metrics.txt` records the reason. Two self-tests exist for the checks that cannot be seen in screenshots: `-- --selftest-input` drives the real input handler with synthetic Escape and click events and exits 0 on PASS; `-- --autotest --selftest-fail=face` forces the missing-face failure and must exit 3.

Keep the output folder outside the repository; the first launch also writes a `.godot/` import cache that is git-ignored.

## Measured on the dev laptop (Intel Iris Xe, 960x540, Vulkan Forward+, Godot 4.7.2)

| Item | Result |
|---|---|
| Scripted route | 13 screenshots, 0 script errors, about 30 s |
| Frame rate | 44-60 FPS for the whole route; **dips to about 13 FPS for a moment when drilling starts** (remeshing plus a burst of rubble) |
| Memory | about 1.2 GiB static |
| Terrain | 289/289 sampled columns streamed; peak 204 m, cloud layer 110-240 m so the peak breaks through |
| Voxel edit call | 47 edits, 0.05 ms average (the edit only; meshing happens asynchronously) |
| Tree shake | The vertex displacement runs: at 25x strength a window around the tree changed in 542 of 8100 sampled pixels against 0 in a control window, and the side-by-side off/on frame (`05_compare_off_left_on_right.png`) shows the tree bent differently. **The pixel count is noisy at the default strength**: across runs the tree window changed 208, 844 and 58 pixels while the control window (which also sees cloud and terrain changes) changed 0, 0 and 878. Treat the shake as visually confirmed, not statistically measured. |
| Interactive mode | 900 frames with no input ran with no errors |

## Not verified / known limits

- **How it looks on a real GPU.** Only the integrated GPU was available. Quality settings (cloud steps, shadows, resolution) are untuned for a desktop.
- **Manual controls.** Keyboard and mouse flight, left-click drilling and H hollowing were not exercised by a person; only the scripted path, a no-input smoke run and the Escape/click mouse-capture self-test were.
- **Crumble is scripted, not structural.** Pieces of ceiling are removed in sequence; disconnected rock does not detect itself and fall. True structural collapse needs a connectivity pass (flood fill) that is not built.
- **Cloud displacement is a visual trick** (trail cavity plus noise push), not a fluid simulation.
- **Art is placeholder**: cone trees, a capsule ship, a procedural terrain shader, hazy hollow interiors (fog and sky ambient need tuning), and no sound.
- The Voxel Tools binary is a third-party prebuilt Godot build; its SHA-256 was recorded at download (`e2292adb0b671508ab220fce5bcd052b1c20674a3385d89c6b38601e6e1f956b`) but the upstream project publishes no checksum to compare against.
- No connection to `src/simulation`. This project does not read or change the game's rules; an engine move would bind them via GDExtension.
