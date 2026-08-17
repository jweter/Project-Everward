# Simulation Clock Prototype

## Purpose

Prove deterministic simulation time, pause/resume, scheduled events, stable ordering, rational time acceleration, and long sparse timelines independently of rendering or the final game engine.

This is intentionally a **reference prototype**, not a commitment to Python as Everward's shipping simulation runtime.

## Implemented proof

`clock.py` provides:

- authoritative integer simulation time,
- one-microsecond prototype ticks,
- a priority-queue event scheduler,
- deterministic insertion-order tie breaking for simultaneous events,
- absolute and relative scheduling,
- event handlers that can deterministically schedule follow-up work,
- pause/resume,
- exact rational time scales using `fractions.Fraction`,
- fractional remainder carry so frame/update chunking does not alter final simulation time,
- sparse event-driven advancement rather than per-frame/per-tick iteration,
- canonical processed-event history,
- support for events thousands of simulated years into the future.

`run_demo.py` executes a small headless Everward-style scenario containing scanning, mining, interstellar arrival, follow-up events, and delayed communication. It emits a SHA-256 digest of the canonical event history so repeated runs can be compared directly.

`test_clock.py` verifies the key invariants automatically.

## Run locally

From the repository root:

```bash
python -m unittest discover -s prototypes/simulation-clock -p 'test_*.py' -v
python prototypes/simulation-clock/run_demo.py
```

No third-party dependencies are required.

## Determinism contract proven here

Given the same:

- initial simulation tick,
- scheduled events,
- event insertion order,
- registered handlers,
- commands,
- and rational time-scale changes,

this prototype must produce the same authoritative simulation time and the same ordered event history regardless of how frequently a renderer or outer application asks time to advance.

That is the architectural property Everward needs for headless simulation, replay/debugging, time acceleration, save migration testing, autonomous descendants, and delayed communications.

## Sparse-time behavior

The clock does **not** simulate every microsecond between two distant events.

If the next scheduled work is fourteen years away, the scheduler can jump directly to that event timestamp. This is critical for Everward because a campaign may contain extremely long quiet travel periods while still needing precise ordering when something actually occurs.

## Prototype decisions that are not yet production commitments

### Tick resolution

One microsecond is convenient for the proof. The shipping simulation's time representation must be chosen after we know the production language, persistence format, physics requirements, maximum campaign horizon, and numerical constraints.

### Python

Python was selected here because it is already accepted for Everward's offline/prototype tooling and lets us prove semantics without coupling them to Godot or Unreal. The production implementation may be C++, GDScript, C#, or another runtime selected by the engine/architecture evidence gate.

### Event payloads

Prototype payloads are Python mappings. Shipping events will need explicit versioned schemas compatible with save/load and migration requirements.

## Acceptance criteria

The prototype passes when:

- a fixed scenario replays identically,
- simultaneous events retain deterministic ordering,
- frame/update chunking does not alter results,
- pause and time scaling behave deterministically,
- handlers can create deterministic follow-up events,
- a 10,000-year sparse timeline completes without iterating each simulated tick,
- and CI/local tests validate these properties.

## Next dependency

Once this PR and the repository-hardening PR are merged, this timing model becomes an input to the broader `headless-simulation` proof. The next major technical proof should combine deterministic time with a minimal deterministic state model and seeded procedural generation rather than adding gameplay systems prematurely.
