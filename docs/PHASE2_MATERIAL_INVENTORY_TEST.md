# Phase 2 — Material Inventory Readout Product Reality Test

## Scope

`PHASE2_VERTICAL_SLICE_PLAN.md`'s Slice 12 ("resource/sample loop") lists
"item/material identity and provenance" as required scope. The "Per-material
storage identity" foundation (`PROJECT_STATUS.md`) already added an
authoritative `material_inventory_kg` breakdown of `storage_used_kg` by
`material_id`, credited by mining and depleted deterministically by generic
consumption, and round-tripped through save/load -- but nothing outside its
own ctest coverage ever read it back: the always-visible telemetry panel's
`STORAGE` row only ever showed the single aggregate kilogram figure, with no
way to tell *what* is actually stored.

This pass wires that existing breakdown into a read-only HUD row and nothing
further:

`SimulationCore::material_inventory_kg()` (existing, unchanged) ->
`DamageAwareProbeRuntime::material_inventory_kg()` (existing, unchanged
forwarding accessor) -> `UProbeSimulationAdapter::GetStoredMaterialInventory()`
(new, `ProbeMiningBridge.cpp`) -> `AEverwardHUD`'s always-visible telemetry
panel (new `INVENTORY` row, directly below the existing `KNOWLEDGE` row)

No new authoritative state, mutation point, save field, or player command is
introduced. This qualifies for the parallel-safe lane the same way the
`KNOWLEDGE` row did: it only reads already-existing, already-ctest-covered
Core state and extends the telemetry panel's height rather than assuming any
still-pending contact/collision/damage Product Reality is correct.

## Behavior

- The always-visible telemetry panel gains an `INVENTORY` row directly below
  the existing `KNOWLEDGE` row.
- With nothing stored, the row reads a muted "INVENTORY EMPTY" prompt rather
  than a blank line or a fabricated zero-mass entry.
- Once mining has credited at least one material, the row lists each stored
  `material_id` and its kilograms (e.g. "INVENTORY
  iron_bearing_silicate_regolith 5.0 KG"), in the same deterministic
  ascending-`material_id` order `material_inventory_kg()` already iterates.
- A row that would overflow the panel's width is truncated with an ellipsis
  (`TruncatedPanelLine`, the same helper the `MANIPULATOR` page's status
  lines already use) rather than wrapping or clipping into the panel edge.
- No new input binding was added or is required: the row only extends the
  existing always-visible panel.

## CI-verifiable acceptance

- `tools/test_phase2_material_inventory_surface.py` confirms the wiring
  chain end to end: `SimulationCore` already owns the authoritative
  breakdown (unchanged by this pass); the Unreal adapter declares a
  read-only `FEverwardMaterialInventoryEntry` struct and
  `GetStoredMaterialInventory()` accessor; the bridge implementation reads
  `Core->material_inventory_kg()` directly (no second breakdown) and fails
  closed to an empty array with no live simulation core; and the HUD draws
  the new row by extending `TelemetryHeight` from 10 to 11 lines rather than
  overlapping the manipulator panel drawn above it.
- The underlying breakdown itself remains covered by
  `src/simulation/tests/simulation_core_tests.cpp` (tracking, accumulation,
  deterministic ascending-order depletion) and
  `src/simulation/tests/save_data_tests.cpp` (round trip, legacy-save
  inference) -- unchanged by this pass.

No Unreal Editor/UBT build was available in this sandbox to compile-verify
`ProbeSimulationAdapter.h`/`ProbeMiningBridge.cpp`/`EverwardHUD.cpp`. The
adapter change follows the exact accessor pattern
`GetSelectedTargetKnowledgeStatus()` already uses and compiles elsewhere
(read `Core`, recompute live, return a plain `USTRUCT`/`TArray`); the HUD
change follows the exact panel-row extension pattern the `KNOWLEDGE` row
already established. The next local Unreal Product Reality pass should
specifically confirm the project still compiles under UBT and that the new
row does not clip against the panel's background or the manipulator page
drawn above it.

## Local Unreal Product Reality acceptance

1. Launch the exact CI-green build and enter PIE.
2. Confirm the always-visible telemetry panel's new `INVENTORY` row reads
   "EMPTY" before anything has been mined, and that the panel's background
   still fully contains every row down through `INVENTORY` with no clipping
   or overlap against the manipulator page drawn above it.
3. Mine the registered physical target (`G`) until at least one extraction
   cycle completes; confirm the row switches to a live reading showing the
   mined material's id and kilograms, matching the mining status widget's
   own recovered-mass figure and the `STORAGE` row's aggregate kilograms.
4. Confirm this pass has not changed scan, target selection/cycling,
   manipulator, mining, contact, or damage behavior, and that the
   `KNOWLEDGE` row immediately above still renders correctly.
5. Record any discrepancy (row overlapping other panel content, a stale or
   mismatched reading, or a build/compile failure) as Product Reality
   evidence.

## Explicitly not complete in this pass

- No inventory management (dropping, transferring, or selling material) --
  this is read-only telemetry over an existing authoritative number.
- No material-specific repair/Fix_It consumption; `consume_stored_material_kg()`
  still depletes in deterministic ascending-`material_id` order rather than
  Fix_It selecting a preferred material. It now returns which material_id(s)
  it actually depleted and how many kilograms came from each, so Fix_It's
  `FixItExecutionStatus::material_consumed_breakdown_kg` and the in-editor
  Fix_It status messages report which stored material a repair/replacement
  drew from -- see `PHASE2_FIX_IT_PLAYABLE_TEST.md`.
- No dedicated inventory HUD page; only the single compact `INVENTORY` row
  on the existing always-visible panel, matching a long list of materials
  only up to the panel's fixed line width before truncating.

## Status

Implemented in the parallel-safe lane; Product Reality pending. Does not by
itself close Slice 12.
