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
7. Graphics scalability must reduce presentation cost without changing authoritative gameplay.
8. The lowest supported preset must remain fully playable; low-end hardware receives the whole game, while high-end hardware receives greater visual fidelity.

See `GRAPHICS_SCALABILITY_STANDARD.md` for the canonical full-game parity and preset rules.

## Quality-tier performance targets

Everward should maintain at least these presentation tiers:

- **Emergency / Minimum** — guaranteed-boot fallback for severely constrained supported hardware;
- **Low** — fully playable modest-hardware target and current laptop-development profile;
- **Medium** — balanced mainstream target;
- **High** — high-quality target for capable gaming hardware;
- **Ultra / Cinematic** — maximum intended visual presentation;
- **Custom** — user-tuned settings derived from the same scalable categories.

The presets may change rendering and memory budgets, but may not change simulation authority, gameplay rules, deterministic outcomes, resource accounting, progression, physics, save semantics, or interaction availability.

### Current low-spec development budget

Until measured evidence justifies a change, the development target for the current constrained laptop profile is:

- approximately 1280×720 windowed where practical;
- approximately 30 FPS cap for simple functional playtesting;
- reduced screen percentage/render resolution when required;
- low or very-low shadows, GI, reflections, post-processing, effects, volumetrics, foliage, and decorative view distance;
- bounded texture-streaming memory appropriate for integrated/shared graphics memory;
- no unnecessary high-cost presentation features when testing Phase-2 mechanics;
- constrained local build parallelism to avoid exhausting system RAM;
- skip unnecessary rebuilds when exact-head binaries are known current;
- launch the minimum Unreal/editor/runtime environment required for the scenario.

This budget is successful only when the game is **usable and enjoyable enough for real gameplay testing**, not merely when the editor process technically starts.

Record actual peak RAM, graphics/shared-memory pressure, startup time, level-load time, and steady-state frame time after successful Product Reality runs. Tune from measurements rather than assumptions.

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

The benchmark should eventually be measured at Low, Medium, High, and Ultra/Cinematic rather than only at one fidelity level. Minimum should be exercised as a guaranteed-boot/readability fallback.

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
- peak load during region transitions,
- startup peak RAM by graphics preset,
- steady-state RAM by graphics preset,
- texture-streaming pool occupancy by graphics preset,
- render-target/buffer pressure by graphics preset,
- local build peak RAM and parallel-worker count.

### Persistence

- save size per 100 hours of representative play,
- save time,
- load time,
- migration time,
- history-ledger size,
- number of persisted versus regenerable regions.

### Rendering

- frame time by major scene type,
- frame time by quality preset,
- local entity count,
- LOD transition cost,
- planet/star rendering cost,
- VFX/particle cost,
- UI cost,
- photo-mode maximum-quality cost,
- quality-preset transition cost where runtime switching is supported.

## Preset-equivalence performance rule

Performance tuning may change presentation workload, but it must not create different mechanical universes.

Where practical, benchmark the same deterministic scenario under multiple graphics profiles and verify that authoritative state hashes or normalized outcomes remain identical. A lower preset that gains performance by dropping required simulation work, resource accounting, physics, AI, or interaction processing violates the design.

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
9. continuous outward exploration creating many persistent regions,
10. representative full-game gameplay under Low and Ultra/Cinematic with identical authoritative outcomes,
11. Minimum-preset startup on the lowest supported hardware profile without memory exhaustion.

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

Each review should replace vague targets with measurements from the current build, including evidence from more than one quality preset once the preset system exists.