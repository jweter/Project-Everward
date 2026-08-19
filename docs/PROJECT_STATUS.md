# Everward Project Status

This file is the operational continuation record for human and scheduled development. It records where active work should resume without replacing the authoritative roadmap, design pillars, accepted ADRs, or architecture documents.

## Current phase

**Phase 1 — Technical Proofs**

Phase 0 project-constitution material is established. Phase 2 — One Probe is not authorized until the Phase 1 exit gate passes with evidence.

## Current milestone / gate

Complete the Phase 1 technical-proof evidence set and satisfy `PHASE1_EXIT_GATE.md`:

- deterministic simulation clock;
- deterministic procedural star-system proof;
- representative space-rendering benchmark;
- massive-coordinate handling proof;
- headless long-duration simulation;
- evidence-backed engine decision packet recommending Godot or Unreal and marked decision-ready.

## Current blockers

The Phase 1 exit gate remains authoritative. Missing or insufficient rendering/engine evidence, a non-decision-ready engine packet, or any missing required technical proof blocks Phase 2 production gameplay.

## Automation operating state

Everward is being prepared for scheduled hourly autonomous development under `AGENT_DEVELOPMENT_POLICY.md`.

Every hourly run should:

1. inspect open PRs and CI first;
2. repair failed existing work before starting new work;
3. merge only work that is independently verified green and fully merge-ready;
4. otherwise advance one highest-value authorized slice of the current roadmap phase;
5. keep affected documentation current;
6. update this file when the continuation point materially changes.

## Repository posture

- Repository visibility: **Public by deliberate operational choice**.
- Project IP posture: **Proprietary, all rights reserved**.
- Public visibility does not grant an open-source license.
- Default branch: `main`.
- Substantive autonomous development: branch + pull request; no direct-to-main development.

## Exact continuation point

All five Phase 1 prototype directories exist, and every prototype's automated test suite is green, including `prototypes/test_phase1_exit_gate.py`, which is now discovered and run by `Foundation checks` CI (previously it existed and passed locally but had no CI step, so a regression in the gate script itself could have merged unverified; see `ERROR_RESOLUTION_LEDGER.md`, 2026-08-18).

The remaining Phase 1 blocker is the rendering-benchmark decision artifact itself: Prototype C's tooling (`prototypes/rendering-benchmark/`) is fully built and tested, but `prototypes/rendering-benchmark/finalize_engine_decision.py` still needs a real, decision-ready artifact produced from actual Godot and Unreal captures on the same physical hardware per `CAPTURE_RUNBOOK.md`. That capture is a physical-hardware/engine-execution task and cannot be produced by a headless automated run.

Until that hardware evidence exists, hourly automation should: verify no open PR/issue needs attention; re-run the full local test/foundation suite to confirm nothing regressed; and, if it finds one, close a further small, concretely evidenced gap in the Phase 1 tooling or its documentation (test coverage, stale contract, drifted fixture, etc.) rather than inventing new roadmap scope. Do not begin Phase 2 — One Probe until the exit gate actually passes with a real `decision_ready` artifact.

2026-08-18: closed one such gap. `docs/TESTING_STRATEGY.md` requires a golden-seed regression bank for procedural generation, but `prototypes/procedural-system` had only per-call equality and range/invariant tests across many seeds, with no pinned expected output — so an unintended change to `generator.py` could have silently altered canonical output for an existing `GENERATOR_VERSION` without failing any test. Added `prototypes/procedural-system/golden_seeds.json` (six diverse pinned cases: ordinary, zero-planet/sparse, resource-rich multi-body, rare O-class star, rare B-class star, large mixed-sign interstellar-scale coordinate) and `prototypes/procedural-system/test_golden_seeds.py`, which replays each case and asserts exact canonical JSON/fingerprint match. Verified the new test fails when a fixture value is deliberately corrupted and passes once restored. No generator, simulation, or benchmark behavior changed.

2026-08-19: closed the same class of gap in `prototypes/headless-simulation`. That prototype's `test_headless.py` proved strong invariants (identical seed/horizon replays exactly, checkpoint/restore matches an uninterrupted run, exact event counts for one specific horizon) but every determinism assertion compared `HeadlessSimulation` output against another call to the same code, never against a value pinned before the comparison existed — so an unintended change to the deterministic-accumulator formula in `runner.py` (a different hash-digest slice, byte order, or hash input format) would have satisfied every existing assertion without failing anything. Added `prototypes/headless-simulation/golden_runs.json` (six diverse pinned cases: ordinary, minimal single-maintenance-year horizon, exact 100-year archive-boundary horizon, negative seed, zero seed, and the 10,000-year Phase 1 proof-scale horizon) and `prototypes/headless-simulation/test_golden_runs.py`, which replays each case and asserts exact canonical-summary/fingerprint match. Verified the new test fails when a fixture value is deliberately corrupted and passes once restored. No `runner.py`, simulation, or benchmark behavior changed. No CI workflow change was needed: `Foundation checks` already runs `python -m unittest discover -s prototypes/headless-simulation -p 'test_*.py' -v`, which discovers the new test file automatically.

2026-08-19 (second slice): the entry directly above named `prototypes/simulation-clock` and `prototypes/coordinate-scale` as remaining candidates for this pattern, contingent on either module growing a non-trivial derived value. Re-checked both. `prototypes/coordinate-scale/test_coordinates.py` already asserts exact hand-computable literal values (e.g. specific `cell`/`offset_mm` components after a boundary-crossing translation), so no change was needed there. `prototypes/simulation-clock/test_clock.py` did have the gap: `set_time_scale()` drives a fractional remainder-carry accumulator in `SimulationClock.advance_wall_ticks()` (exactly such a non-trivial derived value), and every existing scaling test compared two separate `SimulationClock` runs against each other rather than against an independently computed expected value. Added `test_wall_tick_scaling_matches_independently_computed_floor_division` and `test_wall_tick_scaling_matches_closed_form_across_irregular_chunk_sizes` to `prototypes/simulation-clock/test_clock.py`, asserting the accumulator's tick against the closed-form `n * numerator // denominator` computed independently of `SimulationClock`. Verified: with the accumulator's floor division temporarily replaced by `round()`, all 9 pre-existing tests still passed but both new tests failed; reverting restored all 11 passing. No `clock.py` behavior changed. Updated `prototypes/simulation-clock/PROOF.md` and `CHANGELOG.md` to record the new evidence. No CI workflow change was needed: `Foundation checks` already runs `python -m unittest discover -s prototypes/simulation-clock -p 'test_*.py' -v`, which discovers the new tests automatically.

No further candidate of this golden/pinned-output kind is known at this time. Future hourly runs should look for a fresh concretely-evidenced gap (test coverage, stale contract, drifted fixture, etc.) rather than assuming one still exists here.

## Maintenance rule

Update this document in the same PR whenever a change materially alters:

- current phase;
- current milestone/exit gate;
- active blocking work;
- automation operating state;
- repository posture relevant to development;
- exact continuation point.

Historical rationale belongs in `DECISION_LOG.md`; significant failure history belongs in `ERROR_RESOLUTION_LEDGER.md`.
