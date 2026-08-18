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
