# Everward — target-aware scientific scanning

**Source state:** implementation proposed on a review branch. **Product Reality:** pending exact-head UBT, native Unreal automation, and unattended HUD/input capture. No claim of live game acceptance, native build PASS, or owner-run Aura test.

## Player outcome

The physical object the player selects with T is the object scanned when the Sensors controls are active (TAB -> select Sensors -> Enter). Scan results are bound to the original target even if selection changes. Bootstrap mining still requires a *completed scan of the bootstrap deposit itself*, not a scan of an unrelated reference body.

## Existing authoritative contracts preserved

- `AEverwardPlayerController::ExecutePrimarySystemAction` obtains `GetSelectedTargetStatus()` and passes its `FString TargetId` to `CommandStartScan(TargetId, Phase2ScanDurationSeconds)` only in the existing Sensors/expanded-panel branch. No new keybinding.
- Empty selection is passed as an empty ID to the adapter so it produces a sequenced, visible rejection: `no target selected`. The adapter rejects a nonempty but unregistered physical ID; the core remains usable for isolated synthetic-ID simulation tests.
- `SimulationCore::start_scan()` already binds `active_scan_target_id` at start and rejects overlapping scans. **Pressing Enter again never auto-cancels or retargets**. Backspace explicitly calls `CommandCancelScan()`.
- At scan completion, `observe_active_scan_progress()` had only mutated the bound target's science record; `GetSelectedTargetKnowledgeStatus()` follows the current selected target, and `GetDiscoveredTargets()` records prior observed targets.
- Lowering **Sensors allocated power** below the Gen-1 operating minimum deliberately cancels the scan in `ProbeRuntime::allocate_power()`. A separate sensor/energy/thermal lockout pauses scan integration without fabricating progress until unlocked; maintain both existing rules.
- `save_data.hpp` already persists `is_scanning`, `active_scan_target_id`, and `scan_remaining_s`; this slice does not change the save schema.
- The scan-completion event identifies the target and only the bootstrap completion can latch `bBootstrapResourceSurveyed`; an unrelated reference scan must not unlock mining.

## Authoritative HUD presentation

- `FEverwardHUDActionHint` now receives `FEverwardTargetKnowledgeStatus` as read-only input. While a scan is running, the compact ACTION row shows the **bound** scan target and remaining time, regardless of subsequent selection. Time takes formatting priority over long IDs.
- A selected unobserved/partially observed non-bootstrap body now prompts the real TAB/Sensors/Enter scan path; a fully observed reference may prompt tentative manipulation but **never bootstrap mining**.
- The expanded Sensors panel's historical hardcoded `ROCKY BODY / SURVEY REFERENCE`, `SILICATE-RICH // IRON-BEARING MATERIAL`, and `92%` presentation constants were not genuine scan measurements. On completion the panel now looks up the actual `FEverwardDiscoveredTarget` by the scanned target ID and prints only observed level, actual classification (or `COMPOSITION UNRESOLVED`), and observed confidence.
- No new simulation-owned state, HUD panel, scan progress proxy, UMG widget, or fake classification.

## Tests and unattended acceptance

- `Everward.G03.Native.SelectedTargetScanBinding`: world-backed adapter checks no-selection and unregistered IDs, reference-A scan/selected-B separation, second-scan rejection, bound target persistence, target-A evidence and missing reference classification, bootstrap survey preservation, Backspace-equivalent explicit cancel, and sensor-power cancellation.
- `Everward.G03.Native.ContextualHUDActionHint`: bound target and remaining seconds, selection changes, locked Sensors, reference scan hint vs fully observed reference, and bootstrap mining gating.
- `tools/test_phase2_selected_target_scanning_contract.py`, updated `tools/test_phase2_first_run_environment.py` and `tools/test_phase2_scan_discovery_surface.py` protect source-level wiring against reintroduction of the hardcoded target or invented science claims.
- Independent exact-head CI, **UBT**, native automation, and 1920x1080 + 1280x720 in-game evidence are required for Unreal verification.
- Unattended capture scenario: select nothing and press Enter -> rejection; T select bootstrap; press Enter; T select reference B during scan -> bound bootstrap remains; explicit Backspace -> cancel; select B -> scan B; inspect science/discoveries -> only actual B evidence; select bootstrap -> complete scan -> mining survey unlocked; sensor power off -> scan reject or cancel.
- Owner is not the routine regression executor. Paid Aura stays owner-only and is not part of this request.

## Remaining limits

The core currently permits synthetic target IDs for engine-independent unit tests; the Unreal adapter's registered-body guard is a gameplay admission rule, not a claim that all future targets will be static spheres. This slice does not implement scan range physics, a new scientific instrument, composition detection for reference bodies lacking a real material ID, or generalized mining of every scanned body. Those are separate design/QA tasks.
