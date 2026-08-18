"""End-to-end CLI for producing an auditable rendering engine decision packet.

The pipeline intentionally performs no measurement and invents no qualitative
judgment. It loads the canonical scenario, validates both raw engine run
records, requires explicit evidence-bearing qualitative assessments, and emits
the same decision packet used by the Phase 1 engine-selection contract.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any, Mapping, Sequence

from decision_packet import build_decision_packet
from normalization import QUALITATIVE_METRICS, QualitativeAssessment
from run_record import load_run_record
from scenario import load_scenario


def load_qualitative_assessments(path: str | Path) -> dict[str, QualitativeAssessment]:
    """Load one engine's qualitative evidence from JSON without filling gaps."""
    with Path(path).open("r", encoding="utf-8") as handle:
        data = json.load(handle)

    if not isinstance(data, Mapping):
        raise TypeError("qualitative assessment document must be an object")

    expected = set(QUALITATIVE_METRICS)
    actual = set(data)
    missing = sorted(expected - actual)
    unexpected = sorted(actual - expected)
    if missing:
        raise ValueError(f"missing qualitative metrics: {', '.join(missing)}")
    if unexpected:
        raise ValueError(f"unknown qualitative metrics: {', '.join(unexpected)}")

    assessments: dict[str, QualitativeAssessment] = {}
    for metric in QUALITATIVE_METRICS:
        raw = data[metric]
        if not isinstance(raw, Mapping):
            raise TypeError(f"qualitative metric {metric!r} must be an object")
        if set(raw) != {"score", "evidence"}:
            raise ValueError(
                f"qualitative metric {metric!r} must contain exactly score and evidence"
            )
        assessments[metric] = QualitativeAssessment(
            score=raw["score"],
            evidence=raw["evidence"],
        )

    return assessments


def build_packet_from_files(
    scenario_path: str | Path,
    left_record_path: str | Path,
    right_record_path: str | Path,
    left_qualitative_path: str | Path,
    right_qualitative_path: str | Path,
) -> dict[str, object]:
    """Build the Phase 1 decision packet from five explicit evidence files."""
    scenario = load_scenario(scenario_path)
    left_record = load_run_record(left_record_path, scenario_path)
    right_record = load_run_record(right_record_path, scenario_path)
    left_qualitative = load_qualitative_assessments(left_qualitative_path)
    right_qualitative = load_qualitative_assessments(right_qualitative_path)
    return build_decision_packet(
        scenario,
        left_record,
        right_record,
        left_qualitative,
        right_qualitative,
    )


def _parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Build Everward's auditable Phase 1 rendering engine decision packet."
    )
    parser.add_argument("--scenario", required=True, help="canonical scenario JSON")
    parser.add_argument("--left-record", required=True, help="first engine raw run record JSON")
    parser.add_argument("--right-record", required=True, help="second engine raw run record JSON")
    parser.add_argument(
        "--left-qualitative",
        required=True,
        help="first engine qualitative evidence JSON",
    )
    parser.add_argument(
        "--right-qualitative",
        required=True,
        help="second engine qualitative evidence JSON",
    )
    parser.add_argument(
        "--output",
        help="optional output path; defaults to stdout",
    )
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    args = _parser().parse_args(argv)
    packet = build_packet_from_files(
        args.scenario,
        args.left_record,
        args.right_record,
        args.left_qualitative,
        args.right_qualitative,
    )
    rendered = json.dumps(packet, indent=2, sort_keys=True) + "\n"
    if args.output:
        Path(args.output).write_text(rendered, encoding="utf-8")
    else:
        print(rendered, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
