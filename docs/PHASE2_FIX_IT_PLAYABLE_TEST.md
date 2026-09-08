# Phase 2 — Playable Fix_It Integration

## Purpose

Make `Fix_It` an actual playable Everward system rather than lore plus an engine-independent kernel.

The canonical simulation already owns the Repair → Replacement → Upgrade → Redesign → Evolution planning rules. This slice connects the **Repair** portion to the production Unreal probe without duplicating that truth in presentation code.

## Player-visible behavior

A fresh Phase-2 EV-0001 session begins damaged but not helpless:

- computation: 18%
- thermal: 22%
- sensors: 35%
- propulsion: 40%

Every subsystem remains above zero, so the poorly built Generation-1 probe can still limp, scan inefficiently, move badly, and use its mining/manipulator hardware.

`Fix_It` immediately evaluates the authoritative component-integrity state.

It follows its canonical priorities:

1. preserve survival-critical computation/thermal capability;
2. restore missing/critical capability before polishing already-working systems;
3. improve the weakest functioning subsystem in useful integrity bands;
4. never fabricate matter, energy, design knowledge, or future capability.

If the selected repair cannot be afforded, `Fix_It` waits and visibly explains the material/energy requirement.

Once enough mined material and stored energy exist, the existing `FixItRepairExecutor` runs automatically. It consumes **real probe storage and energy**, advances repair over real elapsed repair time, changes the same subsystem-integrity state used by damage/capability checks, then replans.

## Presentation

Until the broader Phase-2 HUD/input redesign lands, a persistent on-screen `FIX_IT // ...` status line exposes:

- current stage;
- current priority subsystem;
- waiting resource requirement;
- repair progress;
- material consumed;
- the canonical planner reason.

This status line is presentation only. It does not determine repair truth.

## Playtest evidence

The runtime records structured events through the existing `APlaytestRecorderActor`:

- `fix_it_awakened`
- `fix_it_runtime_bound`
- `fix_it_repair_started`
- `fix_it_repair_completed`
- `fix_it_repair_interrupted`
- replacement lifecycle events once fabrication becomes available

## Product Reality test

1. Launch a fresh current-main Unreal playtest.
2. Confirm the systems read approximately 18% computation, 22% thermal, 35% sensors, 40% propulsion.
3. Confirm `Fix_It` identifies computation as the first priority and waits for material rather than inventing it.
4. Scan/mine enough bootstrap material to exceed the displayed requirement.
5. Confirm `Fix_It` automatically begins the computation repair.
6. Watch storage decrease and computation integrity increase progressively.
7. Confirm completion at the planner's target band, followed by automatic reassessment.
8. Confirm thermal becomes the next survival-critical priority.
9. Continue mining and verify Fix_It works through staged subsystem recovery rather than jumping everything to 100%.
10. Press F12 on any mismatch between visible status, storage/energy consumption, or subsystem integrity.
11. After the run, confirm the playtest ZIP contains Fix_It lifecycle events.

## Architecture boundary

`AFixItRuntimeActor` is orchestration/presentation glue. It is granted narrow friend access to `UProbeSimulationAdapter` only so it can run the already-tested engine-independent `FixItPlanner`/executors against the authoritative `DamageAwareProbeRuntime`.

It must not grow a second repair model.

Replacement is already architecturally connected, but current Phase 2 deliberately reports it as blocked until real fabrication capability exists. Upgrade, Redesign, and Evolution remain recommendation/design gates and must never become magical in-place hardware mutations.

## Acceptance

This slice is implementation-complete when source/CI checks are green.

It remains **Product Reality pending** until a local Unreal 5.8 playtest demonstrates:

`mine real material -> Fix_It spends it -> subsystem integrity rises -> capability improves -> Fix_It replans`

That loop is a core Everward identity gate.
