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

This pass wires that foundation into the existing scan lifecycle and
nothing further:

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
not add a second, competing degradation model. No classification or
composition estimate is ever produced by this foundation: `KnowledgeLevel`
only ever reaches `Observed`, never `Characterized`, since nothing yet
supplies `ObservationEvidence::classification`. Passive observation
(`ObservationMode::Passive`) is supported by `science_knowledge.hpp` but has
no trigger anywhere in the simulation yet.

This qualifies for the parallel-safe lane the same way target selection and
manipulator reach telemetry did: it reads/mutates only its own new state
behind the existing scan-lifecycle boundary, introduces no new player
command, and does not assume any still-pending contact/collision/damage
Product Reality is correct.

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
- No new input binding was added or is required: the row only extends the
  existing always-visible panel and target-selection/scan surfaces.

## CI-verifiable acceptance

- `src/simulation/tests/simulation_core_tests.cpp` covers
  `observe_active_scan_progress()` directly: no knowledge exists for a
  target before it is ever scanned; a half-completed scan records partial
  `active_scan_s`/`confidence` at `Observed` level with no classification;
  completing the scan accumulates the remainder; an unrelated target's
  knowledge is untouched.
- `src/simulation/tests/save_data_tests.cpp` covers round-tripping
  `target_knowledge` byte-for-byte through save/load (exercised
  incidentally by `build_representative_runtime()`'s existing
  `start_scan`/`advance_wall_ticks` calls), `restore_from_snapshot()`
  rejecting a mismatched key/target_id pair and an out-of-range confidence,
  and a legacy save captured before this field existed inferring an empty
  map rather than throwing.
- `tools/test_phase2_science_knowledge_surface.py` confirms the wiring
  chain end to end: `ProbeStateSnapshot` owns `target_knowledge`;
  `SimulationCore` accumulates it only through the scan lifecycle and
  exposes fail-closed read accessors; `ProbeRuntime`/`DamageAwareProbeRuntime`
  forward without duplicating state; `save_data.hpp` persists it as an
  additive field; the Unreal adapter exposes a read-only status struct
  reusing the existing target-selection result; and the HUD renders the new
  row by extending the telemetry panel's height rather than overlapping the
  rows already below it.

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
7. Confirm this pass has not changed scan start/cancel/complete, target
   selection/cycling, manipulator, mining, contact, or damage behavior.
8. Record any discrepancy (row overlapping other panel content, confidence
   resetting unexpectedly, a stale reading after reselection, or a
   build/compile failure) as Product Reality evidence.

## Explicitly not complete in this pass

- No classification or composition/material estimate is ever produced;
  `KnowledgeLevel` only ever reaches `Observed`.
- No passive-observation trigger exists anywhere in the simulation yet,
  though `science_knowledge.hpp` already supports the mode.
- No persistent "discoveries" list, codex, or decision-enabling gameplay
  consequence — this is read-only telemetry over an accumulating number.
- No inventory/discoveries HUD page; only the single compact `KNOWLEDGE`
  row on the existing always-visible panel.
- Instrument-dependent resolution beyond a fixed nominal baseline
  (`kNominalInstrumentResolution = 1.0`) is not modeled.

## Status

Implemented in the parallel-safe lane; Product Reality pending. Does not by
itself close Slice 11.
