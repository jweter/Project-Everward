"""Assemble decision-grade Unreal run records from measured capture observations."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any, Mapping, Sequence

from run_record import validate_run_record
from scenario import REQUIRED_CAPTURE_FIELDS, load_scenario

PRODUCTION_ENGINE = "unreal"
REQUIRED_OBSERVATION_FIELDS = {
    "observation_version",
    "engine",
    "scenario_name",
    "scenario_version",
    "captured_at_utc",
    "run_record_prefill",
    "manual_evidence_still_required",
}


def load_json_object(path: str | Path) -> dict[str, Any]:
    with Path(path).open("r", encoding="utf-8") as handle:
        value = json.load(handle)
    if not isinstance(value, dict):
        raise TypeError(f"{path} must contain a JSON object")
    return value


def validate_observation(observation: Mapping[str, Any], scenario: Mapping[str, Any]) -> None:
    missing = sorted(REQUIRED_OBSERVATION_FIELDS - set(observation))
    if missing:
        raise ValueError(f"missing observation fields: {', '.join(missing)}")
    if observation["observation_version"] != 1:
        raise ValueError("unsupported observation_version")
    if observation["engine"] != PRODUCTION_ENGINE:
        raise ValueError("observation engine must be 'unreal'")
    if observation["scenario_name"] != scenario["name"]:
        raise ValueError("observation scenario_name does not match scenario")
    if observation["scenario_version"] != scenario["scenario_version"]:
        raise ValueError("observation scenario_version does not match scenario")

    captured_at = observation["captured_at_utc"]
    if not isinstance(captured_at, str) or not captured_at.strip():
        raise ValueError("observation captured_at_utc must be non-empty")

    prefill = observation["run_record_prefill"]
    if not isinstance(prefill, Mapping) or not prefill:
        raise ValueError("run_record_prefill must be a non-empty object")

    manual_fields = observation["manual_evidence_still_required"]
    if not isinstance(manual_fields, list) or not manual_fields:
        raise ValueError("manual_evidence_still_required must be a non-empty list")
    if any(not isinstance(field, str) or not field for field in manual_fields):
        raise ValueError("manual evidence field names must be non-empty strings")
    if len(set(manual_fields)) != len(manual_fields):
        raise ValueError("manual evidence field names must be unique")

    prefill_fields = set(prefill)
    manual_field_set = set(manual_fields)
    overlap = sorted(prefill_fields & manual_field_set)
    if overlap:
        raise ValueError(f"measured and manual evidence overlap: {', '.join(overlap)}")

    combined_fields = prefill_fields | manual_field_set
    if combined_fields != REQUIRED_CAPTURE_FIELDS:
        missing_capture = sorted(REQUIRED_CAPTURE_FIELDS - combined_fields)
        extra_capture = sorted(combined_fields - REQUIRED_CAPTURE_FIELDS)
        details: list[str] = []
        if missing_capture:
            details.append(f"missing: {', '.join(missing_capture)}")
        if extra_capture:
            details.append(f"unknown: {', '.join(extra_capture)}")
        raise ValueError(
            "observation evidence partition does not cover run-record contract"
            + (f" ({'; '.join(details)})" if details else "")
        )


def assemble_run_record(
    observation: Mapping[str, Any],
    manual_evidence: Mapping[str, Any],
    scenario: Mapping[str, Any],
) -> dict[str, Any]:
    validate_observation(observation, scenario)

    required_manual = set(observation["manual_evidence_still_required"])
    supplied_manual = set(manual_evidence)
    if supplied_manual != required_manual:
        missing = sorted(required_manual - supplied_manual)
        extra = sorted(supplied_manual - required_manual)
        details: list[str] = []
        if missing:
            details.append(f"missing: {', '.join(missing)}")
        if extra:
            details.append(f"unexpected: {', '.join(extra)}")
        raise ValueError(
            "manual evidence must exactly match observation requirements"
            + (f" ({'; '.join(details)})" if details else "")
        )

    capture = dict(observation["run_record_prefill"])
    capture.update(manual_evidence)
    record = {
        "record_version": 1,
        "engine": PRODUCTION_ENGINE,
        "scenario_version": observation["scenario_version"],
        "scenario_name": observation["scenario_name"],
        "captured_at_utc": observation["captured_at_utc"],
        "capture": capture,
    }
    validate_run_record(record, scenario)
    return record


def assemble_from_files(
    scenario_path: str | Path,
    observation_path: str | Path,
    manual_evidence_path: str | Path,
) -> dict[str, Any]:
    scenario = load_scenario(scenario_path)
    observation = load_json_object(observation_path)
    manual_evidence = load_json_object(manual_evidence_path)
    return assemble_run_record(observation, manual_evidence, scenario)


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Assemble the Everward Unreal rendering run record")
    parser.add_argument("--scenario", required=True)
    parser.add_argument("--observation", required=True)
    parser.add_argument("--manual-evidence", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args(argv)

    record = assemble_from_files(args.scenario, args.observation, args.manual_evidence)
    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(record, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
