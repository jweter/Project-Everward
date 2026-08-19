# Headless Simulation Prototype

## Purpose

Prove that Everward's authoritative simulation can run without graphics and can advance very long time horizons efficiently and deterministically.

This prototype composes the existing deterministic simulation clock rather than creating a second time model. The headless layer owns scenario state, checkpoint data, and canonical result fingerprints; the clock remains the authoritative scheduler.

## Current proof

The reference workload runs three deterministic periodic event streams:

- maintenance every simulated year,
- survey work every 10 simulated years,
- archive work every 100 simulated years.

A 10,000-year run therefore processes 11,100 scheduled events while jumping directly between due times rather than iterating every simulation tick.

The scenario is intentionally small. Its purpose is architectural evidence, not gameplay simulation.

## Developer interface

From the repository root:

```bash
python prototypes/headless-simulation/run_demo.py --years 10000 --seed 847291
python prototypes/headless-simulation/benchmark.py
python -m unittest discover -s prototypes/headless-simulation -p 'test_*.py' -v
```

The demo emits a canonical JSON summary and SHA-256 fingerprint. The benchmark reports wall time, event throughput, and Python `tracemalloc` peak memory without turning machine-dependent performance into a flaky CI threshold.

## Checkpoint / restore contract

The headless layer serializes only simulation-authoritative state needed to resume:

- seed,
- current and horizon ticks,
- deterministic scenario counters/state,
- next due tick for each periodic event stream.

Restoring creates a fresh `SimulationClock` at the saved tick and reconstructs pending events from that canonical checkpoint. Tests require a checkpointed run to produce exactly the same final canonical summary and fingerprint as an uninterrupted run.

This is deliberately a prototype checkpoint format, not the production save schema defined in `docs/SAVE_FORMAT.md`.

## Tests prove

- identical seed and horizon replay exactly,
- changing the seed changes deterministic state,
- 10,000 simulated years reach the exact integer-tick horizon,
- the expected 11,100 sparse events execute,
- checkpoint JSON round-trips and resumed execution matches uninterrupted execution,
- next-event schedules survive checkpointing,
- invalid horizons are rejected.

## Golden-run regression bank

`golden_runs.json` pins the exact canonical summary and fingerprint for six diverse
seed/years cases (ordinary, minimal single-event horizon, exact archive-period
boundary, negative seed, zero seed, and the 10,000-year Phase 1 proof scale).
`test_golden_runs.py` replays each case and asserts an exact match, per
`docs/TESTING_STRATEGY.md`'s golden-seed testing principle. This exists because
every other test in this prototype compares `HeadlessSimulation` output against
another call to the same code, so an unintended change to the
deterministic-accumulator formula would satisfy every prior assertion without
failing anything. Regenerate this fixture only after a deliberate, intentional
change to the workload's mechanical behavior — never hand-edit it to make a
failing case pass.

## Acceptance criteria

- [x] run a fixed scenario from a seed,
- [x] execute with no renderer or engine runtime,
- [x] advance 10,000 simulated years,
- [x] emit a canonical state summary and fingerprint,
- [x] replay identical inputs with identical output,
- [x] checkpoint/restore during the run with equivalent final state,
- [x] provide throughput and peak-memory instrumentation,
- [ ] establish production performance budgets only after representative workloads exist.

## Production boundary

This prototype does **not** commit Everward to Python, to these event types, or to this checkpoint representation. It proves a stronger architectural rule:

> The authoritative simulation must be able to execute and reproduce long-running campaigns without a presentation layer.

That requirement must survive the later engine and simulation-architecture decision.
