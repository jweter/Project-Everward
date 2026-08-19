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

2026-08-19 (third slice): closed a related but distinct gap in `prototypes/rendering-benchmark`, the prototype whose scoring formula (`benchmark.py:score_result()`) directly produces the weighted recommendation feeding the Phase 1 exit-gate decision packet. `DEFAULT_WEIGHTS` deliberately assigns a different weight to each of the thirteen `REQUIRED_METRICS`, but every existing test exercised it with a uniform metrics vector (or a single outlier), which makes the weighted sum collapse to `value * sum(weights)` and so cannot detect two weights being wired to the wrong metric. Added `test_score_applies_each_metrics_own_declared_weight` to `prototypes/rendering-benchmark/test_benchmark.py`, using thirteen distinct per-metric values checked against a literal expected weighted sum computed independently of `score_result()`'s own iteration. Verified: the new test fails (`4.605 != 5.325`) when `visual_fidelity` and `memory_use` weights are deliberately swapped, while every pre-existing test in `test_benchmark.py`, `test_decision_packet.py`, and `test_normalization.py` still passes under that same mutation; reverting the mutation restores all tests, including the new one. Full `prototypes/rendering-benchmark` suite now 113 tests, all green. No `benchmark.py`, `decision_packet.py`, `normalization.py`, or any other production/simulation behavior changed. No CI workflow change needed: `Foundation checks` already runs `python -m unittest discover -s prototypes/rendering-benchmark -p 'test_*.py' -v`, which discovers the new test automatically. See `ERROR_RESOLUTION_LEDGER.md`, 2026-08-19, for full detail.

2026-08-19 (fourth slice): closed a sibling instance of the same weight-wiring gap, this time in `prototypes/rendering-benchmark/decision_packet.py`. The third slice above covered `benchmark.py:score_result()`, but `decision_packet.py:_metric_deltas()` independently re-applies `DEFAULT_WEIGHTS[metric]` to each metric's left/right score delta to build the decision packet's `top_differentiators`/`all_metric_deltas` evidence tables, and every existing `test_decision_packet.py` case left at most one metric different between left and right (the rest tied at an identical uniform qualitative score), so every tied metric's weighted_delta was 0 regardless of which weight it was paired with — the same degenerate-input blind spot, in a second call site. Added `test_metric_deltas_apply_each_metrics_own_declared_weight`, giving all thirteen `REQUIRED_METRICS` distinct, non-tied left/right deltas and checking each row's `weighted_delta` against a value computed with literal weights (copied from and asserted equal to `DEFAULT_WEIGHTS`) independent of `_metric_deltas()`'s own iteration. Verified: with `DEFAULT_WEIGHTS["visual_fidelity"]` and `DEFAULT_WEIGHTS["memory_use"]` swapped, both this new test and the existing `test_score_applies_each_metrics_own_declared_weight` fail while all 112 other rendering-benchmark tests still pass; reverting restores all 114 passing. No `benchmark.py`, `decision_packet.py`, `normalization.py`, or other production/simulation behavior changed.

2026-08-19 (fifth slice): closed a related mapping-blind-spot gap, this time in `prototypes/rendering-benchmark/normalization.py`. `normalize_pair()` derives each of the five quantitative benchmark metrics (`cpu_frame_time`, `gpu_frame_time`, `memory_use`, `build_distribution_complexity`, `developer_iteration_speed`) from its own named capture field via the `QUANTITATIVE_SOURCES` dict — the mapping that determines which raw hardware-capture measurement backs which metric in the Phase 1 engine-decision evidence. The existing `test_pair_normalization_derives_objective_scores_and_keeps_audit` test only asserted scores/capture_field for `cpu_frame_time` and `gpu_frame_time`, and separately used `memory`/`build` fixture values with an identical 2:1 ratio, so neither the unchecked audit `capture_field` nor the resulting score could have revealed `memory_use` and `build_distribution_complexity` being wired to each other's capture fields. Added `test_pair_normalization_maps_each_quantitative_metric_to_its_own_capture_field` to `prototypes/rendering-benchmark/test_normalization.py`, giving all five quantitative metrics distinct, pairwise-distinct left/right ratios (2.0, 3.0, 1.5, 1.1, 2.5) and asserting each metric's audited `capture_field` and derived score independently. Verified: the new test fails (`'build_size_mib' != 'peak_memory_mib'`) when `QUANTITATIVE_SOURCES["memory_use"]` and `QUANTITATIVE_SOURCES["build_distribution_complexity"]` are deliberately swapped, while all 114 other rendering-benchmark tests still pass under that same mutation (confirming the pre-existing suite could not have caught it); reverting restores all 115 passing. No `normalization.py`, `benchmark.py`, `decision_packet.py`, or other production/simulation behavior changed. No CI workflow change needed: `Foundation checks` already runs `python -m unittest discover -s prototypes/rendering-benchmark -p 'test_*.py' -v`, which discovers the new test automatically. See `ERROR_RESOLUTION_LEDGER.md`, 2026-08-19, for full detail.

Future hourly runs: no further concretely-evidenced gap of this kind is known in `prototypes/rendering-benchmark` at this time; look for a fresh gap across the Phase 1 tooling rather than assuming one still exists here. The Phase 1 exit gate itself remains blocked only on the real hardware-capture decision artifact described above.

2026-08-19 (sixth slice): closed a related coverage gap in `prototypes/procedural-system`'s golden-seed bank rather than `rendering-benchmark`. `generator.py`'s `STAR_TABLE` declares seven weighted spectral classes (`M`, `K`, `G`, `F`, `A`, `B`, `O`), but `golden_seeds.json` pinned only six cases, covering `K`, `G`, `M`, `O`, `B`, and `F` — `A` (the third-rarest class) had no pinned case anywhere in the bank. `test_generator.py`'s `test_star_properties_stay_inside_declared_class_ranges` cannot substitute for this: it looks up the expected range from the same `STAR_RANGES[spectral_class]` the generator used to produce the value, so it would still pass even if `STAR_RANGES` entries were reshuffled between classes; only a pinned golden case with an independently-fixed `spectral_class` can catch that. Added a `rare_a_class_star` case (seed 115, coordinate `(115, -4, 7)`, 6 planets, 1 belt) to `golden_seeds.json`, and added `test_every_star_table_spectral_class_is_pinned_somewhere_in_the_bank` to `test_golden_seeds.py`, asserting the full set of pinned spectral classes equals `STAR_TABLE`'s declared classes. Verified: with the new case's fixture `spectral_class` deliberately corrupted from `A` to `K`, the new test and `test_rare_spectral_classes_are_represented` both failed while the rest of the suite still ran; reverting restored all 18 `prototypes/procedural-system` tests passing (up from 17). No `generator.py`, simulation, or benchmark behavior changed. No CI workflow change was needed: `Foundation checks` already runs `python -m unittest discover -s prototypes/procedural-system -p 'test_*.py' -v`, which discovers the new test automatically.

Future hourly runs: no further concretely-evidenced gap of this kind is known in `prototypes/procedural-system` at this time either; look for a fresh gap across the Phase 1 tooling rather than assuming one still exists in either prototype covered above. The Phase 1 exit gate itself remains blocked only on the real hardware-capture decision artifact described above.

## Maintenance rule

Update this document in the same PR whenever a change materially alters:

- current phase;
- current milestone/exit gate;
- active blocking work;
- automation operating state;
- repository posture relevant to development;
- exact continuation point.

Historical rationale belongs in `DECISION_LOG.md`; significant failure history belongs in `ERROR_RESOLUTION_LEDGER.md`.
