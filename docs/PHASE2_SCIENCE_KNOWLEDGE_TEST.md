# Phase 2 — Science Knowledge Foundation Product Reality Test

## Scope

`PHASE2_VERTICAL_SLICE_PLAN.md`'s Slice 11 ("Science as gameplay") calls for
scanning to evolve from a countdown into increasing knowledge: unknown ->
passive observation -> active scan -> classification -> composition
estimates -> confidence/uncertainty -> persistent discoveries. Issue #215
landed `science_knowledge.hpp` as a standalone, ctest-covered,
engine-independent module (`TargetKnowledgeState`, `apply_observation()`),
but until this pass nothing outside its own tests ever called it — not
`SimulationCore`, not the Unreal adapter, not save/load.

This pass wires that foundation into the existing scan lifecycle. A later
pass (see "Target composition/classification reveal" below) closed the
"no classification is ever produced" gap this section originally described;
the wiring chain below is otherwise unchanged:

`SimulationCore::integrate_scan()` (existing, unchanged trigger) ->
observe_active_scan_progress() (new, in core.hpp) ->
apply_observation() (existing, unchanged, from science_knowledge.hpp) ->
ProbeStateSnapshot::target_knowledge (new, additive save field) ->
ProbeRuntime/DamageAwareProbeRuntime::target_knowledge_state() (new
forwarding accessors) -> UProbeSimulationAdapter::GetSelectedTargetKnowledgeStatus()
(new) -> AEverwardHUD's always-visible telemetry panel (new KNOWLEDGE row)`

Every fixed tick an active scan is actually progressing (the existing
`is_scanning && can_scan` guard `integrate_scan()` already applies), the
scanned target's `TargetKnowledgeState` accumulates real elapsed
observation time as an `ActiveScan` observation. Confidence gain is
`seconds / kNominalActiveScanConfidenceTimeConstantS` (10.0 s, matching
`EverwardPlayerController.h`'s default `Phase2ScanDurationSeconds`), so one
uninterrupted nominal-length scan brings a target's confidence to full
(1.0). A sensor-damage-elongated scan (`DamageAwareProbeRuntime::start_scan`
already stretches `duration_s` by `1 / effectiveness`) still reaches the
same confidence — it simply takes longer wall-clock time to do so, since
duration is the only channel scan damage already degrades; this pass does
not add a second, competing degradation model. Passive observation
(`ObservationMode::Passive`) is supported by `science_knowledge.hpp` but has
no trigger anywhere in the simulation yet.

This qualifies for the parallel-safe lane the same way target selection and
manipulator reach telemetry did: it reads/mutates only its own new state
behind the existing scan-lifecycle boundary, introduces no new player
command, and does not assume any still-pending contact/collision/damage
Product Reality is correct.

## Target composition/classification reveal

A follow-on pass closes the "no classification or composition estimate is
ever produced" gap this document originally described, keeping the exact
same layering discipline `SimulationCore` never fabricating a reading on
its own:

- `StaticSphereBody` (`types.hpp`) gains an optional `material_id`, the
  registered body's own ground-truth composition (empty for a plain
  reference target with no known composition, e.g. `REF-002`/`REF-003`);
  round-tripped through save/load as an additive field the same way
  `target_knowledge`/`material_inventory_kg` already are;
- `SimulationCore::set_target_classification(target_id, material_id)` is
  the sole new mutation point that can move a target from `Observed` to
  `Characterized`. It fails closed (no mutation) for a target never
  observed or an empty `material_id` — `observe_active_scan_progress()`
  itself is completely unchanged and still never supplies
  `ObservationEvidence::classification` on its own;
- `ProbeRuntime::reveal_full_confidence_target_classifications()` (called
  from `advance_wall_ticks()`, after `evaluate_policy()`) is the one place
  that actually has both pieces of information `SimulationCore` lacks: the
  registered `static_bodies_` list and its `material_id`s. Once a
  registered body's `target_knowledge_state()` confidence has reached full
  (1.0) and it is not already `Characterized`, it calls
  `set_target_classification()` with that body's `material_id`;
- the Unreal-side `SCAN-001` bootstrap target is registered with
  `material_id = "iron_bearing_silicate_regolith"` — the exact same literal
  `ProbeMiningBridge.cpp`'s `MakeBootstrapDeposit()` already uses — so a
  fully-confident science scan reports the same composition mining later
  actually extracts, rather than a second invented reading. The two
  additional reference targets remain composition-less by design;
- `FEverwardTargetKnowledgeStatus::Classification` (already declared, unused
  before this pass) is populated exactly as before by
  `GetSelectedTargetKnowledgeStatus()` — no adapter change was needed there
  — and the `KNOWLEDGE` row now shows it once `Characterized`:
  `KNOWLEDGE  CHARACTERIZED // iron_bearing_silicate_regolith` in place of a
  now-constant, uninformative 100% confidence readout.

## Persistent discoveries catalogue

A follow-on pass closes the "no persistent discoveries list" gap this
document's "Explicitly not complete" section originally named. The
`KNOWLEDGE` row above only ever reports whichever single target the
`TARGET` row currently has selected, so knowledge accumulated about a
target became unreadable the moment it was deselected — even though
`SimulationCore`'s own `target_knowledge` map already keeps every entry for
the life of the save (it is never cleared when a target is deselected, or
even when its registered `StaticSphereBody` is later removed by mining-out
or manipulator collection). This pass exposes that already-persistent data
as a read-only catalogue rather than inventing a second store:

- `UProbeSimulationAdapter::GetDiscoveredTargets()` (new
  `ProbeTargetSelectionBridge.cpp` accessor, alongside
  `GetSelectedTargetKnowledgeStatus()`) iterates `Core->target_knowledge()`
  directly and returns one `FEverwardDiscoveredTarget` (target id, level,
  confidence, classification) per entry whose level is not `Unknown` —
  registered-but-never-observed targets are not discoveries yet and are
  skipped rather than padding the list with a fabricated reading. The
  underlying map already iterates in ascending target_id order, so the
  result is deterministic without an explicit sort;
- the always-visible telemetry panel gains a `DISCOVERIES` row directly
  below `INVENTORY`, reading a muted "DISCOVERIES NONE YET" prompt with
  nothing yet observed, or a compact catalogue such as `SCAN-001 //
  iron_bearing_silicate_regolith, REF-002 // 40%` once one or more targets
  have been observed;
- no new authoritative state, save field, mutation point, or player command
  was added — `target_knowledge` itself is unchanged, and this reads the
  exact same map `GetSelectedTargetKnowledgeStatus()` already reads for the
  single currently-selected target.

## Behavior

- The always-visible telemetry panel gains a `KNOWLEDGE` row directly below
  the existing manipulator arm status lines, reusing whichever target the
  `TARGET` row already reports selected — no second "which target" concept.
- With no target selected, the row reads a muted "KNOWLEDGE NO TARGET
  SELECTED" prompt.
- With a target selected but never yet scanned, the row reads a muted
  "KNOWLEDGE NOT YET OBSERVED" prompt rather than fabricating a 0%
  reading.
- Once an active scan on the selected target has progressed at least one
  fixed tick, the row reads live, e.g. "KNOWLEDGE OBSERVED // 12%
  CONFIDENCE", climbing toward 100% as scanning continues and persisting
  (not resetting) across scan cancellation/restart or target
  reselection/deselection and reselection.
- Once that scan's confidence actually reaches 100% on a registered body
  with a known composition (currently only `SCAN-001`), the row switches to
  "KNOWLEDGE CHARACTERIZED // iron_bearing_silicate_regolith" instead of a
  now-constant 100% confidence readout, and stays that way afterward. A
  plain reference target with no known composition (`REF-002`/`REF-003`)
  still only ever reaches "OBSERVED // 100% CONFIDENCE", never
  "CHARACTERIZED", even at full confidence.
- No new input binding was added or is required: the row only extends the
  existing always-visible panel and target-selection/scan surfaces.

## CI-verifiable acceptance

- `src/simulation/tests/simulation_core_tests.cpp` covers
  `observe_active_scan_progress()` directly: no knowledge exists for a
  target before it is ever scanned; a half-completed scan records partial
  `active_scan_s`/`confidence` at `Observed` level with no classification;
  completing the scan accumulates the remainder; an unrelated target's
  knowledge is untouched. It also covers `set_target_classification()`
  directly: a no-op on a never-observed target or an empty `material_id`,
  and moving an already-observed target to `Characterized` otherwise,
  leaving unrelated targets untouched.
- `src/simulation/tests/software_policy_tests.cpp` covers
  `reveal_full_confidence_target_classifications()` end to end through
  `ProbeRuntime`: a registered body with a `material_id`, scanned to full
  confidence, is `Characterized` with that exact material; a registered
  body with no `material_id`, scanned to the same full confidence, stays
  `Observed`; and a scan target that is not a registered body at all is
  likewise never fabricated a classification.
- `src/simulation/tests/save_data_tests.cpp` covers round-tripping
  `target_knowledge` byte-for-byte through save/load (exercised
  incidentally by `build_representative_runtime()`'s existing
  `start_scan`/`advance_wall_ticks` calls), `restore_from_snapshot()`
  rejecting a mismatched key/target_id pair and an out-of-range confidence,
  a legacy save captured before this field existed inferring an empty map
  rather than throwing, a registered body's `material_id` round-tripping
  through save/load, and a legacy static body JSON object missing the
  `material_id` key inferring "no known composition" rather than throwing.
- `tools/test_phase2_science_knowledge_surface.py` confirms the wiring
  chain end to end: `ProbeStateSnapshot` owns `target_knowledge`;
  `SimulationCore` accumulates it only through the scan lifecycle and
  exposes fail-closed read accessors; `ProbeRuntime`/`DamageAwareProbeRuntime`
  forward without duplicating state; `save_data.hpp` persists it (and the
  new `material_id`) as additive fields; the Unreal adapter exposes a
  read-only status struct reusing the existing target-selection result;
  `ProbeSimulationAdapter.cpp` registers the bootstrap target's real mining
  material rather than a second invented one; and the HUD renders the new
  row (and, since this pass, the classification once `Characterized`) by
  extending the telemetry panel's height rather than overlapping the rows
  already below it. It also confirms `GetDiscoveredTargets()` is wired into
  the adapter/HUD (the `DISCOVERIES` row, the panel height's further bump to
  12 lines, and that never-observed entries are skipped) rather than merely
  present alongside them.

No Unreal Editor/UBT build was available in this sandbox to compile-verify
`ProbeSimulationAdapter.h`/`.cpp` or `EverwardHUD.cpp`. The adapter change
follows the exact accessor pattern `GetSelectedTargetStatus()` already uses
and compiles elsewhere in that file (read `Core`, recompute live, return a
plain `USTRUCT`); the HUD change follows the exact panel-row pattern the
existing TARGET/SIM/arm rows already use, extending `TelemetryHeight` from
9 to 10 lines rather than renumbering any existing row's fixed offset. The
next local Unreal pass should specifically confirm the project still
compiles under UBT and that the extra row does not clip against the
panel's background or the manipulator page drawn above it.

## Local Unreal Product Reality acceptance

1. Launch the exact CI-green build and enter PIE.
2. Confirm the always-visible telemetry panel's new `KNOWLEDGE` row reads
   "NO TARGET SELECTED" with nothing selected, and that the panel's
   background still fully contains every row down through `KNOWLEDGE` with
   no clipping or overlap against the manipulator arm rows above it.
3. Press `T` to select the registered physical target. Confirm the row
   switches to "NOT YET OBSERVED".
4. Start a scan (`G`'s existing scan-start path or whatever binding this
   build uses) on the selected target and confirm the row switches to a
   live "OBSERVED // *X*% CONFIDENCE" reading that climbs smoothly while
   scanning continues.
5. Cancel the scan partway through, confirm the confidence reading is
   retained rather than resetting to zero, then restart the scan and
   confirm it continues climbing from where it left off.
6. Deselect the target (retreat past selection range, then `T`) and
   reselect it; confirm the row still reports the previously accumulated
   confidence rather than a fresh "NOT YET OBSERVED" prompt.
7. Continue scanning `SCAN-001` until confidence reaches 100% and confirm
   the row switches to "KNOWLEDGE CHARACTERIZED //
   iron_bearing_silicate_regolith" and stays that way even after cancelling
   the scan, deselecting, or reselecting the target.
8. Cycle (`T`) to one of the two plain reference targets, scan it to 100%
   confidence the same way, and confirm it reads "KNOWLEDGE OBSERVED //
   100% CONFIDENCE" — never "CHARACTERIZED" — since it has no known
   composition.
9. Confirm this pass has not changed scan start/cancel/complete, target
   selection/cycling, manipulator, mining, contact, or damage behavior, and
   that mining `SCAN-001` still reports the same
   `iron_bearing_silicate_regolith` material id the new KNOWLEDGE row does.
10. With `SCAN-001` already characterized (step 7) and a reference target
    already scanned to 100% confidence (step 8), confirm the panel's new
    `DISCOVERIES` row (directly below `INVENTORY`) lists both, and that the
    panel background still fully contains every row down through
    `DISCOVERIES` with no clipping or overlap against the manipulator page
    drawn above it. Deselect all targets (retreat past selection range) and
    confirm the row still reports both entries rather than clearing when
    nothing is selected — this is the specific gap it closes versus
    `KNOWLEDGE` above.
11. Record any discrepancy (row overlapping other panel content, confidence
    resetting unexpectedly, a stale reading after reselection, a reference
    target wrongly characterizing, a mismatched material id between mining
    and science, a discoveries entry disappearing on deselection, or a
    build/compile failure) as Product Reality evidence.

## Explicitly not complete in this pass

- Only one registered body (`SCAN-001`) has a known `material_id` today; a
  real composition/material *estimate* (uncertainty, partial/incorrect
  readings, multiple possible materials) does not exist — classification is
  a single deterministic ground-truth reveal gated on full confidence, not
  a modeled estimation process.
- No passive-observation trigger exists anywhere in the simulation yet,
  though `science_knowledge.hpp` already supports the mode.
- A persistent discoveries catalogue (the `DISCOVERIES` row) now exists, but
  it is still read-only telemetry: no codex UI, no decision-enabling
  gameplay consequence, and no dedicated discoveries HUD page — only the
  single compact row on the existing always-visible panel, truncated the
  same way the `INVENTORY` row already is once the catalogue grows long.
- Instrument-dependent resolution beyond a fixed nominal baseline
  (`kNominalInstrumentResolution = 1.0`) is not modeled.

## Status

Implemented in the parallel-safe lane; Product Reality pending. Does not by
itself close Slice 11.
