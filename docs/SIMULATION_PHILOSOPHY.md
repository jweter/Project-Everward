# Simulation Philosophy

## Simulation owns truth

Everward follows one architectural rule above all others:

> **The simulation determines what is true. Presentation renders or narrates that truth.**

UI, VFX, audio, camera logic, text narration, and any future AI-assisted presentation layer may explain events, but they do not decide whether those events mechanically occurred.

Examples:

- The simulation decides whether a probe survives a stellar flare.
- The renderer visualizes the flare and damage.
- The event log records the outcome.
- A future narrative layer may summarize the event in natural language.

This separation is essential for deterministic testing, balancing, replayability, save migration, and long-running campaigns.

## Determinism is foundational

Given the same:

- universe seed,
- generation algorithm version,
- initial state,
- player commands,
- autonomous-agent decisions,
- and deterministic random streams,

Everward should reproduce the same mechanical results.

A conceptual generation function is:

```text
UniverseSeed
+ SpatialCoordinate
+ GenerationAlgorithmVersion
→ GeneratedRegion
```

The game should generate only what is needed and persist meaningful changes rather than preallocating an enormous universe.

## Event-driven time

Everward spans seconds, hours, years, centuries, and potentially much longer. The core simulation must therefore support scheduled events and aggressive time acceleration without requiring every entity to update every rendered frame.

Conceptually:

```text
SimulationTime
- current_time
- time_scale
- paused

EventQueue
- event_time
- event_type
- entity_id
- payload
```

Examples:

```text
scan_complete      @ +3.4 hours
mining_cycle       @ +9.7 hours
probe_arrival      @ +13.8 years
message_arrival    @ +22.4 years
```

Long-duration simulation should advance to relevant events and aggregate inactive behavior where exact per-tick simulation adds no value.

## Headless simulation is a first-class capability

The simulation must be able to run without graphics. This enables:

- thousands of simulated years in automated tests,
- determinism checks,
- large-agent stress tests,
- balance experiments,
- save/load verification,
- absurd progression tests,
- and procedural-generation regression tests.

The project should eventually support commands conceptually similar to:

```text
everward-sim --years 10000 --seed 847291
```

The exact executable and runtime are intentionally undecided until the engine/architecture gate is resolved.

## Time and information are physical

Travel takes time. Messages take time. Remote descendants act using the information available where they are.

A command sent 18 light-years away does not retroactively control the previous 18 years of local decisions. This is not a UI inconvenience to be patched away; it is a defining simulation property and source of emergent narrative.

## Local fidelity, distant abstraction

Simulation fidelity should scale with relevance.

- Near the player, physics and machinery may require fine-grained updates.
- At system scale, orbital and industrial systems can use event-based or analytic models.
- Distant inactive regions should use aggregated simulation.
- Unobserved regions should remain deterministic potential rather than fully instantiated state.

The system must preserve causal correctness while avoiding the impossible cost of continuously simulating an effectively unlimited universe at maximum detail.

## Persistent history

Important events become part of an appendable historical ledger. Every persistent entity should have a stable identity so a campaign can answer questions such as:

- Who created this probe?
- What generation is it?
- When did it leave its origin system?
- Which observations unlocked this technology?
- When was this message sent and received?
- Which body currently hosts player consciousness?

History is not decorative. It supports debugging, lineage, narrative, save migrations, analytics, and player memory.
