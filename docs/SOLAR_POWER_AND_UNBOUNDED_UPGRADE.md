# Everward — Solar power, and why every component must be upgradeable forever

**Status:** Design, owner-directed 2026-11-28. Slice 1 in progress.
**Parent:** #378 (first playable loop). Relates to `docs/LANDING_AND_STRANDED_START.md`,
`docs/ADJACENT_GENERATION_EVOLUTION.md`, `docs/FIX_IT_UPGRADE_AND_DRONE_ECOLOGY.md`.
**Owner direction, verbatim:**

> you need to be able to get to that next step of the mission or else you are stuck and will die. Run out of power.
> Thus we need the probe must look like the design photo. Big solar panels to catch the energy to live. Which means
> you also need to repair and replace them. Everything on the probe needs to be able to update again and upgrade
> forever. Always increasable. Everything.

## 1. The chain this closes

Stranded on the surface → power is the clock → the panels are the power → so the panels are the first thing worth
repairing → and if a panel can be repaired, replaced and improved without limit, so must everything else be.

The energy clock is what makes the opening a game rather than a tour. The player is not waiting for permission to
leave; they are spending a finite reserve to earn the ability to leave.

## 2. What already exists

Checked in the code, not assumed:

- **The energy balance is real and enforced.** `types.hpp` carries `stored_energy_j`, `energy_capacity_j` and
  `energy_generation_w`; `integrate_energy_balance()` in `core.hpp` applies generation against allocated
  consumption every fixed step; `is_energy_depleted` latches when the reserve empties and clears on a net-positive
  balance. **Depletion already locks the probe out** (`core.hpp:833`, alongside overheating). So "run out of power
  and you are stuck" is implemented today.
- **Generation is a placeholder, and says so.** The comment on `energy_generation_w` states it models an RTG-style
  constant supply because *"solar generation would need the star-distance/irradiance model that does not exist
  yet"*, and defaults to 0.0 so a bare core keeps an exact generation-free baseline.
- **An unbounded-upgrade framework exists and is tested.** `evolution.hpp` has `EvolutionDomain`,
  `EvolutionTrait`, `EvolutionCandidate`, `EvolutionFrontier` and `AdjacentEvolutionGenerator`, with
  `evolution_generator_tests.cpp` covering it. It is consumed by `sensorium.hpp` and **not wired to the probe's
  physical hardware**.
- **Integrity already scales capability** everywhere it exists: thrust and attitude trim by propulsion, scan time
  by sensors, and the autopilot as of #461. There are exactly four `PowerSubsystem` values — Sensors, Propulsion,
  Computation, Thermal — and **no power-generation subsystem**, so panels cannot currently be damaged or repaired.

## 3. Design

### 3.1 The solar array is a component, not a power subsystem

`PowerSubsystem` means "a thing you allocate watts **to**". Panels generate; you do not allocate power to them.
Overloading that enum would also widen the 4-wide allocation arrays and the save format for no conceptual gain.

So the array is its own component with its own integrity:

```
SolarArray {
    double area_m2        // total illuminated area when deployed
    double efficiency     // 0..1, fraction of incident power converted
    double integrity      // 0..1, damage state, repairable like any subsystem
    bool   deployed       // stowed panels catch nothing
}
```

### 3.2 Generation

Generation is additive to the existing `energy_generation_w` baseline, so every current test keeps its exact
behaviour and the RTG remains a legitimate configuration:

```
generated_w = baseline_w
            + irradiance_w_per_m2 * area_m2 * efficiency * integrity * max(0, cos incidence) * (1 - occlusion)
```

A surface-bound probe needs no orbital mechanics for this: a sun **direction** and an irradiance figure for the
body are enough. `cos incidence` is the panel normal against the sun direction, so orientation matters and a probe
lying on its back in shadow generates nothing. Occlusion covers night, terrain shadow and dust, and starts at 0.

| Tunable | Start value | Why |
|---|---|---|
| `irradiance_w_per_m2` | 1361.0 | the solar constant, the honest default at 1 AU |
| `area_m2` (Mark 1) | 6.0 | two large panels, from the canon sheet proportions |
| `efficiency` | 0.22 | credible for rigid space-grade cells |
| `occlusion` | 0.0 | no shadow model yet; raising it is how night arrives |

At full health that is about 1.8 kW facing the star — meaningful against the probe's allocations, and worth
protecting.

### 3.3 Damage, repair, replacement

- Panels take damage from the same impact model as everything else. Damaged panels generate proportionally less:
  the same integrity rule the rest of the probe already follows.
- Fix_It repairs them with mined material and stored energy, like any subsystem, and **the panels are the highest
  priority target when generation is below consumption** — restoring the thing that pays for every other repair
  comes first. That is the survival-first doctrine Fix_It already has, applied to the new component.
- Replacement is repair past 1.0: a destroyed panel is replaced rather than mended, costing more material.

### 3.4 Upgrade forever

The owner's requirement is that nothing has a ceiling. Two rules make that true without inventing a new system:

1. **No capability is capped.** `area_m2`, `efficiency`, `energy_capacity_j`, storage, reach, thrust — each is a
   number that an upgrade raises. There is no maximum tier, and no code may clamp one to a constant.
2. **Cost grows instead.** Each upgrade of a trait costs more material and energy than the last, so progression is
   bounded by effort rather than by a cap. `evolution.hpp`'s frontier model already expresses exactly this —
   candidates adjacent to what you have — and this design wires it to the probe's hardware traits for the first
   time.

Efficiency is the one trait with a physical ceiling (1.0). It approaches it asymptotically; area, capacity and
count do not, which is why "big solar panels" is the right answer to needing more power.

### 3.5 The look follows the function

The probe must read as the Mark 1 from the canon sheets, and the panels are the reason: they are the largest and
most recognisable thing on it. Because area is now a simulation value, **the drawn panel size must derive from
`area_m2`** rather than being decorative. An upgrade that doubles the area must visibly double the panels, and
`--visualtest` can then check the picture against the number — the same drawn-versus-simulated discipline already
applied to the arms.

## 4. Implementation slices

1. **Core: the solar array and its generation.** `SolarArray` in probe state, sun direction and irradiance,
   additive generation in `integrate_energy_balance()`, integrity scaling, deployed/stowed. Unit tests: generation
   scales with each factor, is zero when stowed, in shadow, or at zero integrity, never negative, and the existing
   generation-free baseline is unchanged. Parity test.
2. **Fix_It repairs and replaces panels**, prioritised when generation is below consumption.
3. **Wire `evolution.hpp` to hardware traits**: area, efficiency, capacity as uncapped, cost-growing upgrades.
4. **Godot: panels on the probe**, sized from `area_m2`, oriented to the sun, with a HUD power line showing watts
   in against watts out and time to depletion. `--visualtest` checks drawn area against the simulated figure.
5. **The stranded opening uses all of it**: wake with damaged panels and a draining reserve, mine, repair panels,
   repair propulsion, fly.

## 5. Out of scope here

Orbital mechanics, star distance over a real orbit, a day/night cycle, dust accumulation, battery chemistry,
thermal coupling of the panels, and drone-built panels. Each is a later slice; none is needed for the clock to run.
