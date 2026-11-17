# HUD #384 — Contextual action guidance (follow-up to PR #415)

**Evidence state:** IMPLEMENTED when source and portable/native tests exist; Unreal/Product Reality acceptance remains pending until a pinned Unreal build and actual capture.

## Purpose
Replace the compact HUD's existing ACTION line with one practical, grounded
next-step hint. Preserve the compact panel footprint and the full TAB-only
engineering view. No new input mapping, gameplay mechanics, UMG, simulation
state or authority. Keep the live Fix_It runtime status on the separate line.

## Verified source facts
- `AEverwardPlayerController::SetupInputComponent` binds `T`, `Enter`,
  `TAB`, `F`, `1/2`; `Enter` starts the *bootstrap* scan only while TAB
  systems is open and Sensors is selected.
- `AEverwardPlayerController::Tick` in
  `EverwardPlayerControllerInteractionTick.cpp` handles `7` tool,
  `P` mining staging and `G` extraction; looking only at
  `SetupInputComponent` would miss those real controls.
- `UProbeSimulationAdapter::GetMiningStatus()` in `ProbeMiningBridge.cpp`
  currently describes **only** the bootstrap deposit; it is not the status
  of every selected target.
- `G` calls `CommandMineBootstrapTarget()`; only the engine-independent
  mining rules decide whether the surveyed deposit is accessible with a
  deployed/tool-equipped arm, actual tool-tip contact and storage.
- Probe-to-target *hull surface range* is not tool-tip reach. No numeric
  comparison of those different quantities is used to promise mineability.
- `FEverwardStarterRestorationStatus` has `bSatisfied`,
  `PendingSubsystems`, `WeakestPendingIntegrity`; it does not contain
  `bIsActive`, `bNeedsRepair`, `RequiredMaterial`. Fix_It's live runtime
  actor already prints its authoritative status in its own HUD row.

## Visible behavior and limits
- No selection: `T` cycle target.
- Energy depleted/overheat: warning and TAB system inspection.
- Active scan: remaining scan time; no contradictory scan-start prompt.
- Bootstrap selected and unsurveyed: TAB, select Sensors, Enter to scan only
  when actual Sensors capability is operational/available.
- Surveyed bootstrap: show exhausted deposit/full storage separately; ask for
  arm/tool setup if needed; otherwise suggest **attempting** P staging or G
  extraction (not a guarantee of acceptance).
- Other selected bodies: suggest inspecting manipulator/trying F grasp;
  never imply the bootstrap deposit can be mined via an unrelated selection.
- Pending Fix_It damage is not an unconditional override: mining is part of
  the intentional damaged-awakening resource recovery loop. Preserve the
  independent live Fix_It status.
- The last command result is intentionally not the persistent hint's source:
  failed command details already appear in the temporary command banner and
  would become stale after a subsequent target change.

## Acceptance still required
- Portable source-contract tests (CI) and
  `Everward.G03.Native.ContextualHUDActionHint` (native Unreal Automation).
- Real pinned 1920x1080 and 1280x720 captures: no overflow, overlap,
  duplicate selection prompt, obscured Fix_It/status, or misleading keys.
- Real input flow: no target -> T -> TAB/sensors/Enter -> survey -> deploy
  tool/arm -> P/G with true simulator rejection reasons and resulting storage.
- Confirm non-bootstrap targets do not display a mining-ready hint; confirm
  low power, overheating, and full storage produce correct warning cues.
- If any keymapping/control behavior changes in another PR, re-audit these
  hints before declaring them accepted. No owner-run paid Aura is requested.
