# Simulation Clock Proof Matrix

This file records the specific Phase 1 timing invariants exercised by the prototype.

| Requirement | Evidence |
|---|---|
| Authoritative simulation time | `SimulationClock.tick` is the only clock state used by the scheduler. |
| Stable simultaneous ordering | Queue ordering is `(due_tick, sequence)` and `test_simultaneous_events_preserve_insertion_order` verifies it. |
| Pause/resume semantics | `set_paused()` gates wall-time advancement; covered by unit test. |
| Multiple time scales | `set_time_scale(numerator, denominator)` uses exact rational arithmetic. |
| Frame/update-rate independence | Fine-grained and coarse-grained wall updates are compared in unit tests. |
| Long future events | A 10,000-Julian-year event is scheduled and processed directly. |
| Sparse advancement | `advance_to()` jumps between due events instead of iterating every tick. |
| Deterministic follow-up work | Event handlers may schedule later events and retain canonical order. |
| Replay evidence | A fixed scenario is executed twice and its complete final state/history is compared. |
| Headless execution | `run_demo.py` runs without any rendering or engine dependency. |
| Canonical trace fingerprint | Demo output includes a SHA-256 digest of sorted canonical event history. |
| Exact scaling arithmetic (not merely self-consistent) | `test_wall_tick_scaling_matches_independently_computed_floor_division` and `test_wall_tick_scaling_matches_closed_form_across_irregular_chunk_sizes` assert the fractional remainder-carry accumulator against `n * numerator // denominator`, a closed-form value computed independently of `SimulationClock`, rather than only comparing two clock runs against each other. |

## Gate interpretation

Passing this proof does **not** mean Everward's complete headless simulation architecture is finished. It establishes the timing and scheduling semantics needed by the next proof.

The production implementation must preserve these semantics even if the runtime representation, language, tick resolution, queue implementation, or engine changes.
