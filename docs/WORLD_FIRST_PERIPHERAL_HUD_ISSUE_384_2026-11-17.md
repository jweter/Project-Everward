# World-first peripheral HUD — issue #384, first bounded implementation

**State:** Proposed implementation in an isolated PR; **Unreal Product Reality pending**. This does not claim native compilation, gameplay capture, artistic approval, control-remapping coverage or HUD performance measurements.

## Source audit vs DeepSeek's external design review

DeepSeek's three peripheral-zone direction is useful, but several supplied examples must not become false Everward gameplay contracts:

- No authoritative whole-hull percentage exists. `FEverwardHUDReadModel::WeakestSubsystemFraction` is explicitly the *minimum of subsystem integrities*, not hull integrity. Default UI says **WEAKEST**; comprehensive integrity remains under **TAB**.
- A generic selected target's hull/surface range is not the mining-arm tool-tip gap or proof of mining eligibility. `FEverwardTargetSelectionStatus::SurfaceRangeMeters` is shown as **SURFACE RANGE**. The existing hint resolver deliberately does not claim all scanned targets can be mined.
- F1 is an actual controls reference; TAB is the existing Systems overlay; M is the existing manipulator overlay. There is no need to invent F2/F3 or duplicate controls. The active scan stays bound to its original target even if selection changes; pressing Enter again is not a retarget command.
- `FEverwardHUDActionHint::FromAuthoritativeState` already resolves energy/thermal lockouts, active scan identity and remaining time, selection, sensors, and bootstrap mining hints. No new simulation, adapter, or input authority is needed for this UI-only slice.
- The layout must not invent material quantities for `Fix_It`, nonexistent cinematic objectives, successful extractions, or new scan classifications. Existing `GetFixItStatusSummary()` and selected target knowledge remain read-only.
- The 0.3 ms HUD and zero heap allocations per frame in DeepSeek's draft are **unmeasured proposals**. Existing `DrawHUD()` polls multiple Unreal adapter projections and creates formatted strings; do not claim these targets without instrumentation. `docs/PERFORMANCE_BUDGETS.md` targets approximately 1280×720/30 FPS on the constrained development laptop and asks for actual measurements.

## Smallest player-facing improvement

The default view is now divided into three **peripheral** bounded rectangles, calculated by `FEverwardPeripheralHUDLayout::Compute()` with the existing 0.80–1.75 scale:

1. **Lower left — vital machine state.** Energy fraction, **weakest subsystem** integrity, thermal temperature in kelvin, storage fraction, and the existing `Fix_It` read model. A short F1/TAB/M entry line makes controls accessible. There is no giant full-height engineering panel by default.
2. **Upper right, only when a real target is selected.** The selected target's abbreviated identity (preserving tail), real **surface** range, and its actual science knowledge. No simulated target category, resource identity or mining reach.
3. **Bottom center — next authoritative action.** The existing `FEverwardHUDActionHint`, including its actual key labels and warnings, replaces the ever-present duplicated controls footer. The full controls reference remains F1; TAB still reveals detailed system telemetry.

This implementation deliberately reuses `AHUD` canvas drawing and existing getters; there is no UMG dependency and no mechanical behavior or save schema modification. For the base HUD only, the native geometry helper reserves the middle 60% of viewport width by middle 40% of height (at 1920×1080 and 1280×720). Explicitly opened engineering/manipulator overlays, flight/impact notifications and the immersive world itself are separate from this *default zone* assertion. The right target zone is placed **above** the reserved vertical region, avoiding DeepSeek's contradictory “center always clear, card at x70–96%, y42–62%” coordinates.

## Verification and acceptance

- Portable source contracts: `tools/test_phase2_peripheral_hud_contract.py` and updated established compact HUD, legibility, contextual-action and G03 test-list contracts. These guard wiring and absence of new command authority; they cannot prove rendered placement.
- Native `Everward.G03.Native.PeripheralHUDLayout` performs two viewport checks, verifies all default zone rectangles are within bounds, that the intended world center is not intersected, and that the action zone does not overlap vitals or target. The native `tools/run_unreal_native_g03.ps1` acceptance list now requires all **11** current G03 registrations; older binaries are not sufficient.
- **Still pending:** UBT compile; unattended actual game screenshot at 1920×1080 and 1280×720; font measurement/long-ID and large-number clipping; default view and F1/TAB/M usability; notification collisions and actual HUD-frame-time measurement; accessibility contrast assessment. Owner review only after these objective checks. An unsuccessful/pending gate is not a PASS.

## Follow-up priorities (not bundled in this PR)

1. A faithful contextual action router for target lost/out-of-range, scan-bound-other-target and blocked tool cases **only after** checking actual input and authoritative reach/result types; no fake “RETARGET” action.
2. Notification hierarchy and time-limited scan/impact banners derived from sequenced events, preserving critical warnings and avoiding permanent disclosure of last-completion notices.
3. Objective presentation with a genuinely authoritative next restoration goal, once the chosen `Fix_It`/restoration source and playable event sequence are verified.
4. Measured HUD legibility/contrast, screenshot comparator and low-spec frame budgets before product acceptance.

**Related:** #384 UI priority; #159 Product Reality; #168 end-to-end first-playable gate. The owner does not run routine regression tests and paid Aura is owner-only.
