# Everward — Landing, take-off, and the stranded awakening

**Status:** Design, owner-directed 2026-11-28. Not yet implemented.
**Parent:** #378 (first playable loop), #384 (guidance). Relates to `docs/PHASE2_VERTICAL_SLICE_PLAN.md`.
**Owner direction, verbatim:**

> we need to be able to lan[d] the probe on the ground and take off again. Initial moment of awaking you are
> stranded, low power, you test your controls and find you are luckily on top of a minable resource. All your need
> to do is turn on the miner tool then fix it comes in handy and starts fixing your probe systems enough for basic
> flight to get to the next mining node.

## 1. What this changes

The opening stops being "a damaged probe that can already fly" and becomes "a probe that cannot fly yet, sitting on
the one thing that can fix it". That is a better tutorial than any prompt: the player is taught the controls by
having only a few that work, and the first thing they learn is the thing that saves them.

It gives the damaged awakening a floor. Today a freshly woken probe can fly (badly) and auto-approach (until
tonight, at full speed — see `#378 G10`). Under this design flight is something you **earn back** with the first
load of ore.

## 2. What already exists

Checked in the code, not assumed:

- **Descent shaping is already in the core and unused.** `src/simulation/include/everward/simulation/surface_descent_guidance.hpp`
  (Slice 10) constrains a requested velocity to a controlled-descent envelope, reduces descent speed as clearance
  closes (`full_speed_altitude_m` 25.0, `touchdown_descent_speed_mps` 0.5), prevents commanded inward velocity at
  minimum clearance, and offers a hover profile (`target_altitude_m` 5.0). It explicitly does **not** model
  thruster authority or move the probe. **None of it is exposed through the GDExtension**, so the Godot path has
  never used it.
- **Mining does not require an approach.** `mine_deposit` is gated by the simulation on survey state and the tool
  surface gap. The Godot `aligned` flag is bookkeeping for the HUD, not a precondition. A probe resting within arm
  reach of a deposit should already be able to mine it without flying anywhere.
- **Propulsion integrity already scales what the drive can do**: the player's thrust and attitude trim
  (`impact_damage.hpp`), the scan (sensors), and now the autopilot.
- **Terrain contact exists** as three 40 m proxy spheres tracking the voxel surface near the probe, with swept
  compound-hull contact resolution (#378 G07). Contact stops the probe; nothing yet calls it "landed".

So the missing pieces are a *resting state*, a *take-off gate*, and the *scenario*.

## 3. Design

### 3.1 Landed

"Landed" is a state the simulation owns, not a Godot animation. The probe is landed when, for a continuous
`LANDED_SETTLE_S`, it is in contact with the surface and its body-relative speed is below `LANDED_SPEED_MPS`.

| Tunable | Start value | Why |
|---|---|---|
| `LANDED_SPEED_MPS` | 0.15 m/s | slow enough to exclude a slide or a bounce |
| `LANDED_SETTLE_S` | 0.5 s | one touch is not a landing |
| `TAKEOFF_CLEARANCE_M` | 1.0 m | how far up counts as airborne again |

While landed:

- the probe does not drift: residual tangential velocity is bled off rather than held, so it stays where it was put;
- the arms may be deployed and used (this is the point);
- the HUD says `LANDED`, and says what is under the probe.

### 3.2 Take-off

Take-off is the first thing propulsion is *for*, so it is gated the same way everything else is:

- With propulsion integrity at or below `TAKEOFF_MINIMUM_INTEGRITY` (start value **0.15**), lift is refused and
  says why, pointing at Fix_It. Below that figure the drive cannot lift the hull against the body at all.
- Above it, the available climb rate scales with integrity, exactly as thrust and the autopilot already do. A 20%
  probe lifts off slowly; a repaired one leaves briskly.
- Landing uses the existing descent envelope: descent speed is limited by altitude and tapers to
  `touchdown_descent_speed_mps` at the surface, so a landing is a landing and not an impact. Arriving faster than
  the envelope allows is a contact event with damage, which is already modelled.

### 3.3 The awakening

The probe wakes **landed, stranded, and on a resource**:

1. Propulsion integrity starts below `TAKEOFF_MINIMUM_INTEGRITY`. The probe cannot fly. This is stated, not hidden.
2. A surveyed-on-contact or scannable deposit lies within arm reach of where it rests — no flight needed to work it.
3. The only useful actions are: look around, read the systems panel, deploy an arm, attach the tool, mine.
4. Fix_It consumes the mined material and restores propulsion past the take-off floor.
5. Take-off becomes available, and the next node is somewhere the player must fly to.

The guidance cue from #384 carries this: `OBJECTIVE — Restore flight`, `NEXT: attach the mining tool [Y]`,
`Why? Fix_It needs material to repair propulsion`.

### 3.4 What must not happen

- The probe must not be *silently* stranded. "Nothing happens when I press P" is a bug; "propulsion offline, repair
  it with Fix_It" is the game.
- Landing must not become a cutscene or a snap-to-ground. It is the existing contact model plus a settle test.
- The deposit under the probe must not be a special case in the mining code. It is an ordinary deposit that happens
  to be in reach, which is why §2 matters.
- No new way to move the probe. Take-off uses the same propulsion-limited path as manual flight.

## 4. Implementation slices

1. **Expose descent guidance and a landed state.** Bind the existing `surface_descent_guidance.hpp` helpers through
   the GDExtension; add the landed test (contact + speed + settle) to the simulation side; parity test for both.
2. **Take-off gate and climb scaling**, with the refusal message, plus a `--landingtest` acceptance mode: land from
   a height within the envelope, confirm landed, fail to take off below the integrity floor, repair, take off.
3. **The stranded awakening scenario**: probe starts landed on a deposit with sub-floor propulsion; `--firstloop`
   gains the grounded first mine before any flight.
4. **Guidance** (#384): the objective cue naming the restore-flight goal and the next action.

## 5. Out of scope here

Atmospheric flight model, landing gear or legs as articulated parts, choosing a landing site from orbit, fuel,
re-entry heating, and any landing animation. This is the minimum that makes the owner's opening playable and honest.
