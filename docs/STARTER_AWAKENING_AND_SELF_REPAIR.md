# Canonical Starter Awakening and Self-Repair

This document defines the intended opening progression loop for Everward and the first major use of the probe's **Self Repair** capability.

The opening should teach the player what it means to *be* the machine by making recovery itself the tutorial. The player does not begin as a fully functional spacecraft with every system available. The player awakens as a damaged autonomous probe and must reconstruct their own capability, piece by piece, until the machine is once again fully operational and able to leave the starter zone.

## Canonical opening state

The player awakens stranded on the surface of a moon or planet.

The probe is alive, but only barely functional:

- the computation/core system is operating at the minimum level required for consciousness and control;
- a damaged solar-power system and small battery provide only enough energy to prevent shutdown;
- propulsion and flight are unavailable;
- movement of the probe body is unavailable or effectively unusable;
- several systems are completely offline;
- several other systems operate only at low integrity or reduced capability;
- at least one mining/manipulator arm is partially functional;
- useful raw minerals exist within the arm's initial physical reach;
- the Self Repair capability can diagnose damage and consume power/materials to restore systems.

The scenario must always contain a valid recovery path. The player should feel fragile and constrained, not soft-locked.

## Core opening loop

```text
WAKE
-> DIAGNOSE
-> REACH
-> GATHER
-> SPEND POWER + MATERIAL + TIME
-> REPAIR A CAPABILITY
-> GAIN A NEW ACTION OR BETTER REACH
-> ACCESS BETTER MATERIALS
-> REASSESS
-> REPAIR THE NEXT CAPABILITY
-> RESTORE THE WHOLE PROBE
-> DEPART
```

This is not a detached tutorial layer. It is normal Everward gameplay under extreme early constraints.

Every repaired system should teach the player something because restoring it changes what the player can physically perceive or do.

Examples:

- restore Sensors enough to identify nearby material instead of collecting blindly;
- improve the manipulator to reach deposits that were previously inaccessible;
- restore processing so crude gathered material can become usable repair stock;
- restore power distribution so multiple systems can operate simultaneously;
- restore thermal control so higher-power repairs can run safely;
- restore attitude or maneuvering hardware before restoring full propulsion;
- restore propulsion last enough to prove mobility, then continue toward full flight readiness.

The intended rhythm is:

> **repair something -> learn/use the new capability -> reach the next resource/problem -> repair the next thing**

## Self Repair is a real machine capability

Self Repair is not passive health regeneration and should not be an abstract health-bar refill.

A repair consumes three real constraints from the beginning:

1. **materials** — structural, conductive, thermal, optical, electronic, or other feedstock as the material model becomes richer;
2. **energy** — repairs compete with survival, sensors, computation, thermal management, and other active loads;
3. **time** — reconstruction is a process, not an instantaneous menu action.

Later generations may also require or benefit from fabrication quality, specialized tools, spare parts, repair drones, better diagnostics, redundant hardware, or nanofabrication. Generation 1 should remain comparatively crude and understandable.

## Smart repair priority rule

The default repair planner follows one principle:

> **Restore capability before restoring perfection.**

It should not bring one functioning system from 5% to 100% while another strategically important system remains completely offline, unless doing so is required for immediate survival or to unlock the next repair.

The planner continuously reassesses after every completed repair step.

### Priority 1 — Preserve existence

Protect the minimum systems required to keep the probe alive and able to continue repairing:

- computation/core viability;
- minimum electrical power generation and storage;
- thermal safety;
- any repair/manipulator capability currently required to obtain resources.

An immediate survival threat outranks ordinary restoration.

### Priority 2 — Restore important offline capabilities

When survival is stable, prefer bringing an important `OFFLINE` system to its **minimum useful operating threshold** rather than polishing an already-working system.

The exact threshold can vary by hardware. The important rule is that the repair should create usable capability as soon as practical.

### Priority 3 — Bring the required machine online

Continue restoring required offline systems until every required starter system is operational at least minimally.

This creates breadth of capability before perfection.

### Priority 4 — Reassess the weakest functioning systems

Once every required system is online, reassess based on:

- current integrity;
- strategic importance;
- current mission needs;
- power/material cost;
- whether another repair unlocks additional resources or repair options.

The default planner should generally improve the most damaged/highest-value system first.

### Priority 5 — Repair in useful stages

Repair should proceed through useful functional bands rather than treating integrity as only `broken` or `100%`.

A provisional progression can be:

```text
OFFLINE
-> MINIMUM OPERATIONAL
-> ~25%
-> ~50%
-> ~75%
-> 100%
```

These percentages are design targets, not final balance constants. Individual systems may have meaningful thresholds at different values.

### Priority 6 — Repeat until full restoration

After each repair, the planner runs the priority assessment again. A changing energy reserve, newly available material, newly restored sensor information, or newly accessible deposit may change the correct next action.

## Player agency

The smart planner should recommend and automate sensible repair priorities, especially for a damaged Generation-1 machine, but the player ultimately controls their own body.

The player should be able to inspect:

- what the repair planner wants to repair next;
- why that system has priority;
- required material;
- required energy;
- expected repair time;
- expected post-repair integrity/capability;
- what new action or improvement the repair is expected to unlock.

As the interface matures, the player may override the recommendation and choose a different repair target. A strategically poor choice is allowed if it does not violate an explicit hard safety rule or difficulty-mode constraint.

## Repair planner example

A possible damaged awakening state:

```text
CORE / COMPUTATION ......... 8%   CRITICAL / ONLINE
SOLAR COLLECTION .......... 32%   ONLINE
BATTERY ................... 11%   ONLINE
MINING MANIPULATOR ........ 17%   DEGRADED
SENSORS .................... 0%   OFFLINE
THERMAL CONTROL ........... 14%   DEGRADED
PROCESSING ................. 0%   OFFLINE
ATTITUDE CONTROL ........... 0%   OFFLINE
PROPULSION ................. 0%   OFFLINE
```

The first optimal action does not have to be the same in every generated start, but the planner might choose to restore Sensors to minimum operation because that immediately improves material identification. Later it may restore processing, improve the manipulator's reach, restore power distribution, and eventually recover maneuvering and propulsion.

The important behavior is the policy, not a scripted fixed order.

## Starter-zone resource design

The immediate environment must support a layered recovery path:

- **Tier 0:** material within initial arm reach, sufficient for the first repair;
- **Tier 1:** material identifiable or reachable after the first capability restoration;
- **Tier 2:** material requiring better sensing, reach, processing, or power;
- **Tier 3:** material/components needed for major mobility and propulsion restoration.

This turns the local terrain into a progression puzzle grounded in the probe's physical capabilities rather than invisible quest gates.

Resource placement may be authored for the canonical opening or procedurally generated under strict solvability constraints. Either way, the simulation should be able to prove that a valid recovery chain exists.

## Departure readiness

The canonical starter zone ends when the player has restored the probe completely enough to leave under its own power.

For the first version of this opening, the departure gate is intentionally demanding:

- every required starter subsystem is online;
- every required starter subsystem has reached **100% integrity**;
- power generation/storage is stable;
- thermal state is safe;
- propulsion and attitude control are fully available;
- the probe can demonstrate controlled movement/flight.

The HUD should make this a visible machine-state milestone rather than a hidden quest flag.

Example:

```text
DEPARTURE READINESS

CORE ................ 100%
POWER ............... 100%
THERMAL ............. 100%
SENSORS ............. 100%
MANIPULATOR ......... 100%
PROCESSING .......... 100%
ATTITUDE CONTROL .... 100%
PROPULSION .......... 100%

FLIGHT READINESS: CONFIRMED
```

The first successful departure from the surface should be treated as a major emotional and visual payoff: the player has not merely unlocked a vehicle; they rebuilt their own body well enough to leave the place where they awakened.

## Self Repair progression beyond the opening

Self Repair remains a permanent upgradeable capability after the starter zone.

Possible progression dimensions include:

- diagnosis accuracy;
- repair speed;
- energy efficiency;
- material efficiency;
- material substitution;
- number of simultaneous repairs;
- accessible components;
- repair under hazardous conditions;
- predictive maintenance;
- autonomous repair policy sophistication;
- redundant/reconfigurable hardware;
- robotic repair swarms;
- advanced fabrication and reconstruction.

A later probe should feel dramatically more capable at surviving and rebuilding itself than Generation 1 without turning damage into meaningless instant regeneration.

## Architecture implications

The system should build on the same component model used for collision, impact, power, capabilities, fabrication, and descendants:

```text
physical event / preexisting damage
-> component integrity + failure state
-> reduced or missing capability
-> diagnostic information
-> material + energy + time requirement
-> repair action
-> component integrity restored
-> capability restored or improved
```

Repair priority belongs in authoritative gameplay logic, not only in the HUD. Unreal presents the diagnosis, planner, animation, and visible reconstruction but should not become the sole owner of repair truth.

## Roadmap placement

This opening depends on several systems already present in or planned for the Phase 2–4 sequence:

- physical bodies/contact;
- physically grounded damage and component impairment;
- Prime Generation-1 component structure;
- articulated mining/manipulator arms;
- physical object/material acquisition;
- surface/near-surface operation;
- resource/sample storage;
- processing/fabrication;
- repair/recoverability.

The full canonical awakening should therefore be assembled after those foundations exist, while each prerequisite should be designed so it can support this opening rather than requiring a later rewrite.

The master roadmap's vertical slice and eventual Steam demo should both use this damaged-awakening recovery loop as the canonical opening experience.
