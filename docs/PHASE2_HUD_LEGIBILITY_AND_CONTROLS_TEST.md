# Phase 2 HUD Legibility and Controls Reference Test

Status: **Implemented; Product Reality pending in the local Unreal build.**

## Why this pass exists

The 2026-08-30 current-build captures at 2048x1200 confirmed a P0 usability
failure: the live HUD technically exposed telemetry and bindings, but most
text rendered at approximately 8-11 screen pixels and could not be read at a
normal laptop viewing distance. The manipulator footer compressed the entire
control surface into one miniature line. The systems panel repeated the same
problem with dense status and command text. A visible control that cannot be
read is not discoverable.

The comparison reference was the control presentation in *ΔV: Rings of
Saturn*: immediate telemetry remains in the operating view while a dedicated,
large controls screen groups bindings by physical task. Everward adopts that
information-architecture principle without copying ΔV's art direction.

## Implemented response

- live HUD layout scales from the actual Canvas viewport instead of assuming
  one fixed pixel size;
- all HUD text uses Unreal's medium font with a minimum readable scale;
- left telemetry and right system panels are wider, more opaque, and use
  larger row spacing;
- storage shows exact authoritative mass, capacity, and rounded percentage;
- long control strings are split into readable rows;
- a persistent short entry bar teaches only `F1`, `Tab`, and `M`;
- `F1` opens a dedicated three-column controls reference grouped into Flight
  + Camera, Systems, and Manipulator + Mining;
- the spatial scan/mining label is enlarged from 105 to 170 world units and
  moved higher above the resource body;
- a rejected command is not rendered twice when the automation notice carries
  the identical rejection detail.

## Exact local Unreal 5.8 test

Use the exact CI-green branch build. Keep the same display resolution and
Windows scaling used in the failed 2026-08-30 capture.

1. Launch PIE and take a full-window screenshot before pressing anything.
2. From ordinary seated laptop distance, read aloud every left telemetry row,
   every collapsed Systems row, and the bottom `F1` / `Tab` / `M` entry bar.
3. Press `F1`. Confirm the live HUD is replaced by one large controls page.
4. Without repository documentation, locate the bindings for:
   - full stop;
   - starting a scan;
   - adjusting selected-system power;
   - deploying the Port arm;
   - selecting the wrist/tool;
   - adjusting a joint target;
   - mining.
5. Press `F1` again. Confirm the same live state returns.
6. Press `Tab`; inspect all four system rows, then select Propulsion and
   Sensors. Confirm no label or control line clips outside the panel.
7. Press `M`; inspect the manipulator panel with both arms stowed. Deploy both
   arms and confirm the panel grows without overlapping the left telemetry or
   leaving the viewport.
8. Complete one scan and confirm the target label is comfortably readable and
   does not sit directly on top of the probe at the normal test camera angle.
9. Press `G` before the tool is in reach. Confirm the rejection appears once,
   remains readable, and states the recovery action.
10. Complete two mining cycles. Confirm the always-visible storage row changes
    from `0.0 / 500.0 KG (0%)` to `5.0 / 500.0 KG (1%)` and then
    `10.0 / 500.0 KG (2%)` while the deposit changes from 250 to 245 to 240 kg.

## Acceptance

Pass only if:

- the player can read the ordinary live HUD without leaning in, zooming a
  screenshot, or opening source documentation;
- every current binding can be found from `F1` in under 30 seconds;
- normal HUD, expanded Systems, and expanded Manipulator layouts do not clip
  or overlap at the tested viewport;
- storage exact mass, capacity, percentage, mining notice, and deposit
  remainder agree after both mining cycles;
- repeated rejection text does not create two simultaneous identical alerts.

Record the tested commit, resolution, Windows scaling, screenshots of the
startup HUD, F1 controls page, expanded Systems, expanded Manipulator, first
mining result, and second mining result. This source change does not count as
accepted Product Reality until those screenshots exist.
