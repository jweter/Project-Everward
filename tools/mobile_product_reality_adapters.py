"""Adapters from authoritative Everward evidence into the mobile review surface.

These helpers are deliberately presentation-only. They validate and reshape evidence
that an authoritative deterministic lane already produced; they do not calculate game
state, decide whether mechanics are correct, or mutate saves/simulation state.
"""

from __future__ import annotations

from typing import Any, Mapping

_ALLOWED_STATUSES = {
    "PASS",
    "FAIL",
    "NOT TESTED",
    "REVIEW_REQUIRED",
    "PRODUCT REALITY REQUIRED",
    "PRODUCT_REALITY_REQUIRED",
    "ENVIRONMENT_FAILURE",
}


def adapt_persistence_evidence(evidence: Mapping[str, Any]) -> dict[str, Any]:
    """Shape authoritative save/load evidence for the mobile renderer."""

    return _adapt_authoritative_evidence(
        evidence,
        component_name="Save/Load",
        default_label="Persistence round trip",
    )


def adapt_mining_storage_evidence(evidence: Mapping[str, Any]) -> dict[str, Any]:
    """Shape authoritative mining/storage evidence for the mobile renderer."""

    return _adapt_authoritative_evidence(
        evidence,
        component_name="Mining/Storage",
        default_label="Mining and storage flow",
    )


def adapt_repair_evidence(evidence: Mapping[str, Any]) -> dict[str, Any]:
    """Shape authoritative Fix_It repair evidence for the mobile renderer."""

    return _adapt_authoritative_evidence(
        evidence,
        component_name="Fix_It Repair",
        default_label="Repair capability recovery",
    )


def adapt_tractor_field_evidence(evidence: Mapping[str, Any]) -> dict[str, Any]:
    """Shape authoritative tractor-field simulation evidence for mobile review."""

    return _adapt_authoritative_evidence(
        evidence,
        component_name="Tractor Field",
        default_label="Tractor-field coupling and motion",
    )


def _adapt_authoritative_evidence(
    evidence: Mapping[str, Any], *, component_name: str, default_label: str
) -> dict[str, Any]:
    commit = _required(evidence, "commit")
    scenario = _required(evidence, "scenario")
    timestamp = _required(evidence, "timestamp")
    status = _required(evidence, "status")
    if status not in _ALLOWED_STATUSES:
        raise ValueError(f"invalid authoritative status: {status}")

    events = evidence.get("events")
    if not isinstance(events, list):
        raise ValueError("authoritative events must be a list")
    normalized_events = [_normalize_event(event) for event in events]

    detail = str(evidence.get("detail", "Authoritative deterministic evidence supplied.")).strip()
    label = str(evidence.get("label", default_label)).strip()
    if not label:
        raise ValueError("authoritative label must not be empty")

    debt = str(evidence.get("product_reality_debt", "")).strip()
    if status in {"PRODUCT REALITY REQUIRED", "PRODUCT_REALITY_REQUIRED"} and not debt:
        raise ValueError("PRODUCT REALITY REQUIRED evidence must name remaining debt")
    if not debt:
        debt = "None recorded for this component."

    return {
        "components": {
            component_name: {
                "status": status,
                "scenario": scenario,
                "timestamp": timestamp,
                "commit": commit,
                "detail": detail,
                "product_reality_debt": debt,
            }
        },
        "scenario_views": {
            scenario: {
                "label": label,
                "events": normalized_events,
            }
        },
    }


def _normalize_event(raw: Any) -> dict[str, str]:
    if not isinstance(raw, Mapping):
        raise ValueError("authoritative events must be mappings")
    event = {
        "at": _required(raw, "at"),
        "event": _required(raw, "event"),
    }
    detail = str(raw.get("detail", "")).strip()
    if detail:
        event["detail"] = detail
    return event


def _required(data: Mapping[str, Any], key: str) -> str:
    value = data.get(key)
    if value is None or isinstance(value, bool) or not isinstance(value, (str, int)):
        raise ValueError(f"invalid required field: {key}")
    normalized = str(value).strip()
    if not normalized:
        raise ValueError(f"missing required field: {key}")
    return normalized
