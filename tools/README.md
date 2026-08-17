# Tools

Developer and offline tooling belongs here.

Potential uses include astronomy analysis, procedural-generation inspection, save migration utilities, data conversion, profiling, visualization, content validation, benchmark harnesses, and test-fixture generation.

Python or other scripting languages may be used here without implying they become shipping runtime dependencies.

## Foundation validation

`check_foundation.py` is the dependency-free repository constitution check used by GitHub Actions.

Run it locally from the repository root with:

```text
python tools/check_foundation.py
```

This validates required project-foundation files, detects unresolved merge-conflict markers in text source, and checks that project documentation is non-empty. It is intentionally runnable without GitHub Actions so platform/account failures can be distinguished from project test failures.
