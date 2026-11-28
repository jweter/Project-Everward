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

## 9. Out of scope here

Joystick, HOTAS and gamepad hardware support (the binding system already has `DeviceKind`; wiring real devices is
separate), force feedback, head tracking, and per-scheme camera behaviour beyond what the existing camera exports
already allow.
