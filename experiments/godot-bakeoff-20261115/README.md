# Everward Godot engine-bakeoff proof

This is an **isolated, simplified GDScript feasibility prototype**, not a port of the authoritative Everward C++ simulation and **not** yet an apples-to-apples winner in issue #404. It is a small 3D playable test of laptop launch, agent-authored scenes, visual presentation, basic movement, scan, mining, material accounting, Fix_It-inspired repair, and save/load.

Launch from this folder:

```powershell
& "$HOME\Documents\Everward Engine Bakeoff\tools\godot-4.7.2\Godot_v4.7.2-stable_win64.exe" --path "$HOME\Documents\Everward Engine Bakeoff\godot-proof"
```

Controls: WASD move in local horizontal plane; Q/E vertical; Space brake; T scan; Y approach target automatically; F mine while tool is in range; R repair with harvested resources; F5 save and F6 load; H hide HUD. Follow the on-screen status.

Headless scripted mechanics smoke:

```powershell
& "$HOME\Documents\Everward Engine Bakeoff\tools\godot-4.7.2\Godot_v4.7.2-stable_win64_console.exe" --headless --path "$HOME\Documents\Everward Engine Bakeoff\godot-proof" --quit-after 5 -- --self-test
```

**Limitations**: GDScript mechanics here are provisional and differ from the authoritative C++ Everward physics/recipes/collision, so this cannot be used for gameplay-comparison scores until the real core is bound. A passing headless check is not graphical or owner-playability acceptance; commercial quality, asset pipeline, build packaging, unattended gameplay input verification, repeatable performance runs, and the comparison against Unreal remain PENDING. No paid services or protected worktrees are touched.
