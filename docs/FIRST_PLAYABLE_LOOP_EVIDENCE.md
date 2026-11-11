# First Playable Loop Evidence

Baseline: `af3cf2d296112f1913e5b5d30c8d1fe76e966685`.
Traceability: `engineering/capability-graph.json#phase2-one-probe-development-access/first-loop-verification`.
Depends on: `docs/FIRST_PLAYABLE_LOOP_INVENTORY.md`.

Only documentation and tests change in this slice. No gate was weakened.

## Verification scope (read first)

The runner is a Linux sandbox with CMake, Ninja, g++ and Python. It has **no Unreal Editor, no UnrealBuildTool, no Windows host and no display**. So:

- **PASSED (portable)** means the engine-independent C++ simulation tests, or the Python source-contract tests, ran on the runner and exited 0 at the baseline. Simulation truth is verified; presentation is not.
- **PENDING** means the step needs Unreal compile, an in-editor/runtime session, a HUD, or a mechanic that does not exist. It is never reported as passed.
- The Python `tools/test_phase2_*_surface.py` tests are static source-contract checks. They show the Unreal wiring is present in source. They do **not** show it compiles or behaves in-engine.

## Reproduce the portable checks

```
cmake -S src/simulation -B build/simulation -G Ninja -DBUILD_TESTING=ON
cmake --build build/simulation
ctest --test-dir build/simulation --output-on-failure   # observed: 100% passed, 0 failed, 40 of 40
python3 -m unittest tools/test_phase2_<name>.py        # per-step names below
```

## Per-step results

| # | Loop step | Command / procedure | Observed at baseline | Status |
|---|---|---|---|---|
| 1 | Movement (translation, stop, orientation/righting, power-gated propulsion, contact, damage) | `ctest --test-dir build/simulation -R "simulation_tests\|attitude_control\|compound_contact\|impact_damage\|contact_physics\|software_policy"` | All passed | PASSED (portable). In-engine movement: PENDING |
| 2 | Scanning / observation | `ctest --test-dir build/simulation -R "science_knowledge\|simulation_tests"`; `python3 -m unittest tools/test_phase2_scan_discovery_surface.py tools/test_phase2_science_knowledge_surface.py` | ctest passed; surface tests OK (2 and 14 tests) | PASSED (portable + source contract). In-engine scan/HUD: PENDING |
| 3 | Target selection and manipulation | `ctest --test-dir build/simulation -R "target_selection\|target_cycle\|manipulator"` | All passed | PASSED (portable). In-engine arm control: PENDING |
| 4 | Resource collection (mining and whole-sample) | `ctest --test-dir build/simulation -R "mining\|manipulator_collection"`; `python3 -m unittest tools/test_phase2_scan_to_mining_surface.py tools/test_phase2_manipulator_collection_surface.py tools/test_phase2_material_inventory_surface.py` | ctest passed; surface tests OK (7, 13, 4 tests) | PASSED (portable + source contract). In-engine collection: PENDING |
| 5 | Progression: repair (`Fix_It` consumes stored material and energy) | `ctest --test-dir build/simulation -R "fix_it\|material_consumption\|self_repair"`; `python3 -m unittest tools/test_phase2_fix_it_playable_unreal.py` | ctest passed; surface tests OK (7 tests) | PASSED (portable + source contract). Unreal compile and playtest: PENDING |
| 6 | Objective status (starter restoration derived from component integrity) | `ctest --test-dir build/simulation -R starter_restoration` | Passed | PASSED (portable). Unreal compile of `GetStarterRestorationStatus()` and HUD use: PENDING |
| 7 | Persistence (save/load incl. inventory and knowledge) | `ctest --test-dir build/simulation -R save_data` | Passed | PASSED (portable). Save/load UI in-engine: PENDING |
| 8 | Construction / fabrication | None. No fabrication module exists (`docs/PHASE2_VERTICAL_SLICE_PLAN.md` Slice 13). | Not run | PENDING (not implemented) |
| 9 | Repeatable objective | `ctest --test-dir build/simulation -R objective_cycle` | Passed | PASSED (portable). The engine-independent repeatable objective (`objective_cycle.hpp`, built on starter restoration) can be triggered and completed any number of times from simulation state and round-trips through save/load. In-engine wiring written, Unreal compile and native run PENDING: `UProbeSimulationAdapter::CommandTriggerNextRestorationCycle()` / `GetRestorationObjectiveCycleStatus()` forward to `objective_cycle.hpp` each fixed step (`Everward.G03.Native.RestorationObjectiveCycleMirror` checks trigger, re-trigger rejection and no completion without damage). In-engine damage-then-repair to Completed, HUD display and play: PENDING |
| 10 | End-to-end first loop in editor or packaged runtime (wake, scan, collect, repair, restore) | Requires Unreal Editor or a packaged Windows build. `docs/PHASE2_LOCAL_PLAYTEST_HARNESS.md` and `docs/PHASE2_FIRST_RUN_PLAYTEST.md` describe the local procedure. | Not run on this runner | PENDING (no Unreal/Windows on runner) |

## Limits

- Per-step portable tests exercise each step's simulation truth in isolation. `src/simulation/tests/first_loop_chained_tests.cpp` (ctest `everward_first_loop_chained_tests`) additionally chains damaged wake, a real runtime scan, mining/collection, Fix_It repair (consuming stored material and energy) and starter restoration in one deterministic simulation run, executed twice and compared exactly. The only direct state writes are the documented damaged-wake integrity values; material arrives only via mining. This is simulation-level proof only: the in-engine chain (step 10) remains PENDING.
- "Complete first loop verified in editor or runtime" is **not** achieved. Steps 8 to 10 stay pending, and so does every in-engine item above.
- Remaining Product Reality debt: Unreal build and packaging, controls/HUD usability, in-engine playtest of the loop, fabrication (Slice 13), and in-engine wiring of the repeatable objective (step 9's simulation truth is now covered; its Unreal trigger UI/HUD is not).
