# Issue #384 — compact authoritative HUD bridge (first delivery slice)

## Why not paste the DeepSeek UMG example verbatim?

Everward currently uses `AEverwardHUD : AHUD` with Canvas drawing, an existing
`FEverwardProbeTelemetry` from `UProbeSimulationAdapter`, and a separate
`AFixItRuntimeActor` that owns the live repair-orchestration presentation.
A new `UUserWidget` and `BindWidget` fields would require an authored UMG
asset, a creation path, and a switch from the existing HUD. Three unused files
alone would not improve playability. The example's `HullIntegrityPercent`
and 20-degree temperature placeholder also misrepresent existing truth:
the core exposes four independent subsystem integrities and temperature in K.

## This PR

- Adds a Blueprint-readable `FEverwardHUDReadModel` derived only from the
  existing simulation adapter and the real Fix_It runtime summary. It normalizes
  energy and storage against authoritative capacities, clamps nonfinite values,
  and calls its integrity readout **weakest subsystem**, not hull.
- Defaults to a smaller persistent status card with visible numeric energy
  and subsystem percentages, bar lengths, temperature **K**, selected target
  and **surface** range, a basic scan/move/lockout state, and actual Fix_It
  summary (or an explicit unavailable state).
- `TAB` continues to open the existing full systems detail panel and the
  complete historical live telemetry readout. `M` opens the detailed
  manipulator page, closed by default. `F1` preserves the controls reference.
  No simulation rules, commands, physics, storage, repair economics, hotkeys,
  save format, or event semantics change.
- Retains Canvas drawing. Canvas necessarily repaints each render frame,
  so this does **not** claim event-driven UMG widget invalidation or a render
  performance optimization. That is a separate measured future refactor.

## Verification / remaining acceptance

- Source contract tests cover the compact/expanded split, strict adapter
  boundary, and lack of invented hull integrity or Celsius values.
- Run UnrealBuildTool on the exact PR head and capture real 1920x1080 and
  1280x720 game frames. Verify no overlap with banners, flight readout,
  action reference, target labels, or new Fix_It summary; verify the
  damaged-awakening -> scan -> mine -> storage -> Fix_It progression.
- Run keyboard/selection/control regressions for TAB, M, F1, and global SPACE.
- This is **not** closure of #384: contextual target actions, accessibility,
  photo-mode/HUD hide, visual layout proof and full UX acceptance remain.
- No owner-run paid Aura required or authorized. Do not mark Unreal/Product
  Reality PASS without actual build and capture evidence.
