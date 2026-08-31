# Unbounded Machine Control and Industrial Scale

## Canonical status

This document defines a core Everward control and scaling doctrine.

Everward must not impose an arbitrary gameplay cap on the number of probes, worker drones, storage bins, depots, recycling centers, fabricators, refineries, construction units, relay stations, or other player-built machine infrastructure.

If the player can supply the required matter, energy, fabrication capability, space, transport, maintenance, time, and communication, the game should allow continued expansion.

The practical constraints are physical, logistical, computational, and informational — not a designer-authored `MAX_UNITS = 50` rule.

This doctrine extends:

- `ROADMAP.md`;
- `FIX_IT_UPGRADE_AND_DRONE_ECOLOGY.md`;
- `RECYCLING_CENTER_AND_CLOSED_MATERIAL_LOOP.md`;
- `MINING_MATERIALS_REPLICATION_DOCTRINE.md`;
- `PERFORMANCE_BUDGETS.md`;
- `SIMULATION_PHILOSOPHY.md`.

## Core fantasy

The control progression should feel like:

```text
I control my arm.
-> I control my first worker drone.
-> I assign tasks to groups of machines.
-> I control factories and logistics networks.
-> I define industrial objectives and operating policies.
-> I write doctrines my descendants and autonomous systems follow.
-> I eventually create a civilization too large and distant for immediate direct control.
```

That final state is not a UI failure. It is an intended Everward outcome.

The player begins as one embodied machine and may eventually become the origin of a physically distributed machine civilization.

## No arbitrary population or infrastructure ceiling

There is no canonical hard unit cap for gameplay purposes.

The player should be able to build, subject to simulation constraints:

- one mining drone or thousands;
- one storage bin or a distributed storage network;
- one recycling center or regional reclamation infrastructure;
- one fabricator or a system-scale manufacturing network;
- one successor probe or an enormous lineage;
- one industrial site or many independently operating systems.

The game may enforce temporary technical safeguards during development, but these must not become the intended player-facing progression ceiling.

Scaling pressure should instead emerge from:

- available matter;
- energy generation and distribution;
- heat rejection;
- fabrication throughput;
- processing throughput;
- storage capacity;
- transport distance and bandwidth;
- maintenance demand;
- communication latency;
- coordination complexity;
- environmental danger;
- obsolete hardware burden;
- local resource depletion;
- computation and simulation cost.

## Hierarchical command architecture

The player must always be able to drill down to an individual machine when physically and informationally possible, but must never be required to micromanage every machine once the civilization grows.

A canonical hierarchy may resemble:

```text
PRIME / CURRENT PLAYER
|
+-- Home-System Industrial Network
|   |
|   +-- Mining Group Alpha
|   |   +-- Miner 001
|   |   +-- Miner 002
|   |   +-- Miner 003
|   |
|   +-- Logistics Group
|   |   +-- Haulers
|   |   +-- Cargo Depots
|   |
|   +-- Survey Group
|   |   +-- Scanner Drones
|   |   +-- Navigation / Relay Assets
|   |
|   +-- Maintenance Network
|   |   +-- Repair Drones
|   |
|   +-- Industrial Complex
|       +-- Refineries
|       +-- Fabricators
|       +-- Recycling Centers
|       +-- Storage
|
+-- Expeditionary Lineage
    +-- Child Probe A
    +-- Child Probe B
    +-- Child Probe C
```

The exact hierarchy is player-configurable and may evolve. It is an organizational control model, not a mandatory fixed faction tree.

## Four scales of player control

### 1. Direct control

Early-game embodied control.

The player directly operates the current probe:

- movement;
- attitude;
- target selection;
- manipulator arms;
- tool attachment;
- scanning;
- mining;
- repair interaction;
- docking;
- local construction.

This level establishes the physical reality of the machine.

### 2. Task control

The first worker machines should accept concrete objectives such as:

- mine this target;
- scan this region;
- haul this cargo;
- repair this machine;
- move this wreck to recycling;
- maintain this depot;
- construct this structure;
- patrol this volume.

The worker determines low-level execution using its local software, sensors, design, power, tools, and current knowledge.

### 3. Operational control

Once the player has dozens or hundreds of machines, control moves toward groups and systems.

The player should be able to define:

- operating areas;
- task groups;
- routes;
- stockpile targets;
- production quotas;
- priority materials;
- reserve margins;
- maintenance thresholds;
- repair policies;
- retirement policies;
- recycling rules;
- hazard limits;
- construction priorities;
- resource allocation.

Example:

```text
MINING GROUP ALPHA
Objective: Maintain iron reserve above 25,000 kg
Area: Inner Belt Sector 04
Allowed targets: surveyed Fe-rich bodies
Return condition: cargo > 85% or energy reserve < 25%
Maintenance: service below 75% condition
```

### 4. Doctrine control

At machine-civilization scale, the player primarily controls intent and policy.

Example:

```text
HOME-SYSTEM MINING DOCTRINE

Primary objective:
Maintain strategic reserves.

Priority materials:
1. Iron
2. Nickel
3. Carbon
4. Water

Minimum reserve:
Iron: 50,000 kg
Nickel: 15,000 kg

Worker policy:
Repair below 75% condition.
Evaluate retirement below 40% if repair is uneconomical.

Recycling policy:
Salvage intact components first.
Recycle remaining structure.
Preserve historically significant machines unless explicitly overridden.

Expansion policy:
Add mining capacity when sustained demand exceeds sustainable throughput.
```

The player is still in control, but controls the rules under which the civilization operates rather than manually piloting every worker.

## Intent over movement

Everward should preserve a clear distinction between authority and micromanagement.

At high scale, the player issues **intent**:

> Prioritize construction of the successor probe.

The industrial system may then coordinate:

- mining;
- hauling;
- recycling;
- refining;
- storage;
- power allocation;
- fabrication;
- repair scheduling;
- construction sequencing.

The player should be able to inspect why the system chose those actions and override them when desired.

## `Fix_It` as civilization-scale adviser

`Fix_It` expands naturally from repairing one failing probe into evaluating a distributed industrial ecology.

It should eventually inspect telemetry from:

- the current player body;
- worker drones;
- descendants where communication permits;
- storage nodes;
- mining operations;
- transport networks;
- power infrastructure;
- refineries;
- fabricators;
- recycling centers;
- repair facilities;
- construction networks.

`Fix_It` should rank bottlenecks and explain recommended interventions.

Example:

```text
Priority 1 — Logistics bottleneck

Mining output exceeds hauling capacity by 18.4%.
Seven miners average 24% idle time waiting for pickup.

Recommended:
Build three additional haulers.

Alternative:
Expand Depot-04 and reroute Mining Group Beta.
```

Another example:

```text
Priority 2 — Obsolete miner family

Generation-2 miners consume 41% more energy per kilogram extracted
than Generation-5 units.

18 units remain operational.

Recommendation:
Retire progressively as replacements become available.
Recover reusable components before recycling the remaining mass.
```

`Fix_It` recommends. The player decides.

## Storage has physical location

Storage is not a global inventory counter.

Every storage node should have, where relevant:

- physical location;
- capacity by mass and/or volume;
- permitted material classes;
- loading/unloading interfaces;
- access constraints;
- power or thermal requirements where justified;
- transport distance;
- throughput;
- damage state;
- maintenance state.

A civilization with enormous material wealth may still have a logistics problem if the material is in the wrong place.

## Recycling is part of the command network

Recycling centers are physical industrial nodes, not delete/refund buttons.

The player should be able to create reclamation policies such as:

```text
inspect
-> repair if economically useful
-> reuse as-is if useful
-> repurpose if appropriate
-> recover intact components
-> recycle remaining material
-> store or process residue
```

At large scale, autonomous haulers, inspection drones, repair units, construction machines, and recycling centers should execute these policies while preserving material accountability.

## Worker specialization without rigid classes

The control hierarchy must work with the existing worker-drone ecology.

Useful specializations include:

- mining/extraction;
- gathering;
- hauling/logistics;
- scanning/survey;
- repair/maintenance;
- fabrication/construction;
- defense/environmental protection;
- processing/refining;
- relay/communications;
- storage/logistics coordination.

These are functional outcomes of hardware, software, tools, storage, propulsion, power, and assigned goals. They are not immutable RPG classes.

Hybrid machines remain valid.

## Communication latency limits authority

The hierarchy must respect Everward's time-and-distance pillar.

A distant descendant is not a drone on an instantaneous network.

Commands travel.
Information travels.
Remote machines act on local knowledge while waiting.

Therefore the control model becomes:

```text
write doctrine
-> transmit doctrine
-> remote system receives it later
-> remote machines interpret and operate locally
-> events occur
-> reports travel back
-> player eventually learns the outcome
```

At sufficiently large scale, the player may possess legal or historical authority over a branch but not practical real-time control.

That divergence is intentional emergent gameplay.

## Simulation level of detail

Player-facing scale may be effectively unbounded, but computational fidelity cannot be uniform across every entity forever.

Everward therefore requires **simulation level of detail**.

Canonical fidelity tiers should evolve approximately as follows:

### Local / observed

Nearby, relevant machines receive full or near-full simulation as required:

- precise position and velocity;
- physics/contact;
- manipulator/tool state;
- cargo;
- power;
- heat;
- damage;
- component state;
- current tasks.

### Distant active operations

Remote fleets or facilities remain discrete entities but may advance through deterministic event/timestep simulation rather than high-frequency physical updates.

### Remote industrial systems

Large distant operations may use aggregated logistics/production updates while preserving enough state to reconstruct meaningful machine identity, inventories, failures, history, and events.

### Very distant autonomous branches

Ancient or remote lineages may use coarse deterministic simulation until they become relevant through communication, player travel, major events, or direct observation.

The simulation may reduce update fidelity. It must not silently erase existence or history merely because something is far away.

## Persistent identity and history

Where practical, machines and important infrastructure should retain persistent identity.

A worker may accumulate:

- creation date;
- parent/fabricator;
- design generation;
- service history;
- repairs;
- upgrades;
- failures;
- major missions;
- recovered components;
- retirement or destruction state;
- recycling history.

This supports the larger Everward principle that **matter has history**.

A Generation-2 miner that worked for 300 years should not necessarily be indistinguishable from a newly fabricated object of the same template.

## UI requirements

The interface must scale gracefully from one machine to civilization scale.

Required long-term capabilities include:

- search by machine, group, role, lineage, region, system, status, or alert;
- hierarchy/tree navigation;
- map filtering;
- group creation and assignment;
- doctrine editing;
- stockpile and throughput dashboards;
- bottleneck visualization;
- repair/maintenance queues;
- recycling/retirement queues;
- alert aggregation;
- direct drill-down from a civilization-scale alert to the affected machine when information is available;
- history and genealogy views.

The player should never need an external spreadsheet merely to know what their civilization is doing.

## Design test

The system succeeds when the same campaign can support all of these experiences without changing games:

```text
one damaged probe
-> one manually controlled mining arm
-> first worker drone
-> a small coordinated mining team
-> automated material logistics
-> multiple factories and recycling centers
-> a system-scale industrial network
-> multiple autonomous probe lineages
-> interstellar machine civilization
```

The player must retain meaningful authority throughout that progression without being forced to issue thousands of individual movement commands.

## Canonical implementation direction

Near-term development remains disciplined.

Do not build civilization-scale UI before the opening loop is proven.

The implementation sequence should be:

1. reliable direct mining interaction;
2. physically accounted mined material;
3. `Fix_It` repair and recommendation loop;
4. local storage/material handling;
5. first worker machine;
6. task assignment to that worker;
7. first small worker group;
8. operational group policies;
9. industrial-network objectives;
10. civilization-scale doctrine and simulation-LOD systems.

The architecture should, however, avoid choices that make the later scale impossible.

The long-term target is explicit:

> **No arbitrary machine ceiling. Control should scale from physical embodiment to intent, organization, and doctrine while simulation fidelity scales according to relevance and distance.**
