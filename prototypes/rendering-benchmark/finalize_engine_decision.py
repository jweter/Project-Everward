"""Finalize Phase 1 engine evidence into one auditable decision artifact.

This CLI is deliberately downstream of real engine capture. It accepts two
complete run records plus explicit evidence-bearing qualitative assessments,
validates them through the existing normalization/decision contracts, and emits
one stable JSON artifact. It never invents benchmark values and never forces an
engine choice when the benchmark is too close to call.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any, Mapping

from decision_packet import build_decision_packet
from normalization import QUALITATIVE_METRICS, QualitativeAssessment
from scenario import load_scenario


def load_json(path: str | Path) -> dict[str, Any]:
    with Path(path).open("r", encoding="utf-8") as handle:
        data = json.load(handle)
    if not isinstance(data, dict):
        raise ValueError(f"{path} must contain a JSON object")
    return data


def parse_qualitative_document(data: Mapping[str, Any], expected_engine: str) -> dict[str, QualitativeAssessment]:
    if set(data) != {"assessment_version", "engine", "metrics"}:
        raise ValueError("qualitative evidence must contain exactly assessment_version, engine, and metrics")
    if data["assessment_version"] != 1:
        raise ValueError("unsupported qualitative assessment_version")
    if data["engine"] != expected_engine:
        raise ValueError(f"qualitative evidence engine {data['engine']!r} does not match run record {expected_engine!r}")
    metrics = data["metrics"]
    if not isinstance(metrics, Mapping):
        raise ValueError("qualitative metrics must be an object")
    if set(metrics) != set(QUALITATIVE_METRICS):
        missing = sorted(set(QUALITATIVE_METRICS) - set(metrics))
        extra = sorted(set(metrics) - set(QUALITATIVE_METRICS))
        details = []
        if missing:
            details.append(f"missing: {', '.join(missing)}")
        if extra:
            details.append(f"unknown: {', '.join(extra)}")
        raise ValueError("invalid qualitative metric set" + (f" ({'; '.join(details)})" if details else ""))

    parsed: dict[str, QualitativeAssessment] = {}
    for metric in QUALITATIVE_METRICS:
        row = metrics[metric]
        if not isinstance(row, Mapping) or set(row) != {"score", "evidence"}:
            raise ValueError(f"qualitative metric {metric!r} must contain exactly score and evidence")
        parsed[metric] = QualitativeAssessment(row["score"], row["evidence"])
    return parsed


def finalize(
    scenario_path: str | Path,
    left_record_path: str | Path,
    right_record_path: str | Path,
    left_qualitative_path: str | Path,
    right_qualitative_path: str | Path,
) -> dict[str, object]:
    scenario = load_scenario(scenario_path)
    left_record = load_json(left_record_path)
    right_record = load_json(right_record_path)
    left_engine = str(left_record.get("engine", ""))
    right_engine = str(right_record.get("engine", ""))
    left_qualitative = parse_qualitative_document(load_json(left_qualitative_path), left_engine)
    right_qualitative = parse_qualitative_document(load_json(right_qualitative_path), right_engine)

    packet = build_decision_packet(
        scenario,
        left_record,
        right_record,
        left_qualitative,
        right_qualitative,
    )
    return {
        "artifact_version": 1,
        "artifact_type": "everward_phase1_engine_decision",
        "inputs": {
            "scenario": str(Path(scenario_path)),
            "left_run_record": str(Path(left_record_path)),
            "right_run_record": str(Path(right_record_path)),
            "left_qualitative": str(Path(left_qualitative_path)),
            "right_qualitative": str(Path(right_qualitative_path)),
        },
        "decision_packet": packet,
    }


def main() -> None:
    parser = argparse.ArgumentParser(description="Finalize Everward Phase 1 engine benchmark evidence")
    parser.add_argument("--scenario", required=True)
    parser.add_argument("--left-record", required=True)
    parser.add_argument("--right-record", required=True)
    parser.add_argument("--left-qualitative", required=True)
    parser.add_argument("--right-qualitative", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()

    artifact = finalize(
        args.scenario,
        args.left_record,
        args.right_record,
        args.left_qualitative,
        args.right_qualitative,
    )
    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(artifact, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"wrote {output}: {artifact['decision_packet']['status']}")


if __name__ == "__main__":
    main()
