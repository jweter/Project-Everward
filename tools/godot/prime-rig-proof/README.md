# Everward: Prime A rig Godot import acceptance (experimental)

Owner direction: realistic cinematic 3D only. This small tool proves one existing mechanical bridge's GLB import structure in a real Godot executable; it is NOT a claim about visual quality, gameplay, or complete Prime A art.

## Existing sources
- assets/reference/probe/gen1-prime/: seven canonical owner reference images, two exploratory variants, hashes, modeling checklist.
- tools/procedural_mechanics/prime-arm/: source-controlled two-arm 3D kinematic projection, C++ oracle and deterministic GLB exporter. These assets remain a low-detail engineering mesh, NOT a production model.
- Authoritative simulated contact, material inventory and repair remain owned by src/simulation, NOT Godot or the exported GLB.

## Run on an authorized Windows machine

    powershell -NoProfile -ExecutionPolicy Bypass -File tools/godot/prime-rig-proof/run_import_proof.ps1 -GodotExe "C:\path\to\Godot_v4.7.2-stable_win64_console.exe"

Requires: Godot 4.7.2, Node 22+, CMake and C++20 compiler. On Azathoth/Windows the runner explicitly selects the installed Visual Studio 2026 (MSVC) generator. Other platforms use the default CMake generator. No npm install, cloud generation, paid models or Blender required. Default output is a unique folder under %TEMP%. Override with -WorkDir "C:\safe\external\path". The runner rejects paths inside the repository and does not touch Unreal, owner worktrees, or agent processes. Generated GLBs, imported engine cache and test receipt remain external; no binary assets are silently added to the repository.

Test failures include missing expected shoulder/elbow/wrist/tip nodes, absent named modules, insufficient mesh count, missing C++ numeric parity, exporter failure, Godot import failure, or native Jolt physics query regression. The MSVC build directory stays outside the Godot project to prevent .obj compiler artifacts from being mistaken for 3D model files. Temporary imported scene nodes are freed to avoid misleading runtime leak warnings.

This represents an engine-agnostic integration gate:
1. Source-provenance references: verified separately with python tools/validate_reference_assets.py.
2. JS geometry tests + 40-case C++ oracle agreement: verified by this runner.
3. GLB structurally valid: verified by exporter tests.
4. Godot imports GLBs and keeps all named pivots: verified by this runner.
5. Jolt local physics acceptance: tested zero-gravity and CCD settings, scanner raycasts, collision masks, tool-tip shape overlaps and a swept movement stopping at a physical obstacle.
6. Canonical geometry, textured materials, realistic lighting, full Probe A in actual gameplay, real simulation state and visual acceptance: NOT VERIFIED.

See PHYSICS_INTEGRATION_CONTRACT.md for the documented boundary between authoritative C++ physics and auxiliary local Godot/Jolt queries.

Next art milestone: produce a genuine canonical Prime A GLB based on the seven reference sheets, with physically authored panel structures, PBR textures, modular systems and cinematic gameplay-camera evidence, then repeat under Godot Forward+ and low-spec fallback. No cheap box replacement is acceptable.
