"""Render deterministic Everward state evidence for mobile review.

This module is presentation-only: authoritative simulation state is supplied by
callers and is never mutated or reimplemented here.
"""

from __future__ import annotations

from html import escape
from typing import Any, Mapping

_COMPONENT_STATUSES = {
    "PASS",
    "FAIL",
    "NOT TESTED",
    "REVIEW_REQUIRED",
    "ENVIRONMENT_FAILURE",
    "PRODUCT REALITY REQUIRED",
    "PRODUCT_REALITY_REQUIRED",
}
_PRODUCT_REALITY_STATUSES = {
    "PRODUCT REALITY REQUIRED",
    "PRODUCT_REALITY_REQUIRED",
}


def render_state_report(evidence: Mapping[str, Any]) -> str:
    commit = _required(evidence, "commit")
    scenario = _required(evidence, "scenario")
    seed = _required(evidence, "seed")
    initial = evidence.get("initial_state", {})
    final = evidence.get("final_state", {})
    invariants = evidence.get("invariants", {})
    components = evidence.get("components", {})
    scenario_views = evidence.get("scenario_views", {})
    if not isinstance(initial, Mapping) or not isinstance(final, Mapping):
        raise ValueError("initial_state and final_state must be mappings")
    if not isinstance(invariants, Mapping):
        raise ValueError("invariants must be a mapping")
    if any(type(value) is not bool for value in invariants.values()):
        raise ValueError("invariant results must be booleans")
    if not isinstance(components, Mapping):
        raise ValueError("components must be a mapping")
    if not isinstance(scenario_views, Mapping):
        raise ValueError("scenario_views must be a mapping")

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
    component_cards = "".join(
        _render_component_card(name, value, commit)
        for name, value in sorted(components.items(), key=lambda item: str(item[0]))
    )
    components_section = (
        f"<section><h2>Functional test console</h2><div class=\"cards\">{component_cards}</div></section>"
        if component_cards
        else ""
    )
    scenario_section = _render_scenario_timeline_section(scenario_views)
    return f"""<!doctype html><html lang=\"en\"><head><meta charset=\"utf-8\">
<meta name=\"viewport\" content=\"width=device-width,initial-scale=1,viewport-fit=cover\">
<title>Everward Mobile State Review</title><style>
body{{font-family:-apple-system,BlinkMacSystemFont,system-ui,sans-serif;margin:0;padding:16px;background:#f5f5f7;color:#1d1d1f}}
main{{max-width:760px;margin:auto}}section{{background:white;border-radius:14px;padding:14px;margin:12px 0;overflow-x:auto}}
table{{width:100%;border-collapse:collapse}}th,td{{padding:8px;border-bottom:1px solid #ddd;text-align:left}}code{{overflow-wrap:anywhere}}
.cards{{display:grid;grid-template-columns:repeat(auto-fit,minmax(220px,1fr));gap:10px}}.card{{border:1px solid #ddd;border-radius:12px;padding:12px}}
.card h3{{margin:0 0 8px}}.status{{font-weight:700}}details{{margin-top:8px}}details p{{margin:6px 0}}
.scenario-picker{{display:grid;gap:8px}}select{{font:inherit;max-width:100%;padding:8px}}.timeline{{padding-left:22px}}.timeline li{{margin:10px 0}}
.timeline p{{margin:4px 0 0}}[hidden]{{display:none!important}}
</style></head><body><main><h1>Everward State Review</h1>
<section><p><strong>Scenario:</strong> {escape(scenario)}</p><p><strong>Seed:</strong> {escape(seed)}</p>
<p><strong>Commit:</strong> <code>{escape(commit)}</code></p></section>
{scenario_section}
{components_section}
<section><h2>Deterministic state diff</h2><table><tr><th>Field</th><th>Initial</th><th>Final</th></tr>{rows}</table></section>
<section><h2>Automated invariants</h2><ul>{checks}</ul></section>
<section><strong>Human Product Reality:</strong> UNREVIEWED<p>Unreal rendering, game feel, HUD placement, collision feel, packaging, and performance remain separate engine acceptance debt.</p></section>
</main></body></html>"""


def _render_scenario_timeline_section(scenario_views: Mapping[str, Any]) -> str:
    if not scenario_views:
        return ""

    options: list[str] = []
    panels: list[str] = []
    normalized_ids: set[str] = set()
    for index, (scenario_id, raw) in enumerate(
        sorted(scenario_views.items(), key=lambda item: str(item[0]))
    ):
        if not isinstance(raw, Mapping):
            raise ValueError("scenario view entries must be mappings")
        normalized_id = str(scenario_id).strip()
        if not normalized_id:
            raise ValueError("scenario view ids must not be empty")
        if normalized_id in normalized_ids:
            raise ValueError("scenario view ids must be unique after normalization")
        normalized_ids.add(normalized_id)
        label = str(raw.get("label", normalized_id)).strip()
        if not label:
            raise ValueError("scenario view labels must not be empty")
        events = raw.get("events", [])
        if not isinstance(events, list):
            raise ValueError("scenario view events must be lists")

        selected = " selected" if index == 0 else ""
        hidden = "" if index == 0 else " hidden"
        escaped_id = escape(normalized_id)
        options.append(
            f'<option value="{escaped_id}"{selected}>{escape(label)}</option>'
        )
        event_items = "".join(_render_timeline_event(event) for event in events)
        if not event_items:
            event_items = "<li>No deterministic events supplied.</li>"
        panels.append(
            f'<article class="scenario-panel" data-scenario="{escaped_id}"{hidden}>'
            f"<h3>{escape(label)}</h3><ol class=\"timeline\">{event_items}</ol></article>"
        )

    return (
        '<section><h2>Scenario timeline</h2><div class="scenario-picker">'
        '<label for="scenario-selector"><strong>Inspect scenario</strong></label>'
        f'<select id="scenario-selector">{"".join(options)}</select></div>'
        f'{"".join(panels)}'
        "<script>(function(){const selector=document.getElementById('scenario-selector');"
        "if(!selector)return;const panels=document.querySelectorAll('.scenario-panel');"
        "const show=function(){panels.forEach(function(panel){panel.hidden=panel.dataset.scenario!==selector.value;});};"
        "selector.addEventListener('change',show);show();})();</script></section>"
    )


def _render_timeline_event(raw: Any) -> str:
    if not isinstance(raw, Mapping):
        raise ValueError("scenario timeline events must be mappings")
    at = _required(raw, "at")
    event = _required(raw, "event")
    detail = str(raw.get("detail", "")).strip()
    detail_html = f"<p>{escape(detail)}</p>" if detail else ""
    return f"<li><strong>{escape(at)}</strong> — {escape(event)}{detail_html}</li>"


def _render_component_card(name: Any, raw: Any, report_commit: str) -> str:
    if not isinstance(raw, Mapping):
        raise ValueError("component entries must be mappings")
    status = _required(raw, "status")
    if status not in _COMPONENT_STATUSES:
        raise ValueError(f"invalid component status: {status}")
    scenario = _required(raw, "scenario")
    timestamp = _required(raw, "timestamp")
    component_commit = _required(raw, "commit") if "commit" in raw else report_commit
    detail = str(raw.get("detail", "No additional deterministic evidence supplied.")).strip()
    if status in _PRODUCT_REALITY_STATUSES:
        debt = _required(raw, "product_reality_debt")
    else:
        debt = str(raw.get("product_reality_debt", "None recorded for this component.")).strip()
    return (
        '<article class="card">'
        f"<h3>{escape(str(name))}</h3>"
        f'<p class="status">{escape(status)}</p>'
        "<details><summary>Evidence</summary>"
        f"<p><strong>Scenario/test:</strong> {escape(scenario)}</p>"
        f"<p><strong>Commit:</strong> <code>{escape(component_commit)}</code></p>"
        f"<p><strong>Timestamp:</strong> {escape(timestamp)}</p>"
        f"<p><strong>Evidence:</strong> {escape(detail)}</p>"
        f"<p><strong>Remaining Unreal debt:</strong> {escape(debt)}</p>"
        "</details></article>"
    )


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
