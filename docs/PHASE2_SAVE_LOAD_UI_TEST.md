# Phase 2 — Save/Load UI Wiring Product Reality Test

## Scope

`docs/PROJECT_STATUS.md`'s "Deterministic save/load v1" section and
`docs/SAVE_FORMAT.md`'s originating issue (#167) both named the same
outstanding gap after `save_data.hpp` landed: the engine-independent
capture/serialize/restore round trip was ctest-verified, but "no
Unreal-side save/load UI, file I/O, or `SaveGame` wiring exists" -- there
was no way for a player to actually save or load inside the game. This
pass closes exactly that gap and nothing else:

- `UProbeSimulationAdapter::CommandSaveGame()` captures the live probe via
  `save_data.hpp`'s existing `capture_probe_save_data()`, wraps it in a
  single-probe `SaveGameV1`, and writes the serialized JSON to
  `Saved/SaveGames/everward_save_v1.json` via `FFileHelper::SaveStringToFile`;
- `UProbeSimulationAdapter::CommandLoadGame()` reads that same file,
  deserializes it, and rebuilds `Core`/`Manipulators` through the existing
  `restore_probe_runtime()`/`restore_manipulator_rig()` factories -- the
  exact functions `everward_save_data_tests` already exercises -- before
  swapping them in. The new runtime/rig are fully constructed (which
  re-runs every validation `save_data.hpp`'s restore path performs) before
  anything currently live is deleted, so a rejected load (missing file,
  unsupported `save_version`, malformed JSON, an internally inconsistent
  or unreachable-pose snapshot) leaves the in-progress probe completely
  untouched rather than partially replaced;
- `AEverwardPlayerController` binds `F5` (save) and `F6` (load) -- `F9` is
  deliberately avoided, matching `PlaytestRecorderActor.cpp`'s own existing
  note that it conflicts with an Unreal Editor wireframe shortcut in PIE;
- both commands report through the existing `RecordCommandResult`/
  `GetLastCommandResult()` global-feedback path every other command already
  uses, so no new HUD code was needed for confirmation text;
- the `F1` controls reference's SYSTEMS column gains `F5 SAVE PROBE STATE`
  / `F6 LOAD PROBE STATE` rows.

No new save-data field, schema change, or migration behavior is
introduced -- this only wires the already-implemented and already-tested
schema to an actual file and a player-facing command. `ProbeSaveData`'s own
header comment continues to document exactly what is and is not
represented (the single canonical probe's physical/energy/thermal/storage/
scan/power state, component integrity, registered targets, installed
policy, target selection, and manipulator arm state; no lineages,
infrastructure, or other later categories, since those systems do not
exist in the simulation yet).

## Behavior

- `F5` with a live probe writes a save file and reports "probe state
  saved" via the command banner.
- `F6` with a previously written save file restores position, velocity,
  attitude, energy/thermal state, storage, component integrity, registered
  targets (including any that were moved by a still-held manipulator
  grasp), installed policy, target selection, and both manipulator arms'
  deploy/joint/tool/grasp state, then reports "probe state loaded".
- `F6` with no save file yet written reports "no save file found" and
  changes nothing.
- A hand-edited or corrupted save file (unsupported `save_version`,
  missing/malformed field, internally inconsistent state) is rejected with
  the specific error message and changes nothing about the currently live
  probe.

## CI-verifiable acceptance

The underlying capture/serialize/restore round trip already has full ctest
coverage (`everward_save_data_tests`, unchanged by this pass). This pass is
Unreal adapter/input/HUD wiring only -- no new engine-independent behavior
was added, so no new `src/simulation` test is needed. No Unreal Editor/UBT
build was available in this sandbox to compile-verify
`ProbeSimulationAdapter.h`/`.cpp`, `EverwardPlayerController.h`/`.cpp`, or
`EverwardHUD.cpp`; the changes follow the exact `FFileHelper`/`FPaths`/
`IFileManager` file-I/O pattern already compiling in
`PlaytestRecorderActor.cpp`, and the exact `RecordCommandResult`/
`BindKey`/`GetProbeAdapter()` command, input-binding, and feedback patterns
already compiling elsewhere in `ProbeSimulationAdapter.cpp` and
`EverwardPlayerController.cpp`. The next local Unreal Product Reality pass
should specifically confirm the project still compiles under UBT before
relying on this further.

## Local Unreal Product Reality acceptance

1. Launch the exact CI-green build and enter PIE.
2. Fly the probe to a distinct position/attitude, allocate power
   differently than default, start a scan, deploy an arm and move a joint
   off its default angle, then press `F5`. Confirm the command banner
   reads "probe state saved" and a `everward_save_v1.json` file appears
   under the project's `Saved/SaveGames/` directory.
3. Continue playing so state changes further (move away, cancel the scan,
   stow the arm), then press `F6`. Confirm the probe's position, attitude,
   power allocation, and manipulator state visibly snap back to what was
   saved in step 2, and the command banner reads "probe state loaded".
4. Grasp the registered physical target, articulate the arm so the target
   visibly moves, save (`F5`), release or move further, then load (`F6`)
   and confirm the target mesh reappears at its saved (moved) position
   rather than its original registered position.
5. Delete or rename the save file outside the game, press `F6`, and
   confirm a clear "no save file found" rejection rather than a crash or
   silent no-op.
6. Hand-edit the saved JSON file to an invalid value (e.g. an
   out-of-range `save_version`), press `F6`, and confirm a clear rejection
   message naming the problem, with the currently live probe state
   unchanged.
7. Confirm the `F1` controls reference now lists the `F5`/`F6` bindings
   under SYSTEMS.
8. Record any discrepancy (silent failure, a crash, a load that partially
   applies before failing, stale telemetry after a rejected load, or a
   build/compile failure) as Product Reality evidence.

## Explicitly not complete in this pass

- Single save slot only; no save-slot list, naming, or overwrite
  confirmation UI.
- No autosave, and no separate diegetic in-world backup system (per
  `docs/SAVE_FORMAT.md`'s "Autosave and rollback" section, those remain
  distinct, later concerns).
- No schema migration framework beyond rejecting a non-`v1`
  `save_version`, since only `v1` exists so far.
- No lineages, infrastructure, resource stores, discoveries, or any other
  top-level category `docs/SAVE_FORMAT.md` anticipates, since those
  systems do not exist in the simulation yet.
- Does not itself advance Slice 7 or any other vertical-slice completion
  gate; it closes the Unreal-side half of issue #167's persistence gap.

## Status

Implemented in the parallel-safe lane; Product Reality pending.
