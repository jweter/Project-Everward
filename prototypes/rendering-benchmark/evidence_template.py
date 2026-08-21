"""Create incomplete-but-structured capture scaffolds for Unreal Phase 1 rendering runs."""

from __future__ import annotations

from typing import Any, Mapping

from scenario import REQUIRED_CAPTURE_FIELDS

PRODUCTION_ENGINE = "unreal"


def create_evidence_template(engine: str, scenario: Mapping[str, Any]) -> dict[str, Any]:
    """Return a scenario-bound evidence scaffold for Unreal."""
    if engine != PRODUCTION_ENGINE:
        raise ValueError("engine must be 'unreal'")

    capture: dict[str, Any] = {field: None for field in sorted(REQUIRED_CAPTURE_FIELDS)}
    capture["project_settings"] = {}
    capture["screenshots"] = []
    capture["notes"] = ""

    return {
        "record_version": 1,
        "engine": engine,
        "scenario_version": scenario["scenario_version"],
        "scenario_name": scenario["name"],
        "captured_at_utc": "",
        "capture": capture,
    }


def missing_evidence_fields(record: Mapping[str, Any]) -> list[str]:
    missing: list[str] = []
    captured_at = record.get("captured_at_utc")
    if not isinstance(captured_at, str) or not captured_at.strip():
        missing.append("captured_at_utc")

    capture = record.get("capture")
    if not isinstance(capture, Mapping):
        return missing + ["capture"]

    for field in sorted(REQUIRED_CAPTURE_FIELDS):
        if field not in capture:
            missing.append(f"capture.{field}")
            continue
        value = capture[field]
        if value is None:
            missing.append(f"capture.{field}")
        elif isinstance(value, str) and field != "notes" and not value.strip():
            missing.append(f"capture.{field}")
        elif field == "project_settings" and (not isinstance(value, Mapping) or not value):
            missing.append("capture.project_settings")
        elif field == "screenshots" and (not isinstance(value, list) or not value):
            missing.append("capture.screenshots")
    return missing
