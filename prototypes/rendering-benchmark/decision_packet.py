"""Build an auditable Phase 1 engine-decision packet from benchmark evidence.

The packet composes the existing raw-record validation, deterministic
normalization, and weighted comparison contracts. It does not fabricate
measurements and it does not force an engine choice when the weighted result is
too close to call.
"""

from __future__ import annotations

from typing import Any, Mapping

from benchmark import DEFAULT_WEIGHTS, REQUIRED_METRICS, compare_results
from normalization import QualitativeAssessment, normalize_pair


def _metric_deltas(left_metrics: Mapping[str, float], right_metrics: Mapping[str, float]) -> list[dict[str, object]]:
    rows: list[dict[str, object]] = []
    for metric in REQUIRED_METRICS:
        left_score = float(left_metrics[metric])
        right_score = float(right_metrics[metric])
        weighted_delta = (left_score - right_score) * float(DEFAULT_WEIGHTS[metric])
        rows.append(
            {
                "metric": metric,
                "left_score": round(left_score, 3),
                "right_score": round(right_score, 3),
                "weight": float(DEFAULT_WEIGHTS[metric]),
                "weighted_delta": round(weighted_delta, 4),
                "absolute_weighted_delta": round(abs(weighted_delta), 4),
            }
        )
    rows.sort(key=lambda row: (-float(row["absolute_weighted_delta"]), str(row["metric"])))
    return rows


def build_decision_packet(
    scenario: Mapping[str, Any],
    left_record: Mapping[str, Any],
    right_record: Mapping[str, Any],
    left_qualitative: Mapping[str, QualitativeAssessment],
    right_qualitative: Mapping[str, QualitativeAssessment],
) -> dict[str, object]:
    """Return one traceable decision packet from complete benchmark evidence.

    A decisive weighted result is marked ``decision_ready``. A near-tie is
    marked ``additional_evidence_required`` so Phase 1 cannot be closed by an
    arbitrary preference.
    """
    left_result, right_result, audit = normalize_pair(
        scenario,
        left_record,
        right_record,
        left_qualitative,
        right_qualitative,
    )
    comparison = compare_results(left_result, right_result)
    deltas = _metric_deltas(left_result.metrics, right_result.metrics)

    if comparison["outcome"] == "too_close_to_call":
        status = "additional_evidence_required"
        recommendation = None
        rationale = (
            "Weighted results are within the benchmark tie threshold; gather "
            "additional representative evidence before choosing the production engine."
        )
    else:
        status = "decision_ready"
        recommendation = comparison["outcome"]
        rationale = (
            f"{recommendation} leads the weighted benchmark by "
            f"{comparison['score_delta']} points under the declared Phase 1 contract."
        )

    return {
        "packet_version": 1,
        "scenario_version": scenario["scenario_version"],
        "scenario_name": scenario["name"],
        "status": status,
        "recommendation": recommendation,
        "rationale": rationale,
        "comparison": comparison,
        "top_differentiators": deltas[:5],
        "all_metric_deltas": deltas,
        "normalization_audit": audit,
        "decision_log_fields": {
            "question": "Which production engine should Everward use after Phase 1?",
            "candidates": [left_result.engine, right_result.engine],
            "recommended_engine": recommendation,
            "benchmark_outcome": comparison["outcome"],
            "score_delta": comparison["score_delta"],
            "status": status,
        },
    }
