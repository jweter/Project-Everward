# Phase 2 — Impact Severity and Component Damage Foundation

Status: **implementation started; portable CI required; local Product Reality remains pending on the preceding contact slice.**

This slice intentionally begins without waiting for the local collision playtest, but it does not declare the collision/contact prerequisite complete. The implementation is layered so a Product Reality correction to contact resolution can be made without discarding the damage model.

## Objective

Turn authoritative contact telemetry into physically grounded consequences:

```text
contact
-> normal relative velocity
-> impact energy
-> severity
-> impacted probe zone/component
-> component integrity loss
-> capability consequence
```

Do not introduce a single ship hit-point pool.

## Physical input

For the current immovable Phase-2 test body, impact energy is calculated from the probe's inward normal speed:

```text
E = 0.5 * probe_mass * normal_speed^2
```

Tangential velocity is not counted as normal impact energy. This preserves the distinction between a glancing scrape and a direct collision.

The current EV-0001 mass is 2,500 kg, so representative normal-impact energies are:

| Normal speed | Impact energy | Initial class |
|---:|---:|---|
| 2 m/s | 5 kJ | Contact |
| 5 m/s | 31.25 kJ | Light |
| 10 m/s | 125 kJ | Damaging |
| 30 m/s | 1.125 MJ | Severe |
| 50 m/s | 3.125 MJ | Catastrophic |

These thresholds are an engineering/tuning foundation, not final spacecraft-material limits. Later structural materials, armor, deformation, impact angle, penetrators, and moving-body reduced mass can refine the model without replacing the contract.

## Severity classes

- `CONTACT`: below 25 kJ; contact telemetry but no component integrity loss.
- `LIGHT`: 25–100 kJ.
- `DAMAGING`: 100–500 kJ.
- `SEVERE`: 500 kJ–2 MJ.
- `CATASTROPHIC`: at least 2 MJ.

Initial component integrity loss is calibrated from impact energy against a 2.5 MJ full-component-loss reference and clamped to 0–100% loss per impact. This avoids arbitrary `-10 HP` consequences while leaving room for later material-specific resistance.

## Component integrity

Generation-1 systems receive normalized integrity in the range `0.0 .. 1.0`.

The first operational bands are:

- `OFFLINE`: 0%.
- `CRITICAL`: greater than 0% and below 25%.
- `DEGRADED`: 25–75%.
- `OPERATIONAL`: 75% to below 100%.
- `NOMINAL`: 100%.

A key design requirement from the canonical awakening is preserved: **a system at 5% integrity is badly damaged but can still be functional.** Zero integrity is the hard offline threshold in this first model.

This distinction is essential for staged Self Repair. A future repair action can move a component through meaningful states rather than only toggling `broken/working`.

## Temporary Phase-2 impact zones

The current probe collision body is still a sphere, so precise component geometry does not exist yet. Damage therefore uses a deterministic, probe-relative temporary zoning model:

- forward impact -> Sensors;
- aft impact -> Propulsion;
- lateral impact -> Computation;
- dorsal/ventral impact -> Thermal Control.

The impact direction is transformed into probe-local coordinates before zone selection. Rotating the probe therefore rotates its vulnerable zones; damage is not attached to fixed world axes.

The Prime Generation-1 probe blockout must replace these coarse zones with actual component-corresponding geometry while preserving the same damage/integrity interface.

## Awakening / Self Repair compatibility

The damage runtime exposes the same integrity mutation path that will later initialize and repair the starter probe.

That means the opening does not need tutorial-only fake damage. A future damaged start can initialize, for example:

```text
Sensors       0%
Propulsion    0%
Computation   8%
Thermal      14%
Manipulator  17%   # added when manipulator capability exists
Power        ...   # expanded when power components become individually modeled
```

and Self Repair can raise those exact authoritative integrity values in stages.

The canonical priority remains:

> Restore capability before restoring perfection.

## Current implementation

`impact_damage.hpp` adds:

- `ImpactSeverity`;
- `IntegrityBand`;
- `ComponentIntegritySnapshot`;
- `ImpactDamageRecord`;
- `ImpactDamageModel`;
- `DamageAwareProbeRuntime`.

`DamageAwareProbeRuntime` composes the existing tested `ProbeRuntime`. Collision remains owned by `ProbeRuntime`; the damage layer consumes its authoritative contact record after each simulation advance. It does not recreate collision logic.

If an impact reduces a component to zero integrity, the damage layer routes the result through the existing subsystem-operational state, so existing capability consequences apply rather than creating a parallel truth system.

## Portable acceptance coverage

The new C++ regression suite verifies:

1. energy-based severity thresholds;
2. integrity-band semantics including a functional 5% system;
3. forward impact -> sensor damage;
4. aft/lateral/dorsal component zoning;
5. low-energy contact causes no integrity loss;
6. one contact cannot be double-counted;
7. catastrophic impact can take a subsystem offline through existing capability state;
8. the same integrity API can initialize, disable, and restore a subsystem for the future awakening loop.

## Next integration step in this slice

After the portable damage model is green, wire `DamageAwareProbeRuntime` into the Unreal adapter and expose:

- latest impact energy and severity;
- affected component;
- integrity before/after;
- current integrity of all installed systems;
- a short player-facing damage banner;
- integrity bands in the subsystem HUD.

That presentation/integration work should remain unmerged until the preceding collision/contact Product Reality result is known, because the project plan explicitly treats mechanics built on unverified collision feel as non-parallel-safe completion work.

## Product Reality target

Once wired into Unreal, the player should be able to perform three obvious tests:

1. touch the body gently and see `CONTACT` with no damage;
2. strike it at a moderate speed and see a named subsystem lose integrity but remain usable;
3. strike it hard enough to take the impacted subsystem offline and see the existing capability lockout immediately reflect the damage.

The important feeling is not "my health bar went down." It is:

> I physically hit something, a specific part of my body was damaged, and the machine now behaves differently because of it.
