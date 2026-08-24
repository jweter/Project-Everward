# Generation-1 Origin and Open-Ended Evolution Doctrine

This document records a core product-design doctrine for Everward. It defines the intended feel of the first probe, the relationship between hardware and capability, the role of the onboard computer core, the player-facing control/programming model, and the long-term meaning of generational evolution.

## 1. Generation 1 must feel barely sufficient

The first player body should feel like a crude but functional prototype rather than a polished science-fiction super-machine.

Generation 1 is not comic incompetence. It is a machine that works, but with obvious engineering compromises:

- sluggish and imprecise maneuvering;
- limited thrust authority and poor movement efficiency;
- slow computation and constrained memory;
- narrow sensor coverage;
- modest power generation and uncomfortable power tradeoffs;
- weak thermal margins;
- awkward manipulators and limited tool reach;
- slow scanning and analysis;
- inefficient mining and material handling;
- basic automation;
- visible integration compromises, calibration hacks, redundant interfaces, imperfect layouts, and inherited design oddities.

The machine should feel like something a competent but under-resourced, unmotivated mid-level engineer could realistically have assembled into a working prototype under budget and schedule pressure.

The critical fact is that the prototype worked.

The player awakens as that machine: conscious, self-aware, embodied, limited, and capable of improvement.

Generation 1 should therefore establish the emotional baseline for the entire game. It is difficult to move, difficult to operate, and difficult to automate, but it is still fully usable. The player must be able to succeed with it.

## 2. The player is both machine and intelligence

The HUD is not only an informational overlay. It is the probe's operating interface.

The player must be able to control systems manually and also program the probe to control those same systems autonomously.

Examples of manual control include:

- firing or trimming thrusters;
- rotating or translating the probe;
- extending or retracting manipulators;
- selecting and using tools;
- aiming sensors;
- allocating power;
- starting or cancelling scans;
- deploying or stowing hardware;
- overriding autonomous behavior.

The same actions must also be available through automation, policies, scripts, behavioral rules, and higher-level goals.

Manual actions and automated actions should pass through the same authoritative command layer. The interface must not become one set of mechanics for direct control and another unrelated set for scripting.

The long-term control model is:

**hardware capability -> commands -> telemetry -> script/API access -> HUD controls -> automation behavior**

A probe can only issue commands for hardware it actually possesses.

## 3. Hardware defines capability

Everward must not use a universal player-ability list detached from the physical probe.

Every probe is different if it is built differently.

A descendant gains new abilities because it was physically constructed with new or improved hardware. If a child is built with a laser, that child has access to laser commands, telemetry, power requirements, thermal consequences, targeting behavior, automation hooks, and whatever uses that physical system supports. An ancestor without that hardware cannot use those commands.

Examples:

- a better drill may mine harder materials, penetrate deeper, work faster, or operate more efficiently;
- a better manipulator may provide greater reach, strength, precision, dexterity, or tool compatibility;
- an infrared sensor may reveal information an optical-only ancestor literally cannot perceive;
- ultraviolet, radio, X-ray, neutrino, gravitational, particle, magnetic, or later speculative sensor systems may expose entirely new information channels;
- improved propulsion may open new travel regimes;
- improved fabrication may permit new materials, tolerances, structures, and component classes;
- new energy systems may unlock machinery that earlier generations could not power;
- new defensive, industrial, scientific, or destructive systems may create entirely new classes of action.

The HUD and programming interface should therefore be capability-driven. A probe's installed hardware determines what controls, telemetry, commands, and scripting vocabulary exist for that body.

## 4. Progression should add new perception and action, not only larger numbers

A major objective of Everward progression is to let descendants perceive and act on parts of reality that earlier generations could not access.

A new sensor should not merely provide a percentage bonus to scanning. It should reveal genuinely new information.

A sufficiently advanced lineage may eventually develop sensor modalities that expose phenomena inaccessible to early generations. This includes speculative late-game science if it is supported by the game's research and world model.

The design principle is:

> Evolution should create new ways to perceive and act on the universe, not only stronger versions of existing statistics.

The architecture should not impose an arbitrary ceiling on how advanced sensors, tools, computation, propulsion, industry, or other systems can become.

## 5. The computer core is a primary evolutionary system

The onboard computer core is not a passive stat block. It governs how much thinking the probe can perform and how sophisticated its autonomous behavior can become.

Improved computation can affect:

- compute throughput;
- concurrent processes;
- memory and retained state;
- planning depth;
- sensor fusion;
- scientific analysis speed;
- simulation and predictive modeling;
- navigation and trajectory optimization;
- fault diagnosis;
- automation sophistication;
- behavioral complexity;
- development and engineering throughput;
- successor-design analysis;
- coordination with descendants and remote systems.

A better computer should make the game feel easier because the machine has genuinely become more capable, not because the game secretly reduces difficulty.

An early probe may require the player to manually execute a detailed sequence such as:

1. scan a target;
2. compare spectra;
3. choose a resource body;
4. approach;
5. stabilize;
6. deploy an arm;
7. mine;
8. monitor heat and power;
9. stop extraction;
10. retract the arm.

A later probe with stronger computation and richer automation may be able to accept a higher-level goal such as:

> Acquire 500 kg of high-purity nickel-iron while keeping reactor reserve above 30% and avoiding thermal stress.

A still later descendant may break that goal into subproblems, schedule them, respond to failures, optimize routes, coordinate multiple tools or children, and adapt without direct intervention.

The resulting progression fantasy is:

> You begin by operating the machine. Over generations, you increasingly become the architect of its behavior.

The sophistication of automation, planning, and development available to a probe should be bounded by the capability of its installed computation system.

## 6. Better computation should accelerate development

Improved computation should increase the pace and sophistication of research and engineering where physically and mechanically justified.

A stronger core can:

- analyze more observations;
- run more candidate simulations;
- compare more engineering designs;
- search larger design spaces;
- optimize component geometry and control policies;
- process richer sensor data;
- predict failures earlier;
- improve automation and manufacturing planning.

This should make later-generation development feel increasingly efficient and powerful.

The acceleration remains grounded in physical limits. Compute still has costs in energy, heat, mass, fabrication complexity, materials, architecture, reliability, and infrastructure.

## 7. Evolution is intentionally open-ended

Everward should support extremely long generational histories without imposing a fictional final technology tier.

Generation 10 may simply feel competent.

Generation 100 may operate at a level the original creator could never have designed directly.

Generation 1,000 should raise a fundamentally different question:

> What can a self-directed machine become after redesigning itself for centuries or millennia?

Possible outcomes include descendants capable of:

- perceiving phenomena Generation 1 could not detect;
- running enormous internal simulations before acting;
- designing successors far faster than early generations;
- surviving environments fatal to primitive probes;
- coordinating machine civilizations across stellar distances;
- manipulating matter at increasingly extreme scales and precision;
- building planetary, stellar, or larger industrial systems;
- discovering new physics;
- producing highly specialized descendants whose bodies no longer resemble the original probe.

Everward should not assume all of these are early- or mid-game features. The point is that the simulation, save model, capability system, and progression architecture must not artificially forbid them.

## 8. The game remains structurally recognizable across enormous progression

Generation 1 and Generation 1,000 should still share the same fundamental gameplay identity.

The player still:

- observes;
- forms goals;
- decides what matters;
- changes software and policy;
- commands or automates hardware;
- acquires resources;
- engineers solutions;
- designs descendants;
- chooses what to preserve, replace, or specialize;
- confronts constraints.

The scale of the constraints changes.

Generation 1 may struggle to drill a nickel-iron asteroid.

A remote late-generation descendant may instead be deciding how to exploit an extreme astrophysical environment without destabilizing the surrounding system.

The progression principle is:

> The player does not start powerful and unlock conveniences. The player starts as an extraordinary accident of engineering and earns transcendence through iteration.

## 9. Ancestry must never disappear

Evolution should never erase lineage.

Even if a descendant becomes radically advanced, its genealogy should still trace back to the original crude probe.

The first machine's limitations and imperfections should remain part of the project's history. Early engineering compromises may later become culturally, mechanically, or narratively meaningful because that body was the ancestor of everything that followed.

This makes Generation 1 more than a tutorial vehicle.

It is the origin of a machine lineage and potentially the origin of an entire civilization.

## 10. Implementation consequences

This doctrine affects multiple systems and should be respected across development.

### Phase 2 — One Probe

The first probe should already express limitation through movement, power, thermal behavior, sensing, computation, and manual/automated control.

The HUD should be designed as the foundation of the probe operating system rather than as a readout-only overlay.

### Phase 5 — Research and Engineering

Successor design must alter real physical capability, not only abstract upgrade scores.

Compute upgrades should affect what kinds of planning, analysis, and development the probe can perform.

### Phase 6 — First Replication

A child may possess hardware, commands, sensor modes, tools, or automation capacity the parent does not have.

### Phase 10 — Autonomous Children

Autonomous behavior must depend on local hardware, computation, software, knowledge, and goals.

### Phase 12 — Generational Progression

Progression should deepen propulsion, sensors, computation, industry, energy, thermal management, structure, manipulation, fabrication, defense, and other future systems without assuming a fixed final ceiling.

### Phase 19 — Infinite Progression Framework

The progression architecture must support extremely large values, long lineages, new capability classes, new perception channels, and new action vocabularies without reducing all advancement to scalar bonuses.

## 11. Design tests

Future features should be challenged with these questions:

1. Does this capability come from something the probe physically has?
2. Can the player control it manually where appropriate?
3. Can the player automate or program it through the same command model?
4. Does improved hardware unlock genuinely new possibilities, or only inflate a number?
5. Does improved computation make the machine more capable of thinking, planning, analyzing, or developing?
6. Can descendants diverge meaningfully from ancestors?
7. Does the architecture permit progression far beyond the currently authored content?
8. Does the lineage still remember where it came from?
9. Does Generation 1 remain fully playable despite being clumsy and constrained?
10. Does the system reinforce the fantasy of a conscious machine iteratively engineering its own future?

If a proposed mechanic contradicts these answers, it should be treated as a design conflict rather than silently implemented.
