"""Render deterministic Everward state evidence for mobile review.

This module is presentation-only: authoritative simulation state is supplied by
callers and is never mutated or reimplemented here.
"""

from __future__ import annotations

from html import escape
from typing import Any, Mapping


def render_state_report(evidence: Mapping[str, Any]) -> str:
    commit = _required(evidence, "commit")
    scenario = _required(evidence, "scenario")
    seed = _required(evidence, "seed")
    initial = evidence.get("initial_state", {})
    final = evidence.get("final_state", {})
    invariants = evidence.get("invariants", {})
    if not isinstance(initial, Mapping) or not isinstance(final, Mapping):
        raise ValueError("initial_state and final_state must be mappings")
    if not isinstance(invariants, Mapping):
        raise ValueError("invariants must be a mapping")
    if any(type(value) is not bool for value in invariants.values()):
        raise ValueError("invariant results must be booleans")

    keys = sorted(set(initial) | set(final), key=str)
    rows = "".join(
        f"<tr><th>{escape(str(key))}</th><td>{escape(str(initial.get(key, 'MISSING')))}</td>"
        f"<td>{escape(str(final.get(key, 'MISSING')))}</td></tr>"
        for key in keys
    )
    checks = "".join(
        f"<li><strong>{escape(str(name))}</strong>: {'PASS' if value else 'FAIL'}</li>"
        for name, value in sorted(invariants.items(), key=lambda item: str(item[0]))
    )
    return f"""<!doctype html><html lang=\"en\"><head><meta charset=\"utf-8\">
<meta name=\"viewport\" content=\"width=device-width,initial-scale=1,viewport-fit=cover\">
<title>Everward Mobile State Review</title><style>
body{{font-family:-apple-system,BlinkMacSystemFont,system-ui,sans-serif;margin:0;padding:16px;background:#f5f5f7;color:#1d1d1f}}
main{{max-width:760px;margin:auto}}section{{background:white;border-radius:14px;padding:14px;margin:12px 0;overflow-x:auto}}
table{{width:100%;border-collapse:collapse}}th,td{{padding:8px;border-bottom:1px solid #ddd;text-align:left}}code{{overflow-wrap:anywhere}}
</style></head><body><main><h1>Everward State Review</h1>
<section><p><strong>Scenario:</strong> {escape(scenario)}</p><p><strong>Seed:</strong> {escape(seed)}</p>
<p><strong>Commit:</strong> <code>{escape(commit)}</code></p></section>
<section><h2>Deterministic state diff</h2><table><tr><th>Field</th><th>Initial</th><th>Final</th></tr>{rows}</table></section>
<section><h2>Automated invariants</h2><ul>{checks}</ul></section>
<section><strong>Human Product Reality:</strong> UNREVIEWED<p>Unreal rendering, game feel, HUD placement, collision feel, packaging, and performance remain separate engine acceptance debt.</p></section>
</main></body></html>"""


def _required(data: Mapping[str, Any], key: str) -> str:
    value = data.get(key)
    if value is None:
        raise ValueError(f"missing required field: {key}")
    if isinstance(value, bool):
        raise ValueError(f"invalid required field: {key}")
    if not isinstance(value, (str, int)):
        raise ValueError(f"invalid required field: {key}")
    normalized = str(value).strip()
    if not normalized:
        raise ValueError(f"missing required field: {key}")
    return normalized
