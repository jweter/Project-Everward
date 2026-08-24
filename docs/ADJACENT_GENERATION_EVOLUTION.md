# Adjacent-Generation Evolution

Everward does not use a single hand-authored upgrade tree with a final technology tier. Successor design is based on **adjacent engineering possibility**: the next generation is derived from what the current probe already is, what it has learned, and what it can physically manufacture.

## Governing rule

> Every successor may advance only a short engineering distance from its parent.

A new generation can improve an installed system or add a genuinely new capability only when the current design already has nearby prerequisite hardware and the lineage has sufficient scientific, material, fabrication, and computational maturity.

The player should never jump directly from crude Generation-1 hardware to a radically advanced capability. Extraordinary outcomes are reached by accumulating many individually plausible steps.

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

## Candidate classes

### 1. Refinements

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

Refinements remain small. In the initial implementation, one generated step has an engineering distance no greater than 0.15 maturity units.

### 2. Adjacent new capabilities

A new capability is not a free invention. It requires a neighboring capability plus prerequisite engineering maturity.

Examples represented by the initial scaffold:

- optical sensing + sufficient science/fabrication -> near-infrared sensing;
- structure + automation + sufficient fabrication -> basic manipulator;
- manipulator/tool mount + sufficient materials -> basic drill;
- sufficiently advanced computation + science -> onboard design-simulation capability.

Later rules can add UV, radio, spectroscopy, lidar, advanced mining tools, fabrication methods, lasers, exotic propulsion, gravitational sensing, neutrino detection, or speculative far-future systems without changing the generator architecture.

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

## Physical consequences remain visible

Every candidate should eventually explain not only what improves, but what it costs.

The initial generator carries mass, power, and thermal cost factors. Future successor design should also account for:

- volume;
- structural loads;
- material requirements;
- manufacturing tolerance;
- reliability;
- redundancy;
- maintenance burden;
- energy generation/storage;
- heat rejection;
- tool compatibility;
- sensor placement/occlusion;
- propulsion balance;
- construction time and resource cost.

A successor is a new physical machine, not a stat sheet.

## Player-facing successor design

When successor construction becomes playable, the player should see the **reachable frontier**, not the entire theoretical future.

Each option should communicate:

1. what changes;
2. what improves or becomes newly possible;
3. what physical costs/tradeoffs it introduces;
4. what prerequisites made it possible;
5. what future directions it may open.

A Generation-1 machine with weak computation may see only a few obvious engineering directions. A far more capable descendant can evaluate many more alternatives, but each successor still remains close to its parent.

## Determinism and persistence

For identical parent state, knowledge, engineering context, and generator version, candidate generation must be deterministic.

This is important for:

- save/load reproducibility;
- lineage history;
- debugging;
- replayability;
- headless simulation;
- future autonomous child design decisions.

Generator/rule versions must eventually be persisted or migrated with save data once successor generation enters the live save schema.

## Current implementation boundary

`src/simulation/include/everward/simulation/evolution.hpp` provides the first engine-independent scaffold:

- generic evolution domains and traits;
- an `EvolutionContext`;
- deterministic `EvolutionCandidate` generation;
- small-step refinement rules;
- prerequisite-gated adjacent capability rules;
- computation-dependent option breadth;
- a canonical EV-0001 starting context derived from authoritative probe state.

This does **not** yet manufacture or instantiate a successor, spend resources, mutate the live probe, persist a lineage, or expose the successor-design screen in Unreal. Those remain later roadmap work.

The purpose of this Phase-2-era scaffold is to lock the progression architecture early enough that current capability/HUD/software systems grow in a compatible direction instead of assuming a finite upgrade tree.

## Design tests

Any future evolution feature should satisfy all of these:

1. Is the option derived from what the parent actually has or knows?
2. Is the engineering step small enough to feel like one generation?
3. Are new capabilities prerequisite-gated rather than appearing arbitrarily?
4. Are physical tradeoffs represented?
5. Can the result feed the same capability model used by HUD/manual/programmed control?
6. Does better computation improve engineering capability without bypassing physics?
7. Can the system continue generating meaningful nearby options at very high generations?
8. Is the result deterministic from authoritative state and versioned rules?

The long-term promise is not that Everward authors every possible technology in advance. It is that the simulation provides a coherent way for the lineage to keep asking, indefinitely:

> **Given what I am now, what can I become next?**
