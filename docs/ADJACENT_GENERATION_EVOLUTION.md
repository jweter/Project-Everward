# Adjacent-Generation Evolution

Everward does not use a single hand-authored upgrade tree with a final technology tier. Successor design is based on **adjacent engineering possibility**: the next generation is derived from what the current probe already is, what it has learned, and what it can physically manufacture.

## Governing rules

> Every successor may advance only a short engineering distance from its parent.

> At an evolution decision, the player may **improve something the machine already has** or **add a capability the machine cannot currently perform** when that new capability is physically reachable.

> There is no arbitrary maximum upgrade level. If the player can solve the engineering requirements and pay the resource, energy, fabrication, and time costs, continued specialization is earned.

A new generation can improve an installed system or add a genuinely new capability only when the current design already has nearby prerequisite hardware and the lineage has sufficient scientific, material, fabrication, and computational maturity.

The player should never jump directly from crude Generation-1 hardware to a radically advanced capability. Extraordinary outcomes are reached by accumulating many individually plausible steps. That adjacency requirement limits the size of one step; it does **not** impose a final ceiling on how many steps a player may take.

## The canonical strategic choice: deepen or expand

Every major successor-design decision should preserve two conceptually different paths whenever both are currently reachable.

### Deepen: improve an installed system

The player invests in a capability that already exists and makes it better. Examples include stronger propulsion, more sensitive sensors, higher fabrication precision, greater manipulator force, better computation efficiency, stronger structure, or faster extraction.

Repeated refinement is intentionally open ended. A player who repeatedly invests in one domain may eventually create a machine that is wildly specialized and dramatically more capable than a balanced lineage.

### Expand: add a genuinely new capability

The player adds something the current machine could not do before. The new capability must be adjacent to existing hardware, knowledge, materials, and fabrication ability rather than appearing as a free unlock.

Examples represented by the initial scaffold include near-infrared sensing, a basic manipulator, a drilling tool, and onboard design simulation. Later capability rules may expand far beyond these examples.

The important distinction is experiential:

- an **improvement** changes how well the probe performs an existing action;
- a **new capability** changes what actions are possible at all.

The player-facing design should make that difference obvious.

## Why this is procedural rather than a giant tree

A fixed technology tree would eventually become both authoring-limited and incompatible with Everward's open-ended progression. Instead, the game defines:

- engineering domains;
- installed capabilities and traits;
- prerequisite relationships;
- small bounded improvement steps;
- physical tradeoffs;
- science/material/fabrication requirements;
- computation-limited design breadth;
- deterministic candidate generation.

The generator then asks:

> Given what this probe is right now, what can it plausibly become next?

The answer changes as the lineage changes.

## Inputs

A successor-option generation pass considers the current probe's:

- installed hardware and software capabilities;
- maturity of each installed trait;
- scientific knowledge;
- known/usable material capability;
- fabrication precision and manufacturing maturity;
- onboard computation maturity;
- generation/lineage state;
- later: operating experience, environment, discovered phenomena, available infrastructure, resources, and failure history.

The current C++ scaffold models the first subset of these explicitly and is intentionally extensible.

## Candidate class 1 — refinements

A refinement improves something the parent already possesses.

Examples:

- propulsion efficiency;
- propulsion authority;
- optical-sensor sensitivity;
- sensor resolution;
- computation throughput;
- computation efficiency;
- radiator heat rejection;
- thermal-control precision;
- energy density;
- storage handling;
- structural strength;
- structural mass efficiency;
- manipulator precision/force;
- drill hardness/extraction rate;
- fabrication precision/throughput.

One generated refinement remains a small adjacent step. In the initial implementation, one step has an engineering distance no greater than 0.15 maturity units.

There is deliberately **no maturity clamp** and no `MAX_LEVEL` concept in the refinement model. A maturity of 10, 1,000, or 1,000,000 may still generate another valid adjacent refinement. Extreme specialization becomes harder because cost and construction-time scales grow with maturity, not because the game declares the system finished.

This is a core reward philosophy: if a player spends the time and resources required to create an absurdly powerful engine, scanner, manipulator, or other system, the power is earned.

## Candidate class 2 — adjacent new capabilities

A new capability is not a free invention. It requires a neighboring capability plus prerequisite engineering maturity.

Examples represented by the initial scaffold:

- optical sensing + sufficient science/fabrication -> near-infrared sensing;
- structure + automation + sufficient fabrication -> basic manipulator;
- manipulator/tool mount + sufficient materials -> basic drill;
- sufficiently advanced computation + science -> onboard design-simulation capability.

Later rules can add UV, radio, spectroscopy, lidar, advanced mining tools, fabrication methods, lasers, exotic propulsion, gravitational sensing, neutrino detection, or speculative far-future systems without changing the generator architecture.

These examples are not a published list of every future capability. The game should retain substantial undisclosed possibility space so discovery remains meaningful.

## Specialization without rigid classes

Everward may show a small number of recognizable evolutionary **highlights** to communicate what specialization can produce, but those highlights are examples rather than classes or endpoints.

Useful public-facing examples include:

- **Scientific Explorer** — emphasizes sensing, analysis, survey, and scientific reach;
- **Deep-Space Industrial Machine** — emphasizes manipulation, mining, processing, fabrication, storage, and heavy physical work;
- **Advanced Machine Intelligence** — emphasizes computation, automation, design search, coordination, and tightly integrated systems.

They demonstrate possibility without revealing every later form, capability chain, or extreme endpoint. A player should be able to become a hybrid, diverge from all highlighted examples, or specialize far beyond them.

The game should avoid a screen that effectively says, "Here are all 200 technologies you will eventually unlock." The unknown future is part of the reward.

## Spoiler-safe reachable frontier

The successor interface should show the **reachable frontier**, not the entire theoretical technology space.

If both strategic paths are available, the interface should preserve at least one visible option under each conceptual category:

```text
IMPROVE EXISTING SYSTEM
- current reachable refinements...

ADD NEW CAPABILITY
- currently understood adjacent possibilities...
```

The second category should contain only capabilities the current lineage has actually become capable of designing or plausibly hypothesizing. Far-future possibilities remain undisclosed until prerequisite knowledge or engineering context makes them relevant.

This creates curiosity rather than checklist completion:

> What becomes possible if I keep pushing this direction?

Each visible option should communicate:

1. what changes;
2. what improves or becomes newly possible;
3. what physical costs/tradeoffs it introduces;
4. what prerequisites made it possible;
5. only the immediate future directions that the machine could reasonably infer.

Do not spoil distant capability chains merely to prove that content exists.

## Computation affects invention

The onboard computer core is itself part of evolution.

Better computation does not remove physical requirements. It increases how much of the nearby design space the probe can evaluate and eventually how deeply it can simulate candidate successors.

The current scaffold implements the first expression of this rule: greater computation maturity increases the number of candidate directions the generator can surface, while every individual candidate remains bounded to a small engineering step.

Long term, computation may also improve:

- candidate quality;
- prediction confidence;
- multi-system optimization;
- failure forecasting;
- automated trade studies;
- research throughput;
- design-search depth;
- ability to discover non-obvious combinations.

This creates a legitimate feedback loop:

```text
better computation
-> better engineering evaluation
-> better successor choices
-> stronger future computation
-> broader/deeper engineering search
```

It must never become `more compute -> ignore physics`.

## Unlimited power, physical consequences

Everward should permit extreme power without making engineering meaningless.

There is no arbitrary rule saying an engine, scanner, mining system, defense system, or other capability must stop improving because the designer expected players to quit at level 20. Instead, increasingly extreme performance should demand increasingly extreme support:

```text
more capability
-> more/rarer material
-> more fabrication difficulty
-> more construction time
-> greater power demand or efficiency challenge
-> greater heat-rejection burden
-> stronger structure / mounting / control requirements
-> additional supporting-system evolution
```

Diminishing returns may exist where physically or economically appropriate, but they are balancing behavior rather than a hidden ceiling. Likewise, breakthrough materials or architectures may change the cost curve without erasing the lineage's previous investment.

The game should be tested at absurd scales specifically to ensure numerical and simulation systems do not become the real level cap.

## Physical consequences remain visible

Every candidate should explain not only what improves, but what it costs.

The generator carries mass, power, and thermal cost factors. Refinements now also carry open-ended `resource_cost_scale` and `construction_time_scale` values that increase as the source system becomes more mature. Future successor design should additionally account for:

- volume;
- structural loads;
- specific material requirements;
- manufacturing tolerance;
- reliability;
- redundancy;
- maintenance burden;
- energy generation/storage;
- heat rejection;
- tool compatibility;
- sensor placement/occlusion;
- propulsion balance;
- fabrication infrastructure;
- construction time and resource cost.

A successor is a new physical machine, not a stat sheet.

## Determinism and persistence

For identical parent state, knowledge, engineering context, and generator version, candidate generation must be deterministic.

This is important for:

- save/load reproducibility;
- lineage history;
- debugging;
- replayability;
- headless simulation;
- future autonomous child design decisions.

Repeated refinement carries an explicit `refinement_rank` alongside source/resulting maturity so later persistence can identify very deep repeated specialization without requiring a fixed finite list of level IDs.

Generator/rule versions must eventually be persisted or migrated with save data once successor generation enters the live save schema.

## Current implementation boundary

`src/simulation/include/everward/simulation/evolution.hpp` now provides an engine-independent scaffold for:

- generic evolution domains and traits;
- an `EvolutionContext`;
- deterministic `EvolutionCandidate` generation;
- explicit `EvolutionFrontier` categories for **improve existing** and **add new capability**;
- a frontier-selection rule that preserves both categories when both are reachable;
- small-step refinement rules with no maximum maturity;
- escalating resource/time cost scales for extreme specialization;
- repeated-refinement rank tracking;
- prerequisite-gated adjacent capability rules;
- computation-dependent option breadth;
- a canonical EV-0001 starting context derived from authoritative probe state.

This does **not** yet manufacture or instantiate a successor, spend resources, mutate the live probe, persist a lineage, or expose the successor-design screen in Unreal. Those remain later roadmap work.

The purpose of this Phase-2-era scaffold is to lock the progression architecture early enough that current capability/HUD/software systems grow in a compatible direction instead of assuming a finite upgrade tree.

## Design tests

Any future evolution feature should satisfy all of these:

1. Is the option derived from what the parent actually has or knows?
2. Is the engineering step small enough to feel like one generation?
3. When both are reachable, can the player genuinely choose between improving an installed system and adding a new capability?
4. Are new capabilities prerequisite-gated rather than appearing arbitrarily?
5. Are physical tradeoffs represented?
6. Can the result feed the same capability model used by HUD/manual/programmed control?
7. Does better computation improve engineering capability without bypassing physics?
8. Can an installed system continue generating meaningful upgrades at absurd maturity values without a hard cap?
9. Do extreme upgrades become expensive because of physical/economic demands rather than because of a maximum-level rule?
10. Does the player see only the reachable frontier instead of a spoiler-heavy complete future tree?
11. Is the result deterministic from authoritative state and versioned rules?

The long-term promise is not that Everward authors every possible technology in advance. It is that the simulation provides a coherent way for the lineage to keep asking, indefinitely:

> **Given what I am now, what can I become next?**
