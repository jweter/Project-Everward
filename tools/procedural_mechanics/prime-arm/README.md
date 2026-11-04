# Everward Mechanical Bridge v0.1 — Issue #345

**Status:** 3D source implementation in PR #350. Engine-independent CI is authoritative for mathematics and binary structure only. Unreal/Blender import, GPU profiling, exact-build gameplay contact, screenshot fidelity and artistic acceptance require separate recorded evidence. This is an additive, reversible Phase-2 visual-production path; Unreal Engine 5.8 and the C++ simulation remain authoritative.

## What changed

The original 2D mining-arm demo has been replaced by a **two-arm 3D scene**. `layout.json` is a portable rendering/export projection of `manipulator_hull_contact.hpp`, `manipulator_tool_contact.hpp` and `manipulator.hpp`. `bridge_oracle.cpp` compiles the real C++ headers and emits the authoritative layout and **40 independent shoulder/elbow/wrist/tool-tip positions** (2 arms × 4 deployment fractions × 5 joint poses). `bridge_contract.test.mjs` compares both the numeric layout and all coordinates to `model.mjs` within 1e-9 m on the exact commit. Any future C++ change that invalidates the projection must fail CI rather than silently change gameplay art.

**Probe-local coordinate contract:** +X forward, +Y starboard, +Z up; origin relative to ProbeRoot; 1 metre = 100 Unreal centimetres. The C++ arm uses 1.80 m upper / 1.50 m forearm / 0.79 m tool-tip offset; port/starboard shoulder locations are [1.20, −1.05, −0.92] and [1.20, +1.05, −0.92] m. Shoulder fold pitch changes −102° to −38°, fold roll 8° to 24° signed by arm, and the three joints use the exact C++ ranges. Matrices follow the real `Ry(foldPitch+shoulder) * Rx(side*foldRoll)`, then elbow `Ry(elbow)`, then wrist `Ry(wrist)` composition.

## Fast, zero-dependency checks

From the repository root with Node 22, a C++20 compiler (c++ or CXX), and Python 3:

```sh
node --test tools/procedural_mechanics/prime-arm/*.test.mjs
python tools/procedural_mechanics/prime-arm/test_unreal_bridge_surface.py
node --check tools/procedural_mechanics/prime-arm/main.js
```

These tests compile only a tiny header-driven oracle and run dependency-free JavaScript; they do **not** need npm, Chromium, an LLM, Blender or Unreal. The CI workflow runs them under a six-minute timeout.

## Deterministic modular GLB export

```sh
node tools/procedural_mechanics/prime-arm/export_glb.mjs teardown-out/prime-arm-bridge
```

Outputs (gitignored):

- `prime-arm-modules.glb` — unit-normalized, zero-centred independent upper-arm, forearm and tool meshes. Long arms use **local +X**; the mining tool uses **local +Z**, matching the existing Unreal cylinder's Pitch=90° mounting.
- `prime-arm-assembly.glb` — illustrative 3D port/starboard assembly with separate shoulder, elbow and wrist pivot nodes plus named authoritative tool-tip markers. A root Y-up conversion represents the native Z-up source in glTF's coordinate system.
- `prime-arm-manifest.json` — module names, exact expected Unreal component offsets/scales, SHA-256 output checksums, authority and an explicit `claimed_unreal_verified: false` flag.

The GLB generator uses only Node built-ins and the original `module_parts.mjs` catalog, **not downloaded meshes**. It never writes to the Unreal project or Iron God. Geometry is a fast low-poly engineering pass, not final canonical art. The per-part GLB is designed to import as three distinct meshes; the automated test checks the binary header, scene/mesh layout, array bounds, deterministic bytes, contact marker and named joint hierarchy. Actual Blender/Unreal parsing and pivot fidelity are a separate gate.

## Optional offline Three.js studio

Upstream inspiration: `elvarofficial/teardown` at reviewed/pinned commit `a91262ac1109051e014662d42ecb0b7bc1f113b1`. After an **independent license, lockfile, install-hook, cost and memory audit**, place its checkout in the ignored `.tools/teardown/` path and run:

```sh
node .tools/teardown/sims/build.mjs ./tools/procedural_mechanics/prime-arm
```

Expected ignored output: `teardown-out/dist/prime-arm.html`. The studio shows two physically arranged arms, both tool contact markers, deterministic deploy/joint motion, an exploded non-physical disassembly phase and a basic mechanism readout. Do not present the studio as actual in-game physics. It is suitable for locked-camera render/compare/refine work once rendering is verified. No automatic dependency installation, downloaded Chromium, network service or global Claude skill is part of this PR.

## Reversible Unreal 5.8 connection

1. Work in an isolated disposable Unreal checkout, not the active owner or unattended worker workspace. Validate source GLB checksums and inspect the imported content. The guarded `unreal_import.py` can import into `/Game/MechanicalBridge/Pilot` when run inside the Unreal Editor's Python environment with **explicit** `EVERWARD_ARM_IMPORT_SAVE=1`; default behavior refuses to modify Content. If the chosen importer merges meshes, **fail** and split/convert them in an audited Blender workflow instead.
2. Check imported static mesh local axes and size. Each `EW_Arm_Upper` and `EW_Arm_Forearm` is normalized to one metre along +X; the tool is one metre long along +Z and is centred. Confirm no accidental centimetre↔metre scaling, incorrect pivot recentering or flipped mesh normals.
3. The existing `AEverwardProbePawn` exposes three optional StaticMesh soft references: `ProceduralUpperArmMesh`, `ProceduralForearmMesh`, `ProceduralMiningToolMesh`. Assign imported assets in a test pawn instance or its Blueprint defaults. If the project GameMode spawns the native C++ class rather than that Blueprint, point the test GameMode to that Blueprint first. With empty references the existing BasicShapes remain automatically. All pivots, scales, collision flags and the real arm `GetManipulatorArmStates()` animation remain unchanged.
4. Verify in Unreal: both arms stow/deploy, shoulder/elbow/wrist motion, tool attach/detach, correct tool-tip-to-asteroid contact and mining/storage events; run `Fix_It` resource-spend/recovery test if reachable. Record **the actual unedited gameplay**, viewport frames, tool-tip vs collision location, deterministic test results and exact commit.
5. Run the existing approved unattended Windows worker only under its opt-in/idle guards. On the 16 GB integrated-GPU laptop measure startup RAM, frame time and functional parity under Emergency and Low (and higher profiles if supported). Do not claim this complete based only on GLB size or GitHub CI.

**Component truth boundary:** `UProbeSimulationAdapter::GetManipulatorArmStates()` remains the only source for presentation. C++ `manipulator_tool_tip_contact_sample()`/`MiningSystem` decide contact and storage; changing a mesh never changes reach or damage. Explosion offsets belong only to the studio and must not appear in authoritative collision transforms.

## Acceptance statuses (do not collapse)

| Evidence | Present with code PR | Meaning |
|---|---|---|
| C++ → JavaScript numeric parity | Gate in CI | Both render/export geometry model and simulation agree on sampled poses, not actual UE contact |
| GLB structural validator and repeatability | Gate in CI | Binary integrity, named components/pivots, deterministic export |
| Unreal mesh override hook | Source only pending UBT | Artwork can be swapped without changing adapter semantics |
| Actual Blender/Unreal import | Pending Product Reality | Validate units, materials, pivot preservation and visibility |
| Actual Unreal mining collision and storage | Pending Product Reality | Required before player-visible acceptance |
| Low/Emergency GPU, memory and gameplay captures | Pending Product Reality | Required before shipped presentation |
| Subjective Prime A visual acceptance | Pending milestone | User/art director acceptance only after integrated playtest |

**Priority:** this is the parallel-safe machinery workstream. The starter-restoration objective from `FIRST_PLAYABLE_LOOP_INVENTORY.md` remains the earliest missing first-playable-loop dependency. Do not conflate a working studio with a playable game or hold up other nondependent development while hardware verification is pending.
