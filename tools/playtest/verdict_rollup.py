#!/usr/bin/env python3
"""Roll up per-finding playtest classification strings into a single verdict.

Pure logic only: no file, path, stream, clock, random or network input.
"""

from __future__ import annotations

_SEVERITY_BY_CANONICAL_VERDICT: dict[str, int] = {
    "PASS": 0,
    "PARTIAL": 1,
    "FAIL": 2,
    "BLOCKER": 3,
}


def rollup_playtest_verdict(findings: list[str]) -> str:
    """Return the highest-severity verdict (BLOCKER > FAIL > PARTIAL > PASS).

    Entries are matched case-insensitively after stripping whitespace.
    Unrecognized or empty entries are ignored. Duplicates do not change the
    result. An empty list, or a list with no recognized entries, returns
    "PASS".
    """
    highest_verdict = "PASS"
    highest_severity = -1
    for finding in findings:
        canonical = finding.strip().upper()
        severity = _SEVERITY_BY_CANONICAL_VERDICT.get(canonical)
        if severity is not None and severity > highest_severity:
            highest_severity = severity
            highest_verdict = canonical
    return highest_verdict
