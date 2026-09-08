# José Take the Wheel — Smart Autopilot

## Canonical status

**José Take the Wheel** is a canonical Everward navigation and accessibility feature.

The name is an original in-world label for the probe's friendly automatic-driving assistant. It is **not intended as a reference to Jesus, religion, any religious figure, any real person, or any pre-existing fictional character**.

The player-facing idea is simple:

> If the player knows where they want to go but does not want to manually fly the probe there, they can call José and hand over the wheel.

José is a smart autopilot and a friendly hand on the controls, not a separate character that overrides the player.

## Player contract

The long-term interaction should remain easy to understand:

```text
choose destination
-> "José Take the Wheel"
-> autopilot plans and flies the route
-> player observes, scans, manages systems, or simply rides along
-> José arrives safely and returns control
```

At any time the player can take control back immediately.

Manual control always outranks autopilot.

José must therefore support:

- deliberate player-selected destinations;
- one-action engage/cancel behavior;
- immediate manual takeover;
- safe arrival rather than collision with the destination;
- readable current destination and autopilot state;
- understandable failure/rejection messages;
- no teleportation or hidden movement cheats;
- the same authoritative propulsion/physics rules used by manual flight.

## Why this belongs in Everward

Everward intentionally makes physical movement meaningful, but physical controls should not become a barrier to exploration.

José supports several legitimate play styles:

- a player who enjoys engineering/scanning but not six-axis flight;
- a player who has difficulty with the current controls;
- a player who already knows the destination and wants to focus on another system during transit;
- a player who simply does not want to manually solve every point-A-to-point-B movement problem.

This is both an accessibility feature and a quality-of-life automation feature.

It preserves the **You Are the Probe** pillar because the player's machine still physically travels through space. The player is delegating navigation software, not leaving the probe or becoming an external empire controller.

## Physics rule

José must obey Everward's simulation truth.

It does not:

- teleport;
- ignore propulsion capability;
- pass through solid bodies;
- create free energy;
- ignore thermal constraints;
- magically stop from impossible velocities;
- provide instantaneous interstellar travel.

As propulsion, orbital mechanics, hazards, power, heat, collision avoidance, and navigation become more physically complete, José should use those same systems.

A sufficiently damaged probe may therefore have a degraded or unavailable José capability if the systems required to navigate safely are offline.

## Phase-2 first playable implementation

The first implementation deliberately reuses the existing physical target-selection system rather than creating a second destination registry.

Current interaction:

```text
T       cycle/select a physical destination
Y       engage/cancel "José Take the Wheel"
SPACE   stop and immediately take control back
WASDQE  manual translation input immediately takes control back
```

Phase-2 guidance behavior:

- locks the currently selected physical target as José's destination;
- commands movement through `UProbeSimulationAdapter::CommandSetVelocityMetersPerSecond()`;
- aims toward the live position of the selected simulation body;
- uses the authoritative selected-target surface range as the arrival metric;
- progressively slows as the destination is approached;
- stops at a configurable safe surface stand-off instead of driving into the body;
- disengages if the selected destination changes or becomes unavailable;
- disengages if the authoritative propulsion command is rejected;
- displays a persistent discoverability/status line in the current Product Reality HUD.

Initial tuning values are implementation details rather than permanent balance rules:

- cruise speed: 6 m/s;
- arrival surface stand-off: 20 m;
- arrival tolerance: 0.5 m;
- proportional approach gain: 0.35/s.

These values should evolve with the real propulsion and navigation model.

## Intended evolution

The player-facing contract should stay stable while José becomes increasingly capable.

### Local navigation

José should eventually support:

- point-to-point local movement;
- safe approach to asteroids, stations, storage, refineries, and other machines;
- selectable stand-off distance;
- automatic orientation for arrival;
- obstacle/collision avoidance;
- braking-distance prediction;
- moving-target interception;
- docking and rendezvous assistance;
- mining-work-position approach;
- tractor/material-handling approach positions.

### Orbital/system navigation

As the world model matures, José should add:

- orbital intercept planning;
- transfer trajectories;
- arrival-orbit selection;
- fuel/energy-aware route choice;
- thermal/radiation hazard avoidance;
- restricted-zone avoidance;
- ETA and resource estimates;
- route replanning when conditions change.

### Interplanetary/interstellar navigation

At larger scale José becomes a navigation planner rather than a local steering loop:

- select planet, moon, station, probe, waypoint, or star as destination;
- choose or recommend trajectory;
- account for propulsion capability and available resources;
- execute burns and coast phases;
- allow time acceleration where valid;
- warn when the current body cannot safely complete the trip;
- preserve communication latency and physical travel time.

José should never erase Everward's scale. The purpose is to remove unwanted piloting burden, not the consequences of distance.

## Relationship to other automation

José is specifically the probe's **navigation/autopilot layer**.

It complements but is distinct from:

- `Fix_It` — repair, replacement, diagnosis, and evolution adviser;
- mining auto-approach — specialized positioning for the selected manipulator/mining workflow;
- worker/drone task automation — delegated industrial work;
- doctrine control — civilization-scale intent and policy.

Over time these systems may cooperate. For example, a mining task can ask José to reach the work area, then hand control to a mining-positioning routine.

## Player-agency rule

José never traps the player in autopilot.

Any direct manual translation request must immediately release the autopilot. Emergency stop must always remain globally available.

The UI should make it obvious who currently has the wheel:

```text
JOSÉ TAKE THE WHEEL // ENGAGED // <destination>
```

or

```text
JOSÉ TAKE THE WHEEL // [T] SELECT DESTINATION // [Y] ENGAGE
```

## Acceptance criteria for the first slice

The Phase-2 feature is considered Product Reality ready when a laptop playtest demonstrates that the player can:

1. select the current physical target with `T`;
2. press `Y` and observe José begin moving the actual probe;
3. see the current José state/destination on screen;
4. approach while slowing rather than striking the target at cruise speed;
5. stop at the configured stand-off distance;
6. press `SPACE` during transit and immediately regain control;
7. use manual translation during transit and immediately regain control;
8. change/lose the destination without the autopilot continuing blindly;
9. receive a useful message if propulsion/navigation cannot continue.

Future navigation sophistication should preserve this simple player promise:

> **Pick where you want to go. José gets you there.**
