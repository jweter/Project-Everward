# Everward Project Status

This file is the durable operational continuation record for human and scheduled development. It answers: **where are we now, what is verified, what remains open, and what should happen next?** Detailed historical implementation narratives belong in Git history, PRs, playtest evidence, and `ERROR_RESOLUTION_LEDGER.md`.

## Current phase

**Phase 2 — One Probe: ACTIVE, first local integrated playtest complete.**

Phase 1 is complete and Unreal Engine 5.8 is the accepted production direction. The first serious local Phase-2 run has now exercised the current One Probe slice in Unreal and produced structured evidence.

Canonical first-run evidence:

- `playtests/phase2/observations/phase2-first-run-20260824-225821.json`
- `docs/PHASE2_FIRST_RUN_FINDINGS_2026-08-24.md`
- tested commit: `7f9ea88b8e7857f44f80b2f2327fde758dd2ca1a`

The engineering integration is substantially functional, but the product gate **"simply existing as the probe is compelling"** is not yet met. The next gate is therefore not another unrelated subsystem. It is an embodiment/control/legibility pass driven directly by first-run evidence.

## First-run result

Confirmed working locally in Unreal Engine 5.8:

- Unreal C++ build and PIE launch;
- deterministic EV-0001 spawn and generated Phase-2 environment;
- camera orbit/zoom;
- X/Y/Z authoritative translation;
- full-stop command;
- capability discovery/selection;
- power allocation across Propulsion, Sensors, Computation, and Thermal Control;
- sensor scan completion and cancellation;
- Basic Survival policy install/clear;
- computation execution gate below/above 25 W.

Manual/automation shared-state behavior remains **inconclusive**, not failed: the attempted Sensors check did not produce an observable policy-triggered zeroing event, likely because the test did not guarantee the policy trigger condition.

### Subjective ratings

- embodiment: **2/5**
- HUD clarity: **2/5**
- control discoverability: **3/5**
- Generation-1 clunkiness: **4/5**
- movement readability: **3/5**
- automation comprehension: **1/5**
- desire to continue: **1/5**

The strong clunkiness score is useful evidence: primitive starter-probe handling is not the primary problem. The main deficits are spacecraft embodiment, attitude control, subsystem-state legibility, and understandable automation.

## Current embodiment continuation

The first evidence-driven control slice now adds engine-independent authoritative yaw/pitch/roll state, observable attitude commands, and probe-relative translation. Unreal renders the resulting authoritative rotation and continues to submit movement through `UProbeSimulationAdapter`; it does not own flight truth.

The temporary engineering controls use J/L for yaw, I/K for pitch, U/O for roll, and retain W/S, A/D, E/Q as local forward/lateral/vertical trims. Space remains an absolute full stop. This is deliberately a command-driven Generation-1 flight model, not a claim that final rigid-body/thruster physics is solved.

After CI, the next local UE 5.8 evidence pass should verify orientation, visual rotation, local translation, full stop, and control feel. If that passes without a correctness blocker, the next executable slice is the persistent all-subsystem power/status HUD from the same first-run evidence.

## Current authoritative foundation

The project still preserves these production rules and implemented foundations:

- engine-independent C++20 simulation under `src/simulation/`;
- deterministic fixed-step simulation time;
- canonical EV-0001 state for identity, generation, mass, position, velocity, energy, thermal state, storage, capabilities, and subsystem power;
- scan lifecycle and power-budget validation;
- energy/thermal lockouts and recovery hooks;
- engine-independent Generation-1 `ProbeRuntime` software-policy evaluator;
- manual and automated actions converging on authoritative mechanics;
- capability-driven Unreal HUD/control shell and `UProbeSimulationAdapter` boundary;
- adjacent-generation evolution foundation;
- canonical Prime Probe A reference package with provenance validation in Foundation CI.

## Highest-value product findings

### Spacecraft attitude and local-space movement

EV-0001 currently translates along world axes and cannot yaw, pitch, or roll. This feels on rails rather than like flying a spacecraft.

The next playable pass should add authoritative orientation/attitude state plus yaw/pitch/roll controls, then make translation probe-relative. Preserve the full-stop command; it was specifically identified as useful and enjoyable with the intentionally clunky Generation-1 handling.

### Persistent subsystem power/status telemetry

The HUD currently makes the player inspect systems one at a time. The expanded systems panel should show live watts and operational state for all installed subsystems simultaneously, plus total allocation/capacity.

Where possible, state should include an authoritative reason such as `BELOW MINIMUM`, `DISABLED BY POLICY`, or `THERMAL LIMITED` rather than leaving the player to infer why a value changed.

### Explain automation cause and effect

Automation comprehension scored 1/5. When a software policy acts, the player should see what policy acted, what it changed, and why. The next build should include a deterministic retest path that guarantees a Basic Survival trigger so manual/automation shared-state behavior can be proven in one session.

### Perceptible subsystem consequences

Sensors and Thermal Control accept power changes but their gameplay effects are not yet obvious enough. Future allocation changes should produce physically grounded, authoritative consequences rather than presentation-only feedback.

### Prime Probe embodiment for the next serious test

The temporary sphere has completed its engineering-scaffold purpose. The next serious embodiment test should use a recognizable Generation-1 Prime Probe A blockout/skin based on the canonical Scientific Explorer reference set.

The next representation should include at least two articulated manipulator arms with constrained shoulder/elbow/wrist/tool motion. They may begin as a blockout and simple deployment/pose system, but they should be architected as real future capabilities for servicing, instruments, grabbing, mining, and construction rather than permanent decorative geometry.

## Authorized next work

Priority order:

1. record and merge the first local playtest evidence;
2. verify the implemented authoritative orientation/attitude state and yaw/pitch/roll command surfaces in local UE 5.8;
3. verify probe-relative translation preserves full stop and Generation-1 clunkiness;
4. improve the systems HUD with persistent per-system live power/status and total power context;
5. make automation actions/cause visible and provide a deterministic shared-state retest path;
6. add physically meaningful sensor/thermal power feedback where supported by the current simulation model;
7. integrate a recognizable Prime Probe A blockout/skin with articulated manipulator arms for the next embodiment test;
8. repeat the same subjective ratings and compare against the first-run baseline before expanding scope.

Do not jump ahead to Phase 3 astronomy, broad industry, replication, aliens, combat, megastructures, or unrelated procedural content before the One Probe embodiment/control loop becomes compelling enough to justify expansion.

## Open product/architecture decisions

### Final Generation-1 flight model

The next pass should establish real attitude/orientation and probe-relative translation, but it does not need to pretend the final thruster/rigid-body model is solved. Keep commands authoritative, deterministic, and intentionally primitive while avoiding world-axis rail feel.

### Final programming interface

The two-rule Basic Survival policy remains an architectural proof, not the final player-facing programming system. Richer scripts, priorities, diagnostics, and compute-scaled automation remain later work. Manual and automated actions must continue to converge on shared authoritative commands.

### Failure and repair semantics

`set_subsystem_operational()` remains a deterministic hook, not a final wear/damage model. Real failure causes, health representation, and repair semantics remain open until the industrial/recovery loop can support them.

## Phase 2 production rules

- Simulation owns mechanical truth.
- Unreal consumes authoritative state and submits commands through the adapter boundary.
- `src/simulation/` remains buildable/testable without Unreal dependencies.
- Manual and automated controls must share authoritative mechanics.
- Canonical simulation units stay engine-independent; Unreal conversion happens at the presentation boundary.
- Deterministic headless execution remains required.
- Save data remains an explicit versioned schema rather than blind Unreal object serialization.
- Later descendants expose capabilities from installed hardware/software, not from a universal player ability list.
- Better computation may enable richer automation/design capability, but cannot bypass physical hardware, materials, manufacturing, or known science.

## Visual product constraint

Everward must not drift toward a primarily 2D, abstract-map, low-poly, deliberately quirky, or visually lightweight interpretation because it is easier to implement. The target remains cinematic, immersive, high-fidelity 3D scientific realism.

The temporary Phase-2 sphere, markers, labels, and light are engineering scaffolding only and should no longer be treated as sufficient for serious embodiment evaluation.

## Automation operating state

Scheduled development follows `AGENT_DEVELOPMENT_POLICY.md`:

1. inspect open PRs/CI first;
2. repair red work before starting new work;
3. merge only independently verified green and merge-ready work;
4. otherwise advance one highest-value authorized slice;
5. keep affected documentation current;
6. preserve accepted design/architecture decisions.

## Repository posture

- visibility: **Public by deliberate operational choice**;
- IP: **Proprietary, all rights reserved**;
- public visibility does not grant an open-source license;
- default branch: `main`;
- substantive development: branch + pull request, not direct-to-main.
