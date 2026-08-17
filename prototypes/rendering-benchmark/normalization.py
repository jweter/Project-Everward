"""Deterministic normalization for Everward rendering benchmark evidence.

Raw measurements are normalized pairwise for dimensions with objective capture
fields. Qualitative dimensions remain explicit reviewer judgments, but every
score must carry non-empty evidence so the final engine decision is auditable.
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Any, Mapping

from benchmark import BenchmarkResult, REQUIRED_METRICS
from run_record import validate_run_record


# Objective metrics derived directly from canonical run-record capture fields.
# Lower values are better for every source below.
QUANTITATIVE_SOURCES = {
    "cpu_frame_time": "cpu_frame_time_ms",
    "gpu_frame_time": "gpu_frame_time_ms",
    "memory_use": "peak_memory_mib",
    "build_distribution_complexity": "build_size_mib",
    "developer_iteration_speed": "implementation_hours",
}

QUALITATIVE_METRICS = tuple(
    metric for metric in REQUIRED_METRICS if metric not in QUANTITATIVE_SOURCES
)


@dataclass(frozen=True)
class QualitativeAssessment:
    """One explicit normalized judgment with traceable evidence."""

    score: float
    evidence: str


def _validate_assessment(metric: str, assessment: QualitativeAssessment) -> None:
    if isinstance(assessment.score, bool) or not isinstance(assessment.score, (int, float)):
        raise TypeError(f"qualitative metric {metric!r} score must be numeric")
    if not 0.0 <= float(assessment.score) <= 10.0:
        raise ValueError(f"qualitative metric {metric!r} score must be normalized to 0..10")
    if not isinstance(assessment.evidence, str) or not assessment.evidence.strip():
        raise ValueError(f"qualitative metric {metric!r} requires non-empty evidence")


def validate_qualitative_assessments(
    assessments: Mapping[str, QualitativeAssessment],
) -> None:
    """Require exactly the qualitative dimensions left by the raw-data contract."""
    expected = set(QUALITATIVE_METRICS)
    actual = set(assessments)
    missing = sorted(expected - actual)
    unexpected = sorted(actual - expected)
    if missing:
        raise ValueError(f"missing qualitative metrics: {', '.join(missing)}")
    if unexpected:
        raise ValueError(f"unknown qualitative metrics: {', '.join(unexpected)}")
    for metric in QUALITATIVE_METRICS:
        _validate_assessment(metric, assessments[metric])


def normalize_lower_is_better(value: float, peer_value: float) -> float:
    """Normalize one lower-is-better measurement against the peer result.

    The lower value receives 10.0. The higher value receives a proportional
    score of 10 * lower / higher. Equal values tie at 10.0. A zero measurement
    is only accepted when both values are zero; otherwise the non-zero side gets
    0.0 and the zero side gets 10.0.
    """
    for candidate in (value, peer_value):
        if isinstance(candidate, bool) or not isinstance(candidate, (int, float)):
            raise TypeError("normalization values must be numeric")
        if float(candidate) < 0:
            raise ValueError("normalization values cannot be negative")

    value = float(value)
    peer_value = float(peer_value)
    if value == peer_value:
        return 10.0
    if value == 0.0:
        return 10.0
    if peer_value == 0.0:
        return 0.0

    best = min(value, peer_value)
    if value == best:
        return 10.0
    return round(10.0 * best / value, 6)


def normalize_pair(
    scenario: Mapping[str, Any],
    left_record: Mapping[str, Any],
    right_record: Mapping[str, Any],
    left_qualitative: Mapping[str, QualitativeAssessment],
    right_qualitative: Mapping[str, QualitativeAssessment],
) -> tuple[BenchmarkResult, BenchmarkResult, dict[str, object]]:
    """Convert two validated raw runs into complete comparable benchmark results.

    This function never invents qualitative scores. It derives only objective
    dimensions from capture data and requires explicit evidence-bearing inputs
    for every remaining dimension.
    """
    validate_run_record(left_record, scenario)
    validate_run_record(right_record, scenario)
    if left_record["engine"] == right_record["engine"]:
        raise ValueError("benchmark comparison requires two different engines")

    validate_qualitative_assessments(left_qualitative)
    validate_qualitative_assessments(right_qualitative)

    left_metrics: dict[str, float] = {}
    right_metrics: dict[str, float] = {}
    quantitative_audit: dict[str, dict[str, object]] = {}

    for metric, capture_field in QUANTITATIVE_SOURCES.items():
        left_raw = float(left_record["capture"][capture_field])
        right_raw = float(right_record["capture"][capture_field])
        left_score = normalize_lower_is_better(left_raw, right_raw)
        right_score = normalize_lower_is_better(right_raw, left_raw)
        left_metrics[metric] = left_score
        right_metrics[metric] = right_score
        quantitative_audit[metric] = {
            "capture_field": capture_field,
            "left_raw": left_raw,
            "right_raw": right_raw,
            "left_score": left_score,
            "right_score": right_score,
        }

    qualitative_audit: dict[str, dict[str, object]] = {}
    for metric in QUALITATIVE_METRICS:
        left_assessment = left_qualitative[metric]
        right_assessment = right_qualitative[metric]
        left_metrics[metric] = float(left_assessment.score)
        right_metrics[metric] = float(right_assessment.score)
        qualitative_audit[metric] = {
            "left_score": float(left_assessment.score),
            "left_evidence": left_assessment.evidence.strip(),
            "right_score": float(right_assessment.score),
            "right_evidence": right_assessment.evidence.strip(),
        }

    audit = {
        "scenario_version": scenario["scenario_version"],
        "scenario_name": scenario["name"],
        "left_engine": left_record["engine"],
        "right_engine": right_record["engine"],
        "quantitative": quantitative_audit,
        "qualitative": qualitative_audit,
    }

    return (
        BenchmarkResult(str(left_record["engine"]), left_metrics),
        BenchmarkResult(str(right_record["engine"]), right_metrics),
        audit,
    )
