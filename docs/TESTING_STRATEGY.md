# Testing Strategy

Everward's most dangerous failures will often appear only after long simulated time, many autonomous entities, save migrations, or procedural edge cases. Testing therefore needs to be designed into the simulation architecture from the first implementation PR.

## Principles

1. **Test simulation without rendering.** Core mechanics must run headlessly.
2. **Determinism is testable behavior.** Same inputs must produce the same mechanical outputs.
3. **Long time horizons are ordinary tests.** Thousands of simulated years should be runnable in CI or dedicated stress jobs where practical.
4. **Seeds are reproducible bug reports.** Every procedural failure should be reducible to seed + coordinates + algorithm version + relevant state.
5. **Save compatibility is part of correctness.** Schema migrations require fixtures and regression tests.
6. **Performance regressions are defects.** Simulation throughput needs benchmark baselines.

## Test layers

### Unit tests

Fast tests for deterministic pure or mostly pure systems:

- time arithmetic,
- event ordering,
- resource accounting,
- component calculations,
- thermal/energy math,
- generation functions,
- research prerequisites,
- message timing,
- inheritance rules,
- doctrine evaluation.

### Simulation integration tests

Exercise multiple systems together without graphics:

- scan → discovery → research opportunity,
- mine → refine → fabricate,
- construct successor → instantiate child,
- departure → travel → arrival,
- send message → delayed receipt,
- child local decision → later report,
- save → load → continue deterministically.

### Determinism tests

Run identical scenarios twice and compare canonical state hashes or normalized state snapshots.

Test:

- same seed/same commands = same result,
- save/reload midpoint = uninterrupted result,
- time acceleration factor changes do not alter final mechanical truth,
- rendering enabled/disabled does not alter final mechanical truth.

### Property/invariant tests

Examples:

- mass cannot become negative,
- stored resource cannot exceed capacity unless overflow behavior is explicit,
- energy accounting remains conserved within defined model boundaries,
- child ancestry cannot form cycles,
- message receive time cannot precede send time,
- an entity ID is never reused within a campaign,
- procedural generation returns stable results for a fixed version.

### Golden-seed tests

Maintain a compact bank of known seeds/coordinates that exercise:

- ordinary systems,
- binaries,
- extreme bodies,
- sparse systems,
- resource-rich systems,
- numerical edge cases,
- previously fixed bugs.

Do not make all generation tests depend on a single seed.

### Save migration tests

For every supported old save version:

1. load fixture,
2. migrate to current schema,
3. validate required invariants,
4. continue simulation,
5. save again,
6. verify canonical current-format load.

### Performance tests

Track at least:

- simulated years/second for empty and populated scenarios,
- events processed/second,
- memory per active entity,
- save size growth,
- save/load latency,
- region-generation latency,
- autonomous-agent throughput.

## Phase 1 acceptance tests

Before the engine/architecture gate closes, prove:

### Simulation clock
- deterministic event order,
- pause/resume,
- time-scale changes,
- very long future events,
- no drift from rendered frame rate.

### Headless simulation
- thousands of simulated years without presentation,
- repeatable state hash,
- bounded memory growth in a fixed scenario.

### Procedural system
- same seed/coordinate/version produces identical output,
- changed coordinate produces a distinct system,
- generated values satisfy declared physical/data invariants.

### Coordinate scale
- local machinery remains stable,
- system-scale navigation remains stable,
- interstellar map coordinates remain stable,
- transitions between coordinate domains do not produce precision explosions.

## CI policy

Once executable code exists, every pull request should run the fastest deterministic unit/integration suite. Slower long-duration and performance suites may run on a scheduled or explicitly triggered basis if routine CI time becomes excessive.

A passing renderer test must never substitute for a passing headless simulation test.
