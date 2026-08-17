# Performance Budgets

Everward's long-term technical risk is the interaction of simulation scale, persistence, time acceleration, autonomous agents, and high-fidelity rendering. Performance targets must therefore exist before optimization becomes an emergency.

These are **initial engineering budgets**, not promises to players. Phase 1 prototypes should measure them and revise them with evidence.

## Performance philosophy

1. Rendering and simulation are budgeted separately.
2. Headless simulation throughput is a first-class metric.
3. Distant/inactive state should be aggregated instead of updated every frame.
4. Time acceleration must not multiply work linearly with simulated time.
5. Persistent universe size should grow primarily with meaningful interaction, not merely map visibility.
6. Determinism cannot be sacrificed casually for speed.

## Phase 1 proof budgets

### Simulation clock

Target characteristics:

- event scheduling and ordering remain deterministic under all supported time scales,
- no dependence on rendered frame rate,
- one simulated century in a trivial scenario completes effectively instantly relative to interactive use,
- 10,000 simulated years in a sparse headless scenario are practical for developer testing,
- memory does not grow continuously when no new persistent entities/events are being created.

Exact milliseconds/throughput targets should be established after the first implementation language/runtime is selected.

### Procedural generation

Initial targets:

- ordinary star-system generation should feel instantaneous in interactive use,
- deterministic regeneration should not require persisted copies of untouched systems,
- generation should be parallelizable or streamable where the chosen engine/runtime safely permits it,
- generation of neighboring regions must not cause visible gameplay stalls after production streaming is implemented.

### Coordinate scale

The prototype must demonstrate stable behavior at:

- meter-scale machinery,
- kilometer-scale local environments,
- AU-scale systems,
- light-year-scale interstellar positions.

No visible jitter, catastrophic collision precision failure, or unstable trajectory calculations are acceptable in the representative proof.

### Rendering benchmark

For the representative asteroid-mining scene, record rather than guess:

- CPU frame time,
- GPU frame time,
- memory footprint,
- draw/scene complexity,
- lighting/VFX cost,
- UI cost,
- time-acceleration impact,
- development effort required to reach comparable fidelity.

Do not choose the engine from a single FPS number; compare total implementation cost and simulation integration.

## Scaling metrics to track throughout development

### Simulation

- events processed/second,
- simulated years/real second by scenario class,
- active probes simulated/second,
- autonomous decisions/second,
- time spent in physics/movement,
- time spent in economy/industry,
- time spent in procedural generation,
- time spent in persistence.

### Memory

- bytes per active probe,
- bytes per inactive/aggregated probe,
- bytes per modified system,
- event-ledger growth rate,
- cache sizes,
- peak load during region transitions.

### Persistence

- save size per 100 hours of representative play,
- save time,
- load time,
- migration time,
- history-ledger size,
- number of persisted versus regenerable regions.

### Rendering

- frame time by major scene type,
- local entity count,
- LOD transition cost,
- planet/star rendering cost,
- VFX/particle cost,
- UI cost,
- photo-mode maximum-quality cost.

## Long-term stress scenarios

The project should eventually maintain automated or developer-run scenarios for:

1. one probe over 10,000 simulated years,
2. 100 probes with active industry,
3. 1,000 probes across many systems,
4. 10,000+ distant autonomous probes using aggregated simulation,
5. repeated save/load cycles across a long campaign,
6. extreme lineage depth,
7. very large progression values,
8. heavily modified local system with dense structures,
9. continuous outward exploration creating many persistent regions.

## Numerical stability

Infinite progression is a design goal; numerical overflow is not.

Before Phase 19, explicitly test representative capability values at very large scales. Determine which systems require logarithmic representations, arbitrary precision, normalized units, capped intermediate calculations, or other stable mathematical forms.

Never let a progression cap appear merely because a primitive numeric type ran out of range.

## Budget review cadence

Revisit this document at:

- completion of Phase 1 technical proofs,
- engine selection,
- vertical slice,
- Alpha,
- optimization phase,
- Beta.

Each review should replace vague targets with measurements from the current build.
