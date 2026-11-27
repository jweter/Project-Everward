# Everward simulation in Godot (Godot path phase G1)

A GDExtension that runs the real Everward simulation (`src/simulation`) inside Godot, plus a parity check proving that Godot gets exactly the same numbers as a native C++ run.

- `src/everward_sim_core.hpp`: a small engine-independent core that owns the real simulation objects (canonical probe runtime, manipulator rig, mining, save/load). Like the game's mining bridge, it builds a fresh `MiningSystem` for each attempt, so the runtime's storage is the only cargo account. Loads are validated and built in temporaries before anything is replaced.
- `src/everward_sim.*`, `src/register_types.cpp`: the Godot wrapper (`EverwardSim`, a `RefCounted`). It only converts types. Positions are returned as `PackedFloat64Array`, because Godot's `Vector3` is single precision.
- `src/parity_scenario.hpp` and `src/native_parity_main.cpp`: the native reference run.
- `project/tests/parity.gd`: the same steps from GDScript through the extension.
- `check_parity.py`: runs both and fails on any difference.

The scenario sets a velocity and advances 0.6 s, mines six cycles from a 250 kg deposit, saves, mines four more, loads the save, mines two more, then feeds in a corrupt save, which must be rejected with nothing changed. Three review regressions follow: a 600 kg deposit must fill the 500 kg hold (storage counted once, as the game's mining bridge does); loading a save taken before the deposit existed must clear the deposit; a well-formed save with an invalid value (`mass_kg` of -1) must be rejected, never thrown into Godot; and invalid commands (negative ticks, a NaN velocity, a negative deposit) must return false and leave the simulation untouched. No C++ exception crosses the Godot boundary: every command validates its input and contains simulation errors.

## Status vocabulary

`GODOT_VERIFIED` (approved by the owner on 2026-11-27) means: verified by an automated headless Godot run on the exact source commit, with the evidence recorded. It sits alongside `UNREAL_VERIFIED`; it does not replace or imply it, and neither implies a human playtest.

**The parity scenario is GODOT_VERIFIED:** 45 of 45 lines are identical between native and Godot 4.7.2 (headless, official build), covering the G1 tick/mining/save scenario, G2 flight and contact, the G3 first loop and Fix_It. The checker also fails correctly when the native side prints something else (negative control).

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

## G2 flight and contact

`EverwardSim` also exposes `adjust_attitude(yaw, pitch, roll)`, `adjust_local_velocity(x, y, z)` (the probe's own frame: x forward, y right, z up), `get_attitude()`, `get_velocity()`, `add_sphere_body(id, x, y, z, radius)`, `clear_bodies()`, `move_sphere_body(id, x, y, z)` (false for an unknown id; the simulation itself ignores unknown ids silently), `get_last_contact()` and `drain_contact_events()`. The parity scenario turns the probe, thrusts it and flies it into a rock: one contact event on that rock, identical natively and in Godot. The world spike uses this for simulation-driven flight (`--simflight`, see its README).

## G3 first loop

Time now advances in the Unreal adapter's fixed step (16667 ticks, about 1/60 s): `advance_ticks` accumulates wall time and steps the runtime and the manipulator rig together, so the arm path planner's frame time equals the follower's. The rig carries the adapter's collision guard (hull self-collision plus every registered body).

| Method | What it does |
|---|---|
| `add_deposit(id, x, y, z, radius, material_id, display_name, remaining_kg, kg_per_cycle)` | registers a body the probe collides with plus the resource it holds |
| `start_scan(id, seconds)`, `cancel_scan()`, `is_scanning()`, `get_scan_remaining_s()`, `get_scan_target()` | the simulation's scan lifecycle |
| `is_surveyed(id)` | latched from a genuine `ScanCompleted` event for that id (`first_loop_controls.hpp`); a later cancelled scan never clears it (#378 G05) |
| `toggle_arm(arm)` | 0 port, 1 starboard; the direction comes from `arm_toggle_command`, so a press mid-deploy stows and mid-stow deploys (G06) |
| `attach_tool(arm)`, `detach_tool(arm)`, `set_joint_targets(arm, s, e, w)`, `get_arm_state(arm)`, `get_arm_points(arm)` | the rig; points are world shoulder, elbow, wrist and tool tip for drawing |
| `plan_staging(arm, id)` | the #375 solver (`mining_staging_solution`), same call as the Unreal adapter |
| `plan_arm_path(arm, s, e, w)` | `plan_collision_free_joint_path` waypoints with the rig's guard |
| `mine_deposit(id)` | `MiningSystem::mine_once` from the probe's real pose; a fresh system per attempt, as `ProbeMiningBridge` does |

The parity scenario runs the loop natively and in Godot: mining before the survey is refused, the scan surveys, a cancelled rescan keeps the survey, mining from 40 m is refused, then staging, a 2 m/s approach, an 8-waypoint arm path and an accepted 5 kg cycle into named inventory.

Saves (#378 G08): `save_json` writes every world deposit (material, display name, remaining and extracted kg, yield, survey) into the canonical save's additive `world_deposits` field (`save_data.hpp`, tested by `src/simulation/tests/world_deposit_save_tests.cpp`), and `load_json` restores exactly those, so a load can no longer leave a later, smaller deposit beside an earlier inventory or carry over deposits and surveys from the session. Fix_It progress is not saved (a load resets it). Known limits: arm points use `compound_contact.hpp`'s roll sign, which differs from the core's (#441), so they are exact only at zero roll.

### Fix_It (#378 G02)

`enable_fix_it(seed_damaged_awakening)` runs Fix_It on the fixed step exactly as the Unreal `FixItRuntimeActor` does: when idle it asks `FixItPlanner::plan_next` for the next decision, starts a Repair only when stored material and energy cover it (otherwise it waits and spends nothing), advances `FixItRepairExecutor`, and reports completion with the materials consumed. Replacement waits (fabrication is not online); Upgrade, Redesign and Evolution are only shown. With the awakening seed an undamaged probe first drops to the Unreal Gen-1 startup integrity (computation 18%, thermal 22%, sensors 35%, propulsion 40%), and the simulation's damage rules then apply: trims are scaled by propulsion integrity and scans take longer with damaged sensors. Read it with `get_fix_it_status()` (state off/monitor/waiting/repairing, stage, subsystem, reason, progress, material and energy required and consumed, last event), `get_integrity()`, `get_stored_energy_j()` and `drain_fix_it_events()`. The parity lines show it waiting for 0.98 kg with nothing stored, then, after one mined cycle, repairing computation from 18% to 25% and thermal to 25% with the mined material and stored energy.

### Power and software policy (#378 G09)

`allocate_power(subsystem, watts)` (0 sensors, 1 propulsion, 2 computation, 3 thermal), `get_power_allocations()`, `install_basic_survival_policy()` (the Unreal adapter's GEN1 BASIC SURVIVAL policy rule for rule), `clear_policy()`, `get_policy_status()`, `get_scan_power_guidance()` (the simulation's `sensor_power_guidance`) and `get_last_scan_refusal()` (why the last `start_scan` was refused). The parity lines reproduce the G09 drop: at 50% stored energy the policy sheds sensors to 0 W, scans are refused with "Clear policy; raise power; scan", raising sensors alone does not hold while the policy is installed, and clear, raise, scan surveys.

## Not covered yet

Fix_It replacement and fabrication, tractor field, sensor power policy (G09) and the remaining adapter surface (`docs/GODOT_PATH_PLAN.md`).
