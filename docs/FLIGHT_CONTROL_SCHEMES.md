# Everward — Selectable flight control schemes

**Status:** Design, owner-directed 2026-11-28. Slice 1 in progress.
**Parent:** #378. Relates to `input_binding.hpp` (semantic actions already exist),
`docs/LANDING_AND_STRANDED_START.md`, `docs/SOLAR_POWER_AND_UNBOUNDED_UPGRADE.md`.
**Owner direction, verbatim:**

> For the control over the way the probe flies. It must be improved. I remember an old game called Decent. That had
> 3d space flight and was very good with it's controls setup. Can you copy that?

> I don't know honestly. I'd like to have the option to do either. Can you build multiple control schemes that are
> selectable during the game? I'd like to copy the control schemes for the top 10 games of the last 40 years that
> could fit our needs. Then I'll have 10 control schemes that I can cycle through to find the one that feels right
> for each probe setup.

## 1. What is being copied, and what is not

A control scheme — which input moves which axis, whether rotation has rate or snaps, whether velocity damps — is a
**design convention**, freely implementable, and naming the game that popularised it is a factual reference. What
this does **not** do is copy code, assets, or claim to reproduce another game's exact tuning values: those numbers
are not available to us and inventing them under another game's name would be a fabrication. Every figure here is
ours, derived from the probe's own mass and damage model, and every one is a tunable.

So: the *philosophy* of each scheme is faithful. The numbers are a starting point to be felt and changed.

## 2. Why this is cheap

`input_binding.hpp` already declares the whole 6DOF semantic vocabulary — `RollProbeLeft/Right`,
`PitchProbeUp/Down`, `YawProbeLeft/Right`, the three translation pairs, `StopPropulsion`,
`BeginOrCancelCameraAlignedRighting`, `ToggleControlledDescent/Hover`, `ToggleJoseTakeTheWheel`. The actions exist;
the Godot path implements five of six axes and **no roll at all**.

So a scheme is not new plumbing. It is a row of parameters over actions that already have names.

## 3. The real problem a scheme must be able to fix

Rotation today is an **instantaneous pose delta**: mouse pixels × 0.12 written straight into `attitude_degrees`.
There is no angular velocity anywhere in the simulation — no rate, no ramp, no damping, no rotational momentum.
That is why the probe does not feel like a flown object: it is a number being nudged, and it stops dead the moment
input stops.

Descent felt good largely because rotation had *rate*: the ship spun up, carried the spin, and bled it off. So the
scheme model must be able to express both, and the simulation must gain angular velocity for any of the rate-based
schemes to exist at all. That is slice 1, and it belongs in the core so both engines inherit it.

## 4. The parameter model

```
ControlScheme {
    name, reference, one-line description

    rotation_model        Direct | Rate            // pose delta, or angular acceleration
    rotation_authority    deg/s^2 (Rate) or deg per input unit (Direct)
    rotation_rate_max     deg/s cap, 0 = uncapped
    rotation_damping      per-second bleed of angular rate; 0 = holds spin forever (Descent/KSP), high = snappy
    roll_enabled          bool
    auto_level            Off | TowardCamera | TowardSurface   // what happens when input stops

    translation_model     Impulse | Throttle       // tap adds velocity, or hold sets a commanded speed
    translation_authority m/s^2
    flight_assist         0..1 bleed of velocity not aligned with facing; 0 = pure Newtonian drift
    brake                 None | Instant | Gradual
    boost_multiplier

    mouse_model           DirectDelta | ReticleLag | LookOnly
}
```

Everything is scaled by propulsion integrity at the point of use, so a damaged probe is sluggish in *every* scheme.
No scheme may bypass the damage model — that is the rule José broke (#461).

## 5. The ten schemes

Chosen to be distinct in ways a player can feel, not ten variations of one idea. Each names its reference.

| # | Name | Reference | What makes it feel different |
|---|---|---|---|
| 1 | **Six Degrees** | Descent (1995) | Full 6DOF, rate-based rotation, no privileged up, sliding is central, **no brake**, holds attitude when you let go. The reference scheme. |
| 2 | **Newtonian** | Kerbal Space Program | Pure momentum, torque-driven rotation with real inertia, no assist, no auto-level. Stability assist is a *separate* toggle holding current attitude. |
| 3 | **Flight Assist** | Elite / Elite Dangerous | Throttle-based translation, rotation damps to zero, velocity bleeds toward facing. Forgiving. The assist is toggleable, and turning it off is scheme 2. |
| 4 | **Reticle Flight** | Freelancer | Mouse aims a reticle; the probe turns toward it with lag. One hand, very approachable, poor for precision docking. |
| 5 | **Drift Assist** | Subnautica (Seamoth) | 6DOF with heavy damping and self-levelling. Slow, floaty, forgiving — plausibly the best fit for a half-wrecked probe. |
| 6 | **Heavy Hull** | MechWarrior | Hull turns slowly and separately from where the sensors aim. Momentum dominates; you commit to a heading. Fits a probe with two three-metre arms. |
| 7 | **Lander** | Lunar Lander / descent games | Rotate and thrust only, no strafing, gravity-dominant, fuel-conscious. Punishing and exactly right for touchdown. |
| 8 | **Coupled / Decoupled** | Star Citizen | A speed limiter plus a hard mode switch: coupled holds a speed, decoupled is free drift. Two feels on one key. |
| 9 | **Command It** | Homeworld | You do not fly at all: pick a destination and the autopilot flies it. This is José, promoted from a crutch to a scheme. |
| 10 | **Zero-G Jets** | Shattered Horizon | Impulse jets in all six axes with a dedicated full-stop, no assist, roll free. Deliberate, punchy, twitchy. |

Scheme 9 matters more than it looks: the owner built José *because* flying was hard. As a scheme it stops being an
apology for bad controls and becomes a legitimate way to play.

## 6. Selection

- Cycled in game with a dedicated action, and the HUD names the active scheme while flying.
- **Per probe setup**, as the owner asked: the active scheme is saved, so a probe configuration keeps its feel.
- Changing scheme never changes simulation state — only how input becomes commands. A scheme cannot be a cheat.

## 7. How ten schemes stay honest

Ten schemes means nine that nobody is looking at, so a regression in one is invisible. Therefore a scheme is only
accepted with an automated acceptance pass: **every scheme must be able to fly**. For each, the playtest drives the
real inputs and asserts the probe can translate on every axis the scheme allows, rotate on every axis it allows,
stop if the scheme has a brake, and that no scheme produces NaN, runaway spin or velocity the authority cannot
explain. A scheme that cannot pass that is a broken scheme, not a taste.

## 8. Implementation slices

1. **Core: angular velocity.** Rotation becomes rate-based in the simulation — angular rate state, authority,
   damping, clamping, propulsion scaling, validation, save/load, parity. Direct mode remains expressible.
2. **Core: the scheme table** as pure data plus the input→command transform, with per-scheme tests.
3. **Godot: apply, cycle, display**, and save the choice per probe.
4. **Playtest: `--schemetest`**, the acceptance pass of §7 across all ten.
5. Tuning passes with the owner, one scheme at a time, using the exported knobs.

## 10. Reconciliation with the flight benchmark (2026-11-28)

The owner commissioned a 1986-2026 spaceflight control benchmark and passed it in as notes. It is a research
document and says so: its rankings, 0-5 scores and proposed keybindings are its author's engineering judgement
rather than measured consensus, and the quoted review percentages are from retrieved snapshots. Treated as design
input, not as fact, three of its conclusions change this document and one of its recommendations cannot be built
yet. The checks below are against this repository's code, not against the benchmark's claims.

### 10.1 Its architecture is better than ten schemes, and they reconcile

The benchmark's central point is sharper than section 5's table:

> It would need one physics engine with three control-assistance configurations.

That is right, and it is the same insight as "a scheme is data": the axis that actually matters is **how much the
flight computer helps**, not which game a preset is named after. So the structure becomes two levels:

- **Assist level** is the primary control: Assisted, Semi-assisted, Newtonian. One physics, three amounts of help.
- **The ten schemes remain presets** that set an assist level plus the bindings and feel parameters. "Six Degrees"
  is Newtonian with roll and no brake; "Drift Assist" is Assisted with heavy damping. They are starting points for
  the owner to feel, which is what he asked for, and they are no longer pretending to be ten architectures.

### 10.2 Two things it recommends cannot be expressed yet, and one already can

Checked in the code:

- **Thrust ignores mass.** `adjust_local_velocity_mps()` adds a velocity delta directly; `mass_kg` (2500 kg
  default) is consulted only by impact kinetic energy, the tractor field and save/load. So thrust-to-weight is
  currently meaningless and **a probe carrying 500 kg of ore accelerates exactly like an empty one** — which is
  precisely the Space Engineers lesson the benchmark asks for, and the TWR formula it gives has nothing to apply
  to. This is the prerequisite for most of section 7 of the benchmark.
- **The precedent for fixing it is already here.** `tractor_field.hpp` integrates properly: force, then
  `seconds / probe.mass_kg`. Thrust should follow that, not invent a second convention.
- **Gravity exists in the core and never reaches Godot.** `integrate_gravity()` applies
  `gravitational_acceleration()` per step, and is an exact no-op when no planetary body is registered. The
  GDExtension exposes no planetary body, so **the Godot game has no gravity at all**. Liftoff needing TWR above 1,
  the ascent profile in benchmark section 6, and orbital insertion all depend on registering one.
- **There is no atmosphere anywhere.** No drag, density or lift term exists in the core, so the benchmark's
  `F_D` formula has no counterpart to extend.

### 10.3 Its keybinding table collides with the first loop

The proposed defaults cannot be adopted as written. Six keys are already load-bearing in Everward:

| Key | Benchmark proposes | Everward uses it for |
|---|---|---|
| `G` | landing gear | **mine** — the payoff key of the whole first loop |
| `T` | target selection | **deploy / stow the selected arm** |
| `L` | landing assistance | clear the software policy |
| `H` | horizon alignment | hollow out terrain |
| `Q` / `E` | roll | vertical thrust (up / down) |
| `Space` | upward thrust | brake / cancel an operation |

This is a data problem rather than a design conflict, and the binding system already solves it: `input_binding.hpp`
has semantic actions, `DeviceKind`, rebinding and clear/rebind outcomes. Both layouts can exist as named presets
with per-preset bindings. What must not happen is a silent rebind of `G` or `T`, which would break mining and the
manipulator for anyone who had learned them.

Worth noting the benchmark independently proposes `Q`/`E` for roll, which this document had as the missing sixth
axis. The disagreement is only about where vertical thrust goes.

### 10.4 Adopted, in order

1. **Mass-aware thrust** (`F·dt/m`, following `tractor_field.hpp`), because cargo, damage and spacecraft design
   should all change handling, and nothing else in the benchmark works without it.
2. **Relative velocity matching** on one key — the benchmark reports this as the single most-praised control in
   Outer Wilds, it costs little, and it belongs to the same family as the autopilot. It also fits the owner's
   reason for building José.
3. **Intelligent flight control**, Star Citizen's actual contribution: the player states intent and the flight
   computer resolves it into whatever the available thrusters can do. Everward is already shaped for this — the
   simulation owns authority and scales it by subsystem integrity — and it is how a damaged probe should fly.
4. **Register a planetary body for the Godot path**, which switches gravity on and makes landing, TWR and
   the ascent profile real rather than notional.
5. **Atmosphere** (density, drag, lift) once gravity is in, since reentry and aerodynamic ascent depend on it.

### 10.5 Deferred, and why

Reentry heating, stall modelling, orbital trajectory prediction and an advanced autopilot are all downstream of
items 1 and 4 above: none can be tuned, or even tested, against a probe whose thrust does not know its own mass.
The benchmark scores them 6-8 out of 10 and that ordering is accepted; they are simply not next.

One rule from the benchmark is adopted outright and should outlast every scheme in this document:

> The spacecraft should not suddenly switch physics models when it crosses an arbitrary atmospheric boundary.

## 9. Out of scope here

Joystick, HOTAS and gamepad hardware support (the binding system already has `DeviceKind`; wiring real devices is
separate), force feedback, head tracking, and per-scheme camera behaviour beyond what the existing camera exports
already allow.
