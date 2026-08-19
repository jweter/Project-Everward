# Error Resolution Ledger

This ledger records verified development and CI failures whose root causes are useful to preserve as project knowledge. Entries should be concise, evidence-based, and focused on preventing recurrence.

## 2026-08-17 — Rendering benchmark test import failed on Python 3.12

- **Observed failure:** `Foundation checks` failed while importing `prototypes/rendering-benchmark/test_benchmark.py`; all earlier Phase 1 prototype suites passed.
- **Error:** `AttributeError: 'NoneType' object has no attribute '__dict__'` inside `dataclasses._is_type` while applying `@dataclass` in `benchmark.py`.
- **Root cause:** the test loaded `benchmark.py` with `importlib.util.module_from_spec()` and executed it directly without first registering the module in `sys.modules`. Python 3.12 `dataclasses` resolves annotation/module context through `sys.modules[cls.__module__]`, so the nonstandard loader left that lookup missing.
- **Fix:** register the dynamically created module in `sys.modules` before `spec.loader.exec_module(benchmark)`.
- **Scope:** test-loader correction only; no benchmark scoring or production simulation behavior changed.
- **Prevention:** when tests dynamically load modules that define dataclasses or other module-introspective constructs, emulate normal import semantics by inserting the module into `sys.modules` before execution, or prefer a normal package import where practical.
- **Verification:** fresh GitHub Actions run required on the updated PR head.

## 2026-08-17 — Rendering benchmark pipeline tests drifted from canonical scenario identity

- **Observed failure:** PR #13 `Foundation checks` passed foundation, simulation-clock, procedural-system, coordinate-scale, and headless-simulation suites, then failed two rendering benchmark pipeline tests.
- **Error:** `ValueError: run record scenario_name does not match scenario` in `run_record.validate_run_record()`.
- **Root cause:** the new pipeline test fixture hard-coded `scenario_name` as `icy_asteroid_mining_v1`, while the authoritative `scenario.json` contract names the scenario `icy-asteroid-mining`. The validator correctly rejected the stale test fixture.
- **Fix:** derive both `scenario_version` and `scenario_name` in pipeline fixtures directly from the canonical `scenario.json`, and assert against that same authoritative identity instead of duplicating literals.
- **Scope:** test-fixture correction only; validation, normalization, scoring, and production simulation behavior remain unchanged.
- **Prevention:** tests for versioned/canonical contracts should load identity fields from the authoritative fixture unless the purpose of the test is explicitly to verify mismatch rejection. Avoid duplicating contract identifiers across test helpers.
- **Verification:** fresh GitHub Actions run required on the updated PR head.

## 2026-08-17 — Rendering scene manifest lagged canonical scenario version

- **Observed failure:** PR #17 `Foundation checks` passed every prior Phase 1 suite and 46 rendering-benchmark tests, then failed all six new `test_scene_state.py` cases.
- **Error:** `ValueError: scene manifest scenario_version does not match scenario` from `scene_state.validate_manifest()`.
- **Root cause:** the canonical rendering scenario had already advanced to `scenario_version: 2` when deterministic playback timing was added, but the newly introduced `scene_manifest.json` was authored with stale `scenario_version: 1`. The validator correctly treated the manifest as incompatible rather than silently accepting drift.
- **Fix:** bind `scene_manifest.json` to canonical scenario version 2. No scene geometry, animation timing, playback logic, or simulation behavior changed.
- **Scope:** benchmark metadata correction only; renderer-neutral scene truth remains unchanged.
- **Prevention:** every new persisted artifact that binds to a versioned canonical contract must copy its identity from the current authoritative contract at creation time and include a CI test that validates the binding before any deeper behavior assertions. Treat version mismatch as a hard failure, never an implicit migration.
- **Verification:** fresh GitHub Actions run required on the updated PR head.

## 2026-08-18 — Capture readiness audit referenced stale Unreal source filenames

- **Observed failure risk:** the Phase 1 hardware-capture readiness contract required `BenchmarkCaptureSession.h/.cpp`, but the merged Unreal implementation uses `BenchmarkCaptureSessionComponent.h/.cpp`. The synthetic test fixture created whatever paths the contract requested, so CI could pass while the audit remained inconsistent with the real repository layout.
- **Root cause:** the readiness file list was authored against an earlier Unreal capture-session name and the fixture tests derived their filesystem entirely from that same list, creating a self-consistent but stale test oracle.
- **Fix:** update the Unreal required paths to `BenchmarkCaptureSessionComponent.h/.cpp` and add a regression test that asserts every engine-required path exists in the real repository tree.
- **Scope:** readiness-audit and regression-test correction only; engine runtime, benchmark measurements, simulation truth, and scoring logic are unchanged.
- **Prevention:** structural readiness tests must validate declared paths against the real checked-in repository layout in addition to isolated synthetic fixtures. Do not let the configuration under test also generate the only source of truth used to verify itself.
- **Verification:** fresh Foundation checks required on the corrective PR head before merge.

## 2026-08-18 — Phase 1 exit gate audit had no CI coverage

- **Observed failure risk:** `prototypes/phase1_exit_gate.py` is the executable contract for the Phase 1 → Phase 2 gate, and `prototypes/test_phase1_exit_gate.py` fully unit-tests it and passed locally, but `Foundation checks` only ran `unittest discover` scoped to each individual prototype subdirectory (`simulation-clock`, `procedural-system`, `coordinate-scale`, `headless-simulation`, `rendering-benchmark`). The top-level `prototypes/test_phase1_exit_gate.py` was never discovered or executed in CI, so a regression in the gate script itself could merge without any independent verification.
- **Root cause:** the workflow was written prototype-by-prototype before the cross-cutting exit-gate audit and its test file were added at `prototypes/` root, and no step was added to cover that location.
- **Fix:** add a `Test Phase 1 exit gate audit` step running `python -m unittest discover -s prototypes -p 'test_*.py' -t prototypes -v`. Confirmed this scope discovers only `test_phase1_exit_gate.py` (the hyphenated prototype subdirectories are not importable Python packages, so `unittest discover` does not descend into them and no test is run twice).
- **Scope:** CI coverage only; `phase1_exit_gate.py` behavior is unchanged.
- **Prevention:** when a new cross-cutting script and test file are added outside the existing per-prototype directories, add an explicit CI discovery step for that location rather than assuming an existing step already covers it.
- **Verification:** fresh GitHub Actions run required on the updated PR head.

## 2026-08-19 — Rendering benchmark scoring weights were untested per-metric

- **Observed failure risk:** `prototypes/rendering-benchmark/benchmark.py:score_result()` is the formula that produces the weighted score behind the Phase 1 engine recommendation (`decision_packet.py` → `PHASE1_EXIT_GATE.md`). Its `DEFAULT_WEIGHTS` dict deliberately assigns different weights per metric (comment: "Weights intentionally favor the things that are hardest to retrofit later"), but every existing test (`test_benchmark.py`, `test_decision_packet.py`, `test_normalization.py`) passed a metrics vector with the same value repeated across all thirteen `REQUIRED_METRICS` (uniform 8.0, or one outlier metric in `test_differentiators_are_sorted_by_weighted_effect`). A uniform vector makes `score_result` collapse to `value * sum(weights)`, which is identical regardless of which weight is paired with which metric name, so no test could detect two weights being swapped between metrics (e.g. a future rebalancing edit or refactor that iterates `REQUIRED_METRICS`/`DEFAULT_WEIGHTS` out of step).
- **Root cause:** all prior scoring tests used degenerate (uniform, or single-outlier) input data that happened to make the per-metric weighting mechanism unobservable, so passing tests gave no real assurance the weight-to-metric wiring was correct.
- **Fix:** added `test_score_applies_each_metrics_own_declared_weight` to `prototypes/rendering-benchmark/test_benchmark.py`, using thirteen distinct per-metric values and asserting `score_result()` against a literal expected weighted sum computed independently of `benchmark.py`'s own iteration.
- **Scope:** test coverage only; `benchmark.py`, `decision_packet.py`, and `normalization.py` behavior unchanged.
- **Prevention:** when a weighted/keyed formula is safety- or decision-relevant, cover it with at least one test whose input values are all distinct so the mapping between keys and their weights/coefficients is actually exercised, not just their sum or count.
- **Verification:** confirmed the new test fails (`4.605 != 5.325`) when `DEFAULT_WEIGHTS["visual_fidelity"]` and `DEFAULT_WEIGHTS["memory_use"]` are deliberately swapped, and that every other test in `test_benchmark.py`, `test_decision_packet.py`, and `test_normalization.py` still passes under that same mutation (proving the pre-existing suite could not have caught it). Reverted the mutation; full `prototypes/rendering-benchmark` suite (113 tests) and the complete Foundation checks command sequence pass clean.

## 2026-08-19 — Rendering benchmark decision-packet deltas were also untested per-metric

- **Observed failure risk:** the entry directly above fixed `benchmark.py:score_result()`'s untested per-metric weighting, but `decision_packet.py:_metric_deltas()` independently re-implements the same `(score delta) * DEFAULT_WEIGHTS[metric]` pattern to build the `top_differentiators`/`all_metric_deltas` evidence in the Phase 1 engine-decision packet. Every existing `test_decision_packet.py` case constructed left/right qualitative assessments that were uniform except for at most one metric (`visual_fidelity`), so every other metric tied (identical left/right score, weighted_delta always 0 regardless of weight). No test could detect `_metric_deltas()` pairing a metric with the wrong `DEFAULT_WEIGHTS` entry.
- **Root cause:** same class as the entry above (degenerate/near-uniform test inputs make a per-key weighting mechanism unobservable), recurring at a second call site that consumes the same weights dict independently rather than through `score_result()`.
- **Fix:** added `test_metric_deltas_apply_each_metrics_own_declared_weight` to `prototypes/rendering-benchmark/test_decision_packet.py`, giving all thirteen `REQUIRED_METRICS` distinct, non-tied left/right deltas (five via distinct quantitative capture values, eight via distinct qualitative assessments) and asserting each row's `weighted_delta` against a value computed with literal weight constants (checked equal to `DEFAULT_WEIGHTS`), independent of `_metric_deltas()`'s own dict lookup and iteration.
- **Scope:** test coverage only; `benchmark.py`, `decision_packet.py`, and `normalization.py` behavior unchanged.
- **Prevention:** when a shared weighted/keyed formula is reused at more than one call site, cover the weighting at each call site independently — a fix for one function's degenerate-input blind spot does not exercise a sibling function that re-reads the same weights dict.
- **Verification:** confirmed the new test fails together with `test_score_applies_each_metrics_own_declared_weight` when `DEFAULT_WEIGHTS["visual_fidelity"]` and `DEFAULT_WEIGHTS["memory_use"]` are deliberately swapped, while all 112 other rendering-benchmark tests still pass under that same mutation. Reverted the mutation; full `prototypes/rendering-benchmark` suite (114 tests) and the complete Foundation checks command sequence pass clean.
