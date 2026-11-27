# Everward simulation in Godot (Godot path phase G1)

A GDExtension that runs the real Everward simulation (`src/simulation`) inside Godot, plus a parity check proving that Godot gets exactly the same numbers as a native C++ run.

- `src/everward_sim_core.hpp`: a small engine-independent core that owns the real simulation objects (canonical probe runtime, manipulator rig, mining, save/load). Like the game's mining bridge, it builds a fresh `MiningSystem` for each attempt, so the runtime's storage is the only cargo account. Loads are validated and built in temporaries before anything is replaced.
- `src/everward_sim.*`, `src/register_types.cpp`: the Godot wrapper (`EverwardSim`, a `RefCounted`). It only converts types. Positions are returned as `PackedFloat64Array`, because Godot's `Vector3` is single precision.
- `src/parity_scenario.hpp` and `src/native_parity_main.cpp`: the native reference run.
- `project/tests/parity.gd`: the same steps from GDScript through the extension.
- `check_parity.py`: runs both and fails on any difference.

The scenario sets a velocity and advances 0.6 s, mines six cycles from a 250 kg deposit, saves, mines four more, loads the save, mines two more, then feeds in a corrupt save, which must be rejected with nothing changed. Three review regressions follow: a 600 kg deposit must fill the 500 kg hold (storage counted once, as the game's mining bridge does); loading a save taken before the deposit existed must clear the deposit; and a well-formed save with an invalid value (`mass_kg` of -1) must be rejected, never thrown into Godot.

## Status vocabulary

`GODOT_VERIFIED` (approved by the owner on 2026-11-27) means: verified by an automated headless Godot run on the exact source commit, with the evidence recorded. It sits alongside `UNREAL_VERIFIED`; it does not replace or imply it, and neither implies a human playtest.

**G1 is GODOT_VERIFIED for this scenario only:** 17 of 17 lines are identical between native and Godot 4.7.2 (headless, official build). The checker also fails correctly when the native side prints something else (negative control).

## Build (Windows)

Needs Visual Studio's C++ compiler, Python with `pip install scons`, and a godot-cpp checkout on the **4.5** branch. GDExtensions are forward-compatible, so one built against 4.5 loads in 4.7.

    git clone --depth 1 --branch 4.5 https://github.com/godotengine/godot-cpp.git C:\somewhere\godot-cpp
    REM from a "x64 Native Tools" / vcvars64 shell, in this folder:
    scons -j4 godot_cpp=C:\somewhere\godot-cpp platform=windows target=template_debug disable_exceptions=no use_static_cpp=no

`disable_exceptions=no` is required: the simulation's save parser reports bad input with exceptions.

On the dev laptop, SCons did not detect the preview (Insiders) Visual Studio 18. The local godot-cpp checkout was patched so that, inside a vcvars64 shell, `tools/windows.py` uses that environment instead of searching for an install. With a released Visual Studio this is not needed.

Native reference: compile `src/native_parity_main.cpp` with `-I../../../src/simulation/include` and C++20, using any compiler (`cl /std:c++20 /EHsc`, or `python -m ziglang c++ -std=c++20`).

## Check

    python check_parity.py --godot PATH\Godot_v4.7.2-stable_win64_console.exe --native PATH\native_parity.exe

## Known issue

On Godot 4.7.2 with this toolchain, the **first** headless `--import` of a project that registers **any** extension class crashes on exit (139 / 0xC0000005) after the import has finished and written its cache. This was narrowed down as follows:

- An extension that registers no classes imports cleanly.
- A trivial one-method class crashes the same way.
- It crashes with godot-cpp 4.5 and with godot-cpp master built for API 4.7, with the static or the shared C runtime.

Every later import and every run exits 0. So it is not caused by the simulation code, but its root cause (engine or preview compiler) is not established. `check_parity.py` reports it, checks that the cache exists, and re-imports once. It never accepts a crash or a mismatch in the parity run itself.

## Not covered yet

Only this scenario is bound. Flight, contact, scanning, the arm path planner, HUD read models and the rest of the adapter surface are G2 and G3 work (`docs/GODOT_PATH_PLAN.md`).
