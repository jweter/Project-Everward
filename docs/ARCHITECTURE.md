# Architecture

Everward is architected as a long-lived simulation product. The core goal is to keep authoritative game state testable and independent from rendering technology so the project can support time acceleration, large autonomous populations, deterministic replay, save migration, and engine benchmarking.

## Logical layers

```text
┌─────────────────────────────────────┐
│ Presentation                         │
│ rendering · camera · HUD · VFX · SFX│
└──────────────────┬──────────────────┘
                   │ state / commands
┌──────────────────▼──────────────────┐
│ Game Application                     │
│ orchestration · input · use cases    │
└──────────────────┬──────────────────┘
                   │
┌──────────────────▼──────────────────┐
│ Simulation Core                      │
│ time                                 │
│ coordinates / movement               │
│ astronomy                            │
│ probes                               │
│ resources / industry                 │
│ research / engineering               │
│ messages                             │
│ descendants / lineages               │
│ events                               │
│ difficulty                           │
└──────────────────┬──────────────────┘
                   │
┌──────────────────▼──────────────────┐
│ Persistence                          │
│ saves · migrations · history ledger  │
└─────────────────────────────────────┘
```

## Hard boundaries

### Presentation cannot author mechanical truth

Presentation may submit commands and consume read models/events. It may not independently mutate authoritative mechanical state.

### Simulation cannot depend on rendered frame rate

Simulation time must advance according to its own clock and scheduler. Rendering frames are views of simulation state, not the clock source.

### Save state is explicit data

Never depend on blind serialization of arbitrary engine objects as the canonical save format. Persistent data must have a defined schema and migration path.

### Procedural generation is versioned

Generated astronomy must be reproducible from explicit deterministic inputs. Algorithm changes require version identifiers and migration/compatibility strategy.

## Candidate core modules

```text
Simulation
├── Time
├── Events
├── Coordinates
├── Astronomy
├── Motion
├── Environment
├── Resources
├── Industry
├── Probe
├── Components
├── SoftwarePolicy
├── Research
├── Engineering
├── Messages
├── Lineage
├── AutonomousAgency
├── Difficulty
└── History
```

The exact package/module layout remains open until the Phase 1 technical proofs establish the production stack.

## Entity identity

Persistent entities receive stable IDs. Likely entity classes include:

- regions,
- star systems,
- stars,
- planets,
- moons,
- minor bodies,
- probes,
- components/designs,
- facilities,
- resource stores,
- lineages,
- messages,
- discoveries,
- research results,
- civilizations,
- historical events.

IDs must survive save/load and must not be derived from transient engine object addresses.

## Commands, events, and state

Prefer explicit interaction concepts:

- **Command:** request to change simulation state, e.g. schedule a scan or begin fabrication.
- **Validation:** verifies the request is legal under current local state.
- **State transition:** deterministic mechanical update.
- **Domain event:** records that something meaningful happened.
- **Presentation event/read model:** communicates resulting state to UI/audio/VFX.

This does not require a strict event-sourcing implementation. It does require auditable state transitions and a clear event history for important actions.

## Time model

The core clock must support:

- pause,
- multiple acceleration factors,
- exact event ordering,
- deterministic scheduled events,
- long-duration jumps,
- local high-fidelity windows,
- distant aggregated updates.

Avoid naive update-every-entity-every-tick architecture for distant or inactive state.

## Spatial model

Everward spans meters to light-years. Phase 1 must prove a hierarchical/floating-origin or equivalent coordinate strategy that prevents precision failure while preserving deterministic astronomical positioning.

Do not lock a solution before the coordinate-scale prototype.

## Procedural universe

Conceptually:

```text
region = Generate(universe_seed, spatial_coordinate, algorithm_version)
```

Unvisited regions need not exist as persistent objects. Persist observations, modifications, active entities, and any state that can no longer be regenerated from the original deterministic function.

## Autonomy

Remote descendants should operate from:

- local state,
- local knowledge,
- inherited directives,
- reusable doctrines,
- behavior parameters,
- deterministic decision rules or bounded stochastic systems using controlled random streams.

They should not read omniscient global state unavailable to them through communication.

## Tooling

Developer tooling may include Python or other offline tools for astronomy analysis, content generation, test fixtures, profiling, visualization, and migration support. Shipping runtime dependencies should remain minimal and justified.

## Architectural gates

Before production implementation begins, Phase 1 must answer:

1. How is simulation code isolated from presentation in each engine candidate?
2. How is headless simulation executed?
3. What coordinate model survives local-to-interstellar scale?
4. What deterministic event scheduler meets long-duration needs?
5. How are save schemas represented and migrated?
6. How does each engine handle the representative rendering benchmark?
7. What is the cost of integrating thousands of autonomous entities?
