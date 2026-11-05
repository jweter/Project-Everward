# First Playable Loop Inventory

Baseline: `a00f948873edc8bbcaa1c863327410a39aa1a2b9`. Documentation only; no gameplay, content, or config change.

Traceability: `engineering/capability-graph.json#phase2-one-probe-development-access/inventory-playable-mechanics`; authorized by `docs/OWNER_APPROVED_ROADMAP_2026-10-25.md` ("First playable Unreal milestone", Next slice 1).

Scope rule: every claim cites a path that exists at the baseline. "Implemented" means code plus deterministic tests exist. It never means locally accepted Product Reality: `docs/PROJECT_STATUS.md` records Unreal acceptance as pending for most items, and no Unreal Editor/UBT build was available in the authoring sandbox. Nothing below is claimed as in-engine verified.

## Mechanics inventory

| Roadmap element | State at baseline | Evidence |
|---|---|---|
| Player-as-probe movement | Implemented. Authoritative translation, full stop, orientation and camera-aligned righting, power-gated propulsion, contact and damage. | `src/simulation/include/everward/simulation/core.hpp`, `software_policy.hpp`, `compound_contact.hpp`, `impact_damage.hpp`; tests `src/simulation/tests/simulation_core_tests.cpp`, `attitude_control_tests.cpp`, `compound_contact_tests.cpp`, `impact_damage_tests.cpp`; `docs/PROJECT_STATUS.md` ("Verified local foundation") |
| Scanning / observation | Implemented. Power-gated active scan; per-target knowledge; classification reveal, discoveries catalogue and passive observation. | `core.hpp` (`start_scan`), `science_knowledge.hpp`, `composition_estimate.hpp`; tests `science_knowledge_tests.cpp`, `simulation_core_tests.cpp`; `unreal/Source/Everward/ProbeSimulationAdapter.cpp` (`CommandStartScan`); `tools/test_phase2_scan_discovery_surface.py`, `tools/test_phase2_science_knowledge_surface.py`; `docs/PHASE2_SCIENCE_KNOWLEDGE_TEST.md` |
| Target selection and manipulation | Implemented. Select/cycle, reach telemetry, grasp, move, release, arm collision. | `target_selection.hpp`, `manipulator.hpp`, `manipulator_reach.hpp`, `manipulator_grasp.hpp`, `manipulator_move.hpp`, `manipulator_release.hpp`, `manipulator_hull_contact.hpp`; tests `target_selection_tests.cpp`, `manipulator_*_tests.cpp` |
| Resource collection | Implemented. Mining (`mine_once`) and whole-sample collection both credit authoritative storage with per-material identity. Two sampleable bodies exist in the test scene. | `mining.hpp`, `manipulator_collection.hpp`, `core.hpp` (`add_stored_material_kg`, `material_inventory_kg`); tests `mining_tests.cpp`, `manipulator_collection_tests.cpp`, `simulation_core_tests.cpp`; `unreal/Source/Everward/ProbeMiningBridge.cpp`, `EverwardPlayerControllerMiningControls.cpp`; `tools/test_phase2_scan_to_mining_surface.py`, `tools/test_phase2_manipulator_collection_surface.py`, `tools/test_phase2_material_inventory_surface.py`; `docs/PHASE2_SCAN_TO_MINING_TEST.md`, `docs/PHASE2_MANIPULATOR_COLLECTION_TEST.md` |
| Progression / construction (repair) | Implemented for repair only. The probe wakes damaged (computation 18%, thermal 22%, sensors 35%, propulsion 40%). `Fix_It` plans, waits on real material and energy, and runs repair or replacement that consumes stored material and raises integrity. | `fix_it.hpp`; `unreal/Source/Everward/FixItRuntimeActor.cpp` (damaged-awakening seeding, `fix_it_repair_*` events), spawned in `EverwardGameMode.cpp`; tests `src/simulation/tests/fix_it_tests.cpp`, `material_consumption_tests.cpp`; `tools/test_phase2_fix_it_playable_unreal.py`; `docs/PHASE2_FIX_IT_PLAYABLE_TEST.md` |
| Progression / construction (fabrication) | Not implemented. Slice 13 is only a plan, and no processing or fabrication module exists in `src/simulation/include/everward/simulation/`. | `docs/PHASE2_VERTICAL_SLICE_PLAN.md` ("Slice 13"); directory listing |
| Persistence | Implemented. Deterministic save/load including inventory and knowledge. | `save_data.hpp`, `save_data_tests.cpp`, `docs/PHASE2_SAVE_LOAD_UI_TEST.md` |
| Objective / completion loop | Not implemented as runtime state. See below. | `self_repair.hpp:130`; `docs/STARTER_AWAKENING_AND_SELF_REPAIR.md` |

### Loop coverage

The design loop is `WAKE -> DIAGNOSE -> REACH -> GATHER -> SPEND -> REPAIR -> USE -> ... -> RESTORE THE WHOLE PROBE -> DEPART` (`docs/GAMEPLAY_LOOP.md`, "Canonical awakening loop"). At the baseline, source and tests cover the path from wake through gather, spend and repair. The terminal steps (restore the whole probe, depart) have no runtime representation.

## What is missing

1. **Authoritative starter-restoration objective state.** `SelfRepairPlanner::is_fully_restored()` (`self_repair.hpp:131`) expresses the departure gate, but its only caller is `src/simulation/tests/self_repair_tests.cpp`. A search of `src/simulation/include` and `unreal/Source` for objective/mission/departure/exit-gate state finds only the comment at `self_repair.hpp:130`. Nothing in the tick, adapter, HUD, save data or playtest events reports whether the loop's goal is met.
2. **Fabrication (Slice 13).** There is no processing or fabrication module.
3. **Material-specific consumption policy.** `Fix_It` still uses the generic consume overload. `docs/PROJECT_STATUS.md` ("Named-material atomic consumption boundary") records this as out of scope until a component-to-material policy is authorized.
4. **Planner duplication.** `SelfRepairPlanner` is not wired to the runtime, while `Fix_It` (`fix_it.hpp`) is.
5. **Local Unreal Product Reality** for nearly every item above is pending (`docs/PROJECT_STATUS.md`). This is a verification gap, not a code dependency.

## Smallest missing loop dependency (exactly one)

**A read-only, engine-independent, authoritative "starter restoration objective status" derived from the probe's existing component-integrity state.**

It should report which required subsystems are still below 100% (by the `is_fully_restored` rule), and whether the objective is satisfied. It should be exposed through the adapter in the same accessor pattern as `GetSelectedTargetStatus()`.

Why it precedes the others:

- It needs no new mechanic. Every input already exists: `component_integrity()` (`core.hpp`) and `is_fully_restored` (`self_repair.hpp`). The other candidates need new design or authorization (fabrication, the material policy) or a tooling or hardware pass (Product Reality).
- It is what makes the existing mechanics a loop instead of disconnected subsystems, which is the roadmap's stated requirement ("one coherent playable gameplay loop ... not merely disconnected subsystems"). Without a defined, queryable goal, neither a player nor a Product Reality run can tell when the loop has been completed.
- Product Reality verification of the full loop (roadmap Next slice 3) cannot be specified without a stated, observable completion condition.
- Fabrication and the material policy change what the player can do. This dependency only defines when the loop is done, so it can be closed independently and reversibly.

Open points for the implementing slice, not decided here: whether power and thermal stability (`STARTER_AWAKENING_AND_SELF_REPAIR.md`'s exit condition also mentions them) are part of the first objective, and whether `Fix_It`'s integrity ceiling can reach 100% for every subsystem. Neither was verified in this inventory.

## Limits of this document

- Static reading of source, tests and docs only. No Unreal build, playtest, or in-engine run was performed.
- Test names are cited as present, not re-run for this document; the canonical preflight is run for the PR.
- "Repeatable" (the roadmap's wording for the objective) is unaddressed: no repeat mechanic exists, and the inventory does not assume one.

## Slice delivered: starter restoration objective status

Traceability: `engineering/capability-graph.json#phase2-one-probe-development-access/first-loop-integration-slice`.

- Simulation truth: `src/simulation/include/everward/simulation/starter_restoration.hpp` (`evaluate_starter_restoration`), read-only, derived from component integrity with `SelfRepairPlanner::is_fully_restored`.
- Presentation mirror: `UProbeSimulationAdapter::GetStarterRestorationStatus()` (`ProbeTargetSelectionBridge.cpp`), no gameplay authority.
- Portable tests: `src/simulation/tests/starter_restoration_tests.cpp`.
- Scope decision: the objective covers the four integrity-tracked subsystems only; power/thermal stability and "repeatable" remain open.
- **Pending (not verified):** Unreal compile of the adapter accessor and any in-engine/HUD use. No UBT was available on the runner.
