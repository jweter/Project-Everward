"""Canonical scenario contract for the Everward rendering benchmark."""

from __future__ import annotations

import json
from pathlib import Path
from typing import Any, Mapping


REQUIRED_TOP_LEVEL = {
    "scenario_version",
    "name",
    "target_resolution",
    "target_fps",
    "duration_seconds",
    "simulation_seconds_per_real_second",
    "camera_sequence",
    "camera_stage_durations_seconds",
    "required_scene_features",
    "required_capture",
    "fairness_rules",
}

POSITIVE_INTEGER_FIELDS = (
    "target_fps",
    "duration_seconds",
    "simulation_seconds_per_real_second",
)

UNIQUE_STRING_LIST_FIELDS = (
    "camera_sequence",
    "required_scene_features",
    "fairness_rules",
)

REQUIRED_CAPTURE_FIELDS = {
    "engine_version",
    "os_version",
    "cpu_model",
    "gpu_model",
    "ram_gib",
    "project_settings",
    "cpu_frame_time_ms",
    "gpu_frame_time_ms",
    "peak_memory_mib",
    "implementation_hours",
    "build_size_mib",
    "screenshots",
    "notes",
}


def load_scenario(path: str | Path) -> dict[str, Any]:
    """Load and validate a benchmark scenario JSON document."""
    with Path(path).open("r", encoding="utf-8") as handle:
        data = json.load(handle)
    validate_scenario(data)
    return data


def validate_scenario(data: Mapping[str, Any]) -> None:
    """Reject incomplete or ambiguous benchmark-scene definitions."""
    missing = sorted(REQUIRED_TOP_LEVEL - set(data))
    unexpected = sorted(set(data) - REQUIRED_TOP_LEVEL)
    if missing:
        raise ValueError(f"missing scenario fields: {', '.join(missing)}")
    if unexpected:
        raise ValueError(f"unknown scenario fields: {', '.join(unexpected)}")

    if data["scenario_version"] != 2:
        raise ValueError("unsupported scenario_version")
    if not isinstance(data["name"], str) or not data["name"].strip():
        raise ValueError("scenario name must be non-empty")

    resolution = data["target_resolution"]
    if (
        not isinstance(resolution, list)
        or len(resolution) != 2
        or any(not isinstance(value, int) or value <= 0 for value in resolution)
    ):
        raise ValueError("target_resolution must contain two positive integers")

    for field in POSITIVE_INTEGER_FIELDS:
        if not isinstance(data[field], int) or data[field] <= 0:
            raise ValueError(f"{field} must be a positive integer")

    for field in UNIQUE_STRING_LIST_FIELDS:
        values = data[field]
        if not isinstance(values, list) or not values:
            raise ValueError(f"{field} must be a non-empty list")
        if any(not isinstance(value, str) or not value.strip() for value in values):
            raise ValueError(f"{field} entries must be non-empty strings")
        if len(values) != len(set(values)):
            raise ValueError(f"{field} entries must be unique")

    stage_durations = data["camera_stage_durations_seconds"]
    if (
        not isinstance(stage_durations, list)
        or len(stage_durations) != len(data["camera_sequence"])
        or any(not isinstance(value, int) or value <= 0 for value in stage_durations)
    ):
        raise ValueError("camera_stage_durations_seconds must contain one positive integer per camera stage")
    if sum(stage_durations) != data["duration_seconds"]:
        raise ValueError("camera_stage_durations_seconds must sum to duration_seconds")

    capture = data["required_capture"]
    if not isinstance(capture, list):
        raise ValueError("required_capture must be a list")
    capture_set = set(capture)
    if capture_set != REQUIRED_CAPTURE_FIELDS or len(capture) != len(capture_set):
        missing_capture = sorted(REQUIRED_CAPTURE_FIELDS - capture_set)
        extra_capture = sorted(capture_set - REQUIRED_CAPTURE_FIELDS)
        detail = []
        if missing_capture:
            detail.append(f"missing: {', '.join(missing_capture)}")
        if extra_capture:
            detail.append(f"unknown: {', '.join(extra_capture)}")
        if len(capture) != len(capture_set):
            detail.append("duplicates present")
        raise ValueError("invalid required_capture fields" + (f" ({'; '.join(detail)})" if detail else ""))
