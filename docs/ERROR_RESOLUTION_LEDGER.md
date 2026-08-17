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
