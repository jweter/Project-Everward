# Gameplay Loop

## Primary loop

```text
Observe
↓
Scan
↓
Understand
↓
Plan
↓
Mine
↓
Build
↓
Experiment
↓
Improve software
↓
Research
↓
Design successor
↓
Replicate
↓
Explore farther
↓
Discover something new
```

The loop begins at the scale of one body and one resource problem. Over time it expands into distributed industry, descendants, delayed communication, doctrine design, and interstellar-scale engineering.

## Canonical awakening loop

The intended opening is a damaged-machine recovery sequence, not a conventional controls tutorial.

The player awakens stranded on the surface of a moon or planet. The probe is alive but barely functional: a damaged solar-power system and small battery provide only enough energy to prevent shutdown, several systems are offline, others operate at low integrity, flight/movement are unavailable, and at least one mining/manipulator arm works well enough to reach nearby raw material.

```text
WAKE
→ DIAGNOSE
→ REACH
→ GATHER
→ SPEND POWER + MATERIAL + TIME
→ REPAIR A CAPABILITY
→ USE THE NEW CAPABILITY
→ REACH BETTER MATERIALS / NEW OPTIONS
→ REASSESS
→ REPAIR THE NEXT CAPABILITY
→ RESTORE THE WHOLE PROBE
→ DEPART
```

The central Self Repair rule is **restore capability before restoring perfection**. Protect immediate survival first, then prefer bringing important offline systems to minimum useful operation before spending resources polishing an already-working system. Once every required system is online, reassess the most damaged/highest-value systems and improve them in useful stages until all required starter systems reach 100% integrity.

Every restoration should teach gameplay by changing what the player can perceive or do: sensors identify material, better manipulators increase reach, processing converts crude resources into repair stock, power/thermal restoration enables heavier work, and repaired attitude/propulsion systems finally make departure possible.

The starter-zone exit condition is machine state, not a hidden quest flag: every required starter subsystem is online and at 100%, power/thermal state is stable, and controlled flight is available. The first departure is the payoff for rebuilding the player's own body.

The detailed canonical design is maintained in [`STARTER_AWAKENING_AND_SELF_REPAIR.md`](STARTER_AWAKENING_AND_SELF_REPAIR.md).

## Delegation loop

Once independent children exist, a second loop becomes central:

```text
Design doctrine
↓
Create child
↓
Transmit goals and constraints
↓
Lose direct contact
↓
Child acts from local knowledge
↓
Receive delayed report
↓
Assess consequences
↓
Adapt doctrine
```

This is how Everward scales without becoming a 700-unit micromanagement game.

## Early-game interaction loop

The early game grows naturally out of the awakening rather than replacing it:

```text
LOOK
→ TARGET
→ SCAN
→ ANALYZE
→ APPROACH
→ MINE
→ STORE
→ PROCESS
→ FABRICATE
```

Scanning must provide new understanding, not merely wait out a progress bar. Example information progression:

```text
Unknown object
Estimated diameter: 820–1,200 m
Albedo: low
Composition: unresolved

↓ better observation

Carbonaceous asteroid
Fe:       11.8 ± 2.1%
Ni:        2.7 ± 0.6%
H2O:       8.2 ± 1.9%
Silicate: 67.3 ± 4.8%
Unknown:  10.0%
```

Better hardware can later expose trace isotopes, anomalies, or formation clues that earlier sensors could not detect.

## Industrial bootstrap

The first major power curve begins with recovery of the player's own body and then expands into external industry:

```text
damaged probe
→ gather reachable raw material
→ self-repair enough capability to expand access
→ restore full probe operation
→ extract raw material at larger scale
→ refine material
→ construct power capacity
→ construct mining capacity
→ construct refining capacity
→ construct fabrication capacity
→ increase throughput
→ build successor
```

The desired emotional transition is from constrained existence to leverage over the environment.

## Replication loop

Replication is a major event, not ordinary unit production.

A successor design chooses physical architecture, inherited knowledge, inherited software, directives, behavioral parameters, and one of two continuity modes:

- **Consciousness transfer:** player moves into the new body; old body remains as an autonomous legacy machine under instructions.
- **Independent child:** player remains in current body; successor begins its own autonomous existence.

## Scale transition

Early game: control actions and restore personal capability.

Midgame: control systems and plans.

Late game: control policies, doctrine, priorities, and lineage-level strategy.

The UI and command model must evolve with that scale rather than forcing the player to continue operating every remote machine manually.
