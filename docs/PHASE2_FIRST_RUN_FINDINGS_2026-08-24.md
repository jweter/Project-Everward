# Phase 2 First-Run Findings — 2026-08-24

## Result

The first serious local Unreal Engine 5.8 Phase-2 integration run completed successfully enough to validate the current One Probe engineering slice, but it did **not** satisfy the product gate that simply existing as EV-0001 is compelling.

Canonical evidence:

- `playtests/phase2/observations/phase2-first-run-20260824-225821.json`
- tested commit: `7f9ea88b8e7857f44f80b2f2327fde758dd2ca1a`
- engine: Unreal Engine 5.8

## Confirmed working

- Unreal C++ build and PIE launch;
- EV-0001 spawn and generated test environment;
- camera orbit and zoom;
- three-axis translation;
- full-stop command;
- subsystem power allocation;
- capability discovery/selection;
- sensor scan completion and cancellation;
- Basic Survival policy install/clear;
- 25 W computation execution gate.

The manual/automation shared-state check remains inconclusive because the attempted sensor retest did not produce an observable policy-triggered zeroing event. Do not classify this as a failure until retested under an explicit trigger condition with clearer feedback.

## Subjective ratings

| Dimension | Score |
| --- | ---: |
| Embodiment | 2/5 |
| HUD clarity | 2/5 |
| Control discoverability | 3/5 |
| Generation-1 clunkiness | 4/5 |
| Movement readability | 3/5 |
| Automation comprehension | 1/5 |
| Desire to continue | 1/5 |

The strong clunkiness score is important: primitive Generation-1 handling is not the main problem. The problem is that the player does not yet feel sufficiently like they are inhabiting and operating a spacecraft, and the machine's internal state/automation is too opaque.

## Highest-value findings

### 1. Add real attitude/orientation control

Current movement changes world-space X/Y/Z velocity. The probe cannot yaw, pitch, or roll, so motion feels on rails rather than like spacecraft flight.

Next pass should add authoritative orientation/attitude state and yaw/pitch/roll command surfaces, then make translational commands probe-relative. Preserve the full-stop command; it was specifically identified as useful for controlling the intentionally clunky starter machine.

### 2. Make power state continuously legible

The systems panel should show every installed subsystem's live allocation and state at once, for example:

```text
PROPULSION        40 W   READY
SENSORS           25 W   READY
COMPUTATION       30 W   READY
THERMAL CONTROL   20 W   READY
TOTAL            115 / 200 W
```

Where relevant, show reasons such as `BELOW MINIMUM`, `DISABLED BY POLICY`, `THERMAL LIMITED`, or other authoritative causes rather than forcing the player to infer them.

### 3. Make automation explain itself

Automation comprehension scored 1/5. When a policy acts, the HUD must show an explicit cause/effect message, e.g.:

```text
BASIC SURVIVAL -> SENSORS 0 W
reason: stored energy below 60%
```

The next retest of manual/automation shared state should deliberately force a known trigger and show whether automation and manual commands are acting on the same state.

### 4. Give subsystem power perceptible consequences

Sensors and Thermal Control accepted allocation changes but did not produce sufficiently obvious gameplay differences. Power should increasingly influence meaningful behavior such as scan speed/range/resolution/confidence and heat rejection/cooling headroom. Keep these effects grounded in the authoritative simulation rather than adding presentation-only bonuses.

### 5. Replace the sphere for the next serious feel test

The temporary sphere has completed its engineering-scaffold purpose. Further embodiment testing should use a recognizable Generation-1 Prime Probe A blockout.

The next playable representation should include:

- recognizable Prime Probe body/skin based on the canonical Generation-1 Scientific Explorer references;
- articulated manipulator assemblies rather than permanently decorative arms;
- at least two major arms with constrained shoulder/elbow/wrist/tool articulation;
- deliberately slow, mechanical deployment/motion appropriate to Generation 1;
- architecture that can later support grabbing, servicing, instruments, mining, and construction without treating the arms as cosmetic-only geometry.

A finished production art pass is not required for the next test. A mechanically credible, well-proportioned blockout is sufficient if it materially improves embodiment.

## Authorized next playable pass

Bundle the next work around one objective: **make EV-0001 feel like a primitive spacecraft-machine the player inhabits and understands.**

Priority order:

1. authoritative yaw/pitch/roll orientation and probe-relative translation;
2. preserve full stop and current intentionally clunky response;
3. persistent live subsystem power/status summary;
4. explicit automation cause/effect feedback and deterministic shared-state retest path;
5. perceptible sensor/thermal power consequences where the current model can support them cleanly;
6. Prime Probe A blockout/skin with articulated manipulator arms for the next embodiment test.

Do not use this result as justification to jump to Phase 3 astronomy, broad industry, replication, or unrelated systems. The current gap is embodiment, legibility, and control feel.
