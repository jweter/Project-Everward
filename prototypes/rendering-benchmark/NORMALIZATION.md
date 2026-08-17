# Rendering Benchmark Normalization Contract

This document defines the boundary between raw benchmark capture evidence and the normalized 0..10 metrics consumed by `benchmark.py`.

## Principle

The engine decision must be reproducible from evidence. Raw measurements are never overwritten by normalized scores, and qualitative judgments are never disguised as measurements.

## Objective dimensions

The following benchmark dimensions are derived directly from validated run-record fields. Lower raw values are better.

| Benchmark metric | Raw capture field |
| --- | --- |
| `cpu_frame_time` | `cpu_frame_time_ms` |
| `gpu_frame_time` | `gpu_frame_time_ms` |
| `memory_use` | `peak_memory_mib` |
| `build_distribution_complexity` | `build_size_mib` |
| `developer_iteration_speed` | `implementation_hours` |

Each pair is normalized symmetrically. The better raw value receives 10.0 and the peer receives `10 * better / worse`. Equal values tie at 10.0. This preserves the measured ratio instead of inventing engine-specific thresholds.

The pairwise score is only meaningful when the two runs use the same canonical scenario and comparable hardware/configuration under the scenario fairness rules.

## Qualitative dimensions

All remaining benchmark dimensions require an explicit 0..10 assessment plus non-empty evidence:

- visual fidelity
- scene complexity
- HUD effort
- procedural workflow
- simulation integration
- large-coordinate behavior
- save/load implications
- commercial licensing

Evidence should point to concrete artifacts or observations such as screenshots, profiler captures, implementation notes, engine documentation, project settings, reproduction steps, or licensing terms. A score without evidence is invalid.

## Audit output

`normalize_pair()` returns both complete `BenchmarkResult` objects and an audit dictionary containing:

- canonical scenario identity,
- engine identities,
- every objective source field and raw value,
- every derived objective score,
- every qualitative score,
- every qualitative evidence note.

The audit record should be preserved with the benchmark evidence used for the eventual engine ADR.

## Scope boundary

This contract does not choose Godot or Unreal, does not fabricate measurements, does not establish final production performance budgets, and does not substitute normalized scoring for engineering judgment. It exists so the Phase 1 engine decision can be reviewed and reproduced from the same evidence.
