"""Validate that two rendering benchmark captures are actually comparable.

The engine decision is only meaningful when Godot and Unreal were captured from
one canonical handoff, one source revision, and materially identical benchmark
hardware. This module keeps that evidence-integrity gate separate from scoring.
"""

from __future__ import annotations

from typing import Any, Mapping

from run_record import validate_run_record


REQUIRED_PROVENANCE_FIELDS = {
    "source_revision",
    "handoff_sha256",
}

HARDWARE_FIELDS = (
    "os_version",
    "cpu_model",
    "gpu_model",
    "ram_gib",
)


def _validate_provenance(label: str, provenance: Mapping[str, Any]) -> None:
    keys = set(provenance)
    if keys != REQUIRED_PROVENANCE_FIELDS:
        missing = sorted(REQUIRED_PROVENANCE_FIELDS - keys)
        unexpected = sorted(keys - REQUIRED_PROVENANCE_FIELDS)
        details = []
        if missing:
            details.append(f"missing: {', '.join(missing)}")
        if unexpected:
            details.append(f"unknown: {', '.join(unexpected)}")
        raise ValueError(
            f"invalid {label} provenance fields"
            + (f" ({'; '.join(details)})" if details else "")
        )

    for field in REQUIRED_PROVENANCE_FIELDS:
        value = provenance[field]
        if not isinstance(value, str) or not value.strip():
            raise ValueError(f"{label} provenance field {field!r} must be non-empty")


def validate_capture_pair(
    scenario: Mapping[str, Any],
    left_record: Mapping[str, Any],
    right_record: Mapping[str, Any],
    left_provenance: Mapping[str, Any],
    right_provenance: Mapping[str, Any],
) -> dict[str, object]:
    """Reject benchmark pairs that do not share one evidence provenance boundary."""
    validate_run_record(left_record, scenario)
    validate_run_record(right_record, scenario)

    if left_record["engine"] == right_record["engine"]:
        raise ValueError("capture pair requires two different engines")

    _validate_provenance("left", left_provenance)
    _validate_provenance("right", right_provenance)

    if left_provenance["source_revision"] != right_provenance["source_revision"]:
        raise ValueError("capture pair source revisions do not match")
    if left_provenance["handoff_sha256"] != right_provenance["handoff_sha256"]:
        raise ValueError("capture pair handoff hashes do not match")

    hardware_mismatches = []
    for field in HARDWARE_FIELDS:
        left_value = left_record["capture"][field]
        right_value = right_record["capture"][field]
        if left_value != right_value:
            hardware_mismatches.append(
                {
                    "field": field,
                    "left": left_value,
                    "right": right_value,
                }
            )

    if hardware_mismatches:
        fields = ", ".join(item["field"] for item in hardware_mismatches)
        raise ValueError(f"capture pair benchmark hardware does not match: {fields}")

    return {
        "pair_version": 1,
        "scenario_version": scenario["scenario_version"],
        "scenario_name": scenario["name"],
        "engines": [left_record["engine"], right_record["engine"]],
        "source_revision": left_provenance["source_revision"],
        "handoff_sha256": left_provenance["handoff_sha256"],
        "hardware": {
            field: left_record["capture"][field]
            for field in HARDWARE_FIELDS
        },
        "status": "comparable",
    }
