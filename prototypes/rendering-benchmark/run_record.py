"""Auditable raw run records for the Everward rendering benchmark.

A run record captures the measured evidence from one engine implementation of
one canonical scenario. It deliberately keeps raw measurements separate from
normalized scoring so later engine decisions can be reproduced and challenged.
"""

from __future__ import annotations

import json
from pathlib import Path
from typing import Any, Mapping

from scenario import REQUIRED_CAPTURE_FIELDS, load_scenario


REQUIRED_TOP_LEVEL = {
    "record_version",
    "engine",
    "scenario_version",
    "scenario_name",
    "captured_at_utc",
    "capture",
}

NUMERIC_NONNEGATIVE_FIELDS = {
    "cpu_frame_time_ms",
    "gpu_frame_time_ms",
    "peak_memory_mib",
    "implementation_hours",
    "build_size_mib",
}


def load_run_record(path: str | Path, scenario_path: str | Path) -> dict[str, Any]:
    """Load and validate a raw benchmark run record against a scenario."""
    with Path(path).open("r", encoding="utf-8") as handle:
        data = json.load(handle)
    scenario = load_scenario(scenario_path)
    validate_run_record(data, scenario)
    return data


def validate_run_record(data: Mapping[str, Any], scenario: Mapping[str, Any]) -> None:
    """Reject incomplete, mismatched, or obviously invalid benchmark evidence."""
    missing = sorted(REQUIRED_TOP_LEVEL - set(data))
    unexpected = sorted(set(data) - REQUIRED_TOP_LEVEL)
    if missing:
        raise ValueError(f"missing run record fields: {', '.join(missing)}")
    if unexpected:
        raise ValueError(f"unknown run record fields: {', '.join(unexpected)}")

    if data["record_version"] != 1:
        raise ValueError("unsupported record_version")

    engine = data["engine"]
    if engine not in {"godot", "unreal"}:
        raise ValueError("engine must be 'godot' or 'unreal'")

    if data["scenario_version"] != scenario["scenario_version"]:
        raise ValueError("run record scenario_version does not match scenario")
    if data["scenario_name"] != scenario["name"]:
        raise ValueError("run record scenario_name does not match scenario")

    captured_at = data["captured_at_utc"]
    if not isinstance(captured_at, str) or not captured_at.strip():
        raise ValueError("captured_at_utc must be a non-empty string")

    capture = data["capture"]
    if not isinstance(capture, Mapping):
        raise TypeError("capture must be an object")

    capture_keys = set(capture)
    if capture_keys != REQUIRED_CAPTURE_FIELDS:
        missing_capture = sorted(REQUIRED_CAPTURE_FIELDS - capture_keys)
        extra_capture = sorted(capture_keys - REQUIRED_CAPTURE_FIELDS)
        details = []
        if missing_capture:
            details.append(f"missing: {', '.join(missing_capture)}")
        if extra_capture:
            details.append(f"unknown: {', '.join(extra_capture)}")
        raise ValueError("invalid capture fields" + (f" ({'; '.join(details)})" if details else ""))

    for field in NUMERIC_NONNEGATIVE_FIELDS:
        value = capture[field]
        if isinstance(value, bool) or not isinstance(value, (int, float)):
            raise TypeError(f"capture field {field!r} must be numeric")
        if float(value) < 0:
            raise ValueError(f"capture field {field!r} cannot be negative")

    ram = capture["ram_gib"]
    if isinstance(ram, bool) or not isinstance(ram, (int, float)) or float(ram) <= 0:
        raise ValueError("capture field 'ram_gib' must be a positive number")

    screenshots = capture["screenshots"]
    if not isinstance(screenshots, list) or not screenshots:
        raise ValueError("capture field 'screenshots' must be a non-empty list")
    if any(not isinstance(item, str) or not item.strip() for item in screenshots):
        raise ValueError("screenshot entries must be non-empty strings")

    project_settings = capture["project_settings"]
    if not isinstance(project_settings, Mapping) or not project_settings:
        raise ValueError("capture field 'project_settings' must be a non-empty object")

    for field in ("engine_version", "os_version", "cpu_model", "gpu_model", "notes"):
        value = capture[field]
        if not isinstance(value, str):
            raise TypeError(f"capture field {field!r} must be a string")
        if field != "notes" and not value.strip():
            raise ValueError(f"capture field {field!r} must be non-empty")
