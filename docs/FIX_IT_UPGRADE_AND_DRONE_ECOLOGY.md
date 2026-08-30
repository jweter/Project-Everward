# Fix_It Upgrade Guidance and Worker-Drone Ecology

## Canonical status

This document records a core Everward progression system and extends the canonical `Fix_It` origin defined in `ROADMAP.md`.

`Fix_It` begins as a crude survival-oriented engineering program with one dangerously broad instruction: **make the probe work**. Over time, that behavior grows from damage recovery into diagnosis, prioritization, replacement planning, redesign, automation, and machine-lineage evolution.

The intended progression is:

```text
observe
-> diagnose
-> rank bottlenecks / risks
-> recommend next action
-> acquire resources
-> repair / replace / build
-> evaluate results
-> redesign / evolve
```

The system must remain physically grounded. `Fix_It` cannot create capability without materials, energy, fabrication access, time, tooling, and a mechanically valid design path.

## Upgrade recommendation engine

`Fix_It` should continuously evaluate the current probe, nearby worker machines, infrastructure, environment, resource state, mission goals, and operational telemetry.

It should generate **ranked upgrade recommendations**, not merely report damage.

Recommendation inputs should eventually include:

- damaged or offline systems;
- throughput bottlenecks;
- energy deficits;
- thermal constraints;
- storage limits;
- excessive travel or idle time;
- tool limitations;
- scan coverage gaps;
- mining inefficiency;
- material shortages;
- maintenance burden;
- environmental hazards;
- mission priorities;
- available fabrication capability;
- known technology and materials;
- observed failure modes from previous designs.

Each recommendation should explain:

1. **What is limiting performance or survivability?**
2. **Why this upgrade is currently high priority.**
3. **Expected benefit.**
4. **Material cost.**
5. **Energy cost.**
6. **Fabrication / installation time.**
7. **Required tools or precursor capability.**
8. **Tradeoffs introduced by the change.**
9. **Alternatives**, when multiple engineering solutions exist.

The player remains the final authority. `Fix_It` recommends and explains; the player chooses whether to follow, defer, reject, or pursue another engineering strategy.

This should replace the feel of a conventional arbitrary skill tree. Progress should emerge from engineering need, observed operation, available matter, and player intent.

## Repair -> Replacement -> Upgrade -> Redesign -> Evolution

The canonical progression extends beyond simple health restoration:

### Repair
Restore damaged existing hardware to useful operation.

### Replacement
Fabricate a new component when repair is inefficient, impossible, or no longer strategically sensible.

### Upgrade
Install a materially or functionally superior component while preserving the surrounding architecture.

### Redesign
Change the architecture because incremental upgrades have reached diminishing returns or created new constraints.

### Evolution
Build successor bodies, specialized worker machines, infrastructure, or probe descendants whose designs differ substantially from the original Generation-1 machine.

The key design principle is that evolution has a **cause**. Machines change because the simulation reveals pressure to change them.

## Worker-drone ecology

Worker drones are a major industrial and progression layer. They are not cosmetic companions and should not be treated as abstract production bonuses.

Every worker machine should physically exist, consume materials and energy, occupy space, use storage and tools, travel through the world, suffer damage, require maintenance, and become a candidate for repair, upgrade, salvage, or recycling.

Initial specialist roles should include:

### Mining / extraction drones
- travel to known deposits;
- use drills, cutters, excavators, manipulators, or other appropriate extraction tools;
- consume power and tool life;
- create real extracted material that must be handled physically;
- expose extraction-rate, tool, thermal, mobility, and durability bottlenecks to `Fix_It`.

### Gathering / hauling drones
- collect loose resources or extracted material;
- move cargo between mining sites, parent probes, depots, processors, and fabrication nodes;
- make storage volume, payload mass, range, route efficiency, and recharge time meaningful design variables.

### Scanner / survey drones
- extend sensor range and coverage;
- prospect for resources;
- map terrain and orbital/local-space regions;
- investigate hazardous or distant areas before committing the parent probe;
- provide data that creates new engineering and industrial options rather than functioning as a detached exploration meter.

### Repair / maintenance drones
- inspect and service the parent probe, other drones, infrastructure, and later large installations;
- allow distributed maintenance and eventually coordinated repair operations;
- remain constrained by access, tools, materials, energy, and component knowledge.

### Fabrication / construction drones
- assemble components, infrastructure, depots, processors, large structures, and eventually successor probes;
- become important when the original probe's own manipulator reach, payload, precision, or simultaneous-work capacity becomes a bottleneck.

### Defense / environmental-protection drones
- protect machines and infrastructure in hostile environments;
- address threats such as debris, radiation events, extreme weather, dangerous terrain, biological hazards, hostile machines, or later direct combat;
- evolve from the same sensor, propulsion, power, manufacturing, damage, and repair systems rather than forming a disconnected combat subsystem.

## No rigid classes

These roles describe useful specializations, not immutable RPG classes.

A machine's role should emerge from its physical design, installed components, software policy, tools, storage, mobility, power system, and assigned goals.

Hybrids should be possible. A scanner may carry a small sample collector. A hauler may gain defensive countermeasures. A mining drone may become a heavy industrial platform. A repair drone may evolve into a general fabrication unit.

Eventually `Fix_It` may determine that maintaining many narrowly specialized platforms is less efficient than designing a new general-purpose or hybrid architecture.

## Drones are themselves subject to Fix_It

This is critical.

`Fix_It` does not only inspect the player probe. Worker machines generate telemetry and failure history that can drive recommendations.

Example:

```text
Mining Unit 03 spends 31% of operating time returning for recharge.

Observed bottleneck: energy endurance.

Possible responses:
- increase onboard battery capacity;
- reduce propulsion energy cost;
- add local solar collection;
- construct a nearby charging depot;
- build a dedicated hauler so the miner no longer transports material;
- replace the unit with a redesigned mining platform.
```

The system should learn from repeated operational evidence. Poor designs should create reasons for better designs.

## Recommendation scope expands with civilization scale

Early game recommendations are local and survival-oriented:

- restore minimum power;
- repair sensors;
- repair manipulator reach;
- fabricate a replacement joint;
- increase battery reserve.

Mid-game recommendations become industrial:

- build a mining drone;
- add a cargo hauler;
- create a local depot;
- improve refining throughput;
- add autonomous maintenance;
- specialize one descendant for surveying.

Late-game recommendations become architectural and civilizational:

- retire an obsolete chassis family;
- redesign a worker fleet around new propulsion;
- construct regional repair infrastructure;
- replace centralized fabrication with distributed nodes;
- build a successor body because the current probe architecture is no longer efficient for its required mission.

## Physical logistics remain mandatory

There is no magical shared inventory.

Worker drones must interact with physical logistics:

```text
scan
-> identify material
-> mine / gather
-> load cargo
-> transport
-> unload to storage / processing
-> refine / fabricate
-> repair / replace / construct
-> recycle scrap and obsolete hardware back into useful feedstock where plausible
```

This makes storage, transport, range, throughput, depot placement, maintenance, and specialization meaningful engineering problems.

## Recycling and salvage

Worker machines, failed components, obsolete upgrades, destroyed hardware, and eventually defunct probes should remain part of the material economy.

They may be:

- repaired;
- cannibalized for parts;
- repurposed;
- salvaged;
- recycled into feedstock;
- preserved as historical artifacts or infrastructure.

Everward should trend toward closed material cycles. Matter should have history.

## Player experience target

The player should repeatedly experience this pattern:

> I built something because I needed it. It worked. Its weaknesses became visible. `Fix_It` showed me what was limiting it. I chose how to solve the problem. The new solution changed what I could do next.

That is the core emotional and mechanical bridge from one broken probe to a self-improving machine civilization.
