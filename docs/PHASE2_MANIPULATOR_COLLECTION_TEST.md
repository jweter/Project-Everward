# Phase 2 — Manipulator Sample Collection Product Reality Test

## Scope

`PHASE2_VERTICAL_SLICE_PLAN.md`'s Slice 12 ("resource/sample loop") initial
scope lists two items no prior pass implemented: "sampleable object/material"
and "manipulator/tool acquisition" of a sampled object, as opposed to
`mining.hpp`'s repeated-cycle tool-beam extraction from a deposit
(`CommandMineBootstrapTarget`). A sample is a bounded object collected once,
in full, the moment it is grasped and within reach -- there is no partial
extraction, survey gate, or extraction-per-cycle concept, unlike mining.

This pass adds exactly that minimum interaction:

- `StaticSphereBody` (`types.hpp`) gains `sample_mass_kg`, the ground-truth
  mass a body contributes to authoritative storage if collected whole. Zero
  (default) means "not manipulator-collectible" -- every pre-existing
  reference/deposit body is unaffected;
- `ProbeRuntime::remove_static_sphere_body()` (`software_policy.hpp`,
  forwarded through `DamageAwareProbeRuntime`) is the sole deregistration
  mutation point, mirroring `add_static_sphere_body()`'s sole registration
  boundary;
- `manipulator_collection.hpp` (new) adds `attempt_collect_grasped_target()`,
  the same kind of gated wrapper `manipulator_grasp.hpp`/
  `manipulator_release.hpp` already established over `ManipulatorRig`: it
  fails closed (no mutation) whenever nothing is held, the held body is no
  longer registered, or the held body is not manipulator-collectible
  (`sample_mass_kg <= 0`). Only once eligible does it release the grasp and
  report what to credit -- it deliberately does **not** itself deregister the
  body or credit storage, the same division of responsibility the "move"
  sub-slice already established between `grasped_target_position()` (a
  read) and `update_static_sphere_body_position()` (the runtime's own
  mutation);
- `UProbeSimulationAdapter::CommandCollectGraspedTarget()` composes the two:
  on success it calls `Core->remove_static_sphere_body()` and
  `Core->add_stored_material_kg()`, the same storage boundary
  `CommandMineBootstrapTarget` already uses;
- `X` (bound alongside `F`'s grasp/release) triggers collection on whichever
  arm the manipulator HUD page currently has selected;
- the Phase-2 test environment gains a fourth registered body, `SAMPLE-001`
  (`phase2-test-target-004`), distinct from the mining deposit and the two
  plain reference bodies, with a positive `sample_mass_kg` and its own
  `carbonaceous_chondrite_fragment` material identity. Its mesh/label mirror
  the held body's live position while grasped (exactly like `SCAN-001`
  already does) and hide exactly once the body is actually collected and
  deregistered, rather than leaving a ghost mesh at its last position.

No change to mining, grasp's own reach gate, release's hull/other-body gate,
target selection/cycling, contact, or damage behavior.

## Behavior

- Approaching `SAMPLE-001`, selecting it (`T`), and grasping it (`F`) behaves
  exactly as any other registered body already does (`PHASE2_MANIPULATOR_GRASP_TEST.md`).
- Collecting (`X`) while holding `SAMPLE-001` in reach succeeds: `HOLDING`
  clears, the mesh/label disappear from the world, global feedback reports
  the collected mass and material, and the always-visible `INVENTORY` row
  (`PHASE2_MATERIAL_INVENTORY_TEST.md`) gains or grows a
  `carbonaceous_chondrite_fragment` entry by the collected mass.
- Collecting (`X`) while holding nothing, or while holding a body with no
  `sample_mass_kg` (e.g. `SCAN-001` or a plain `REF-***` reference body), is
  rejected: global feedback reports "arm has nothing collectible to stow",
  and (for a genuine grasp) the arm keeps `HOLDING` its target unchanged.
- No change to deploy/stow, joint articulation, tool attach/detach, release's
  own hull/other-body gate, target selection/cycling, contact, or damage
  behavior.

## CI-verifiable acceptance

- `src/simulation/tests/manipulator_collection_tests.cpp`
  (`everward_manipulator_collection_tests` in `src/simulation/CMakeLists.txt`)
  covers: nothing held fails closed; a since-deregistered held body fails
  closed; a held body with `sample_mass_kg == 0.0` fails closed without
  clearing the grasp; a genuine sample succeeds, releases the grasp, and
  reports the correct body/material/mass while leaving the registered-body
  list itself untouched (this module's own contract); the gate stays scoped
  to the queried arm; and the runtime overload matches the free function
  while confirming it does not itself touch storage. All 30
  `src/simulation` CTest suites (including this one) were run locally and
  pass.
- `save_data_tests.cpp` gained `sample_mass_kg` round-trip coverage in the
  existing full-state round trip plus a legacy-save inference case (absent
  field reads back as `0.0`, i.e. "not collectible", matching every
  pre-existing body).
- `tools/test_phase2_manipulator_collection_surface.py` (new) proves the
  math is engine-independent and fails closed, that the collection module
  itself performs neither the registry removal nor the storage credit, that
  the adapter composes both after a successful gate, and that the player
  controller/HUD/environment wiring is actually present. The pre-existing
  `tools/test_phase2_target_selection_surface.py` registered-body count
  assertion was updated from 3 to 4 to match the new fourth body.
  `python3 -m unittest discover -s tools -p "test_phase2*.py"` and
  `python3 tools/quality_preflight.py --full` both pass at this exact head.

No Unreal Editor/UBT build was available in this sandbox to compile-verify
`ProbeSimulationAdapter.h`/`.cpp`, `EverwardPlayerController.h`/`.cpp`,
`EverwardHUD.cpp`, or `EverwardPhase2TestEnvironment.h`/`.cpp`. Each change
follows an exact pattern that already compiles in the same file
(`CommandReleaseGraspedTarget`'s structure for the new command,
`ToggleManipulatorGrasp`'s arm-resolution for the new controller method,
`RefreshScanTargetPosition`'s fail-closed position mirroring for the new
environment refresh, and the existing `ReferenceTargetSpawns`/
`add_static_sphere_body` aggregate-init calls for the new registered body).
The next local Unreal Product Reality pass should specifically confirm the
project still compiles under UBT before relying on this further.

## Local Unreal Product Reality acceptance

1. Launch the exact CI-green build and enter PIE.
2. Confirm `SAMPLE-001` is visible near the existing `SCAN-001`/`REF-002`/
   `REF-003` bodies, with its own distinct label and a visually distinct
   (darker, less reflective) material from the regolith-rock bodies.
3. Deploy an arm, select `SAMPLE-001` (`T`, cycling with the other bodies if
   necessary), approach until `REACH` reads "IN REACH", and grasp it (`F`).
   Confirm `HOLDING SAMPLE-001` (or its registered id) appears exactly as
   `PHASE2_MANIPULATOR_GRASP_TEST.md` documents for any other body.
4. Articulate the arm (`M`, `,`/`.`) and confirm the mesh/label visibly
   follow the wrist while held, exactly as `PHASE2_MANIPULATOR_MOVE_TEST.md`
   documents for `SCAN-001`.
5. Collect it (`X`). Confirm: `HOLDING` clears; the mesh and label disappear
   from the world entirely (not merely retinted or left stationary); global
   feedback reports the collected mass and `carbonaceous_chondrite_fragment`;
   and the `INVENTORY` row gains/grows a matching entry.
6. Attempt to select or interact with the now-collected body's id again (`T`
   cycling should simply skip it). Confirm no stale selection, ghost mesh, or
   crash results.
7. Grasp `SCAN-001` (the mining deposit) or one of the plain `REF-***`
   bodies instead and attempt to collect it (`X`). Confirm this is rejected
   ("arm has nothing collectible to stow") and the arm keeps holding the
   target, and that mining (`G`) and release (`F`) on those bodies are
   unaffected.
8. Confirm this pass has not changed deploy/stow, joint articulation, tool
   attach/detach, target selection/cycling, contact, or damage behavior.
9. Record any discrepancy (mesh/label surviving collection, storage/
   inventory not updating, a non-sample body being collectible, a stale
   `HOLDING` state, or a build/compile failure) as Product Reality evidence.

## Explicitly not complete in this pass

- Only one sampleable body exists in the test scene; a richer variety of
  sample types/materials is future scope.
- Material-specific consume/use still does not exist -- repair/Fix_It
  continues to draw generically from whatever is in storage regardless of
  provenance (see `PHASE2_MATERIAL_INVENTORY_TEST.md`'s own gap list).
- No velocity/momentum, particle effect, or other presentation flourish on
  collection beyond the mesh/label disappearing and the HUD updating.
- Slice 12's completion gate remains the local Product Reality pass recorded
  in `PROJECT_STATUS.md`; this closes the "sampleable object/material" and
  "manipulator/tool acquisition" gaps but does not by itself close the slice.

## Status

Implemented in the parallel-safe lane; Product Reality pending. Does not by
itself advance Slice 12's completion gate.
