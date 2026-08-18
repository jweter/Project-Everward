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
