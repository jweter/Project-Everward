"""Evidence model and scorer for the Everward rendering benchmark.

The benchmark does not choose an engine. It validates that Godot and Unreal
results are complete, comparable, and scored from the same declared criteria.
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Mapping


REQUIRED_METRICS = (
    "visual_fidelity",
    "cpu_frame_time",
    "gpu_frame_time",
    "memory_use",
    "scene_complexity",
    "hud_effort",
    "procedural_workflow",
    "simulation_integration",
    "large_coordinate_behavior",
    "save_load_implications",
    "build_distribution_complexity",
    "developer_iteration_speed",
    "commercial_licensing",
)

# Higher score is always better after normalization. Weights intentionally favor
# the things that are hardest to retrofit later: simulation integration,
# coordinate robustness, visual target, and developer iteration speed.
DEFAULT_WEIGHTS = {
    "visual_fidelity": 0.14,
    "cpu_frame_time": 0.08,
    "gpu_frame_time": 0.08,
    "memory_use": 0.05,
    "scene_complexity": 0.04,
    "hud_effort": 0.06,
    "procedural_workflow": 0.08,
    "simulation_integration": 0.12,
    "large_coordinate_behavior": 0.10,
    "save_load_implications": 0.05,
    "build_distribution_complexity": 0.05,
    "developer_iteration_speed": 0.10,
    "commercial_licensing": 0.05,
}


@dataclass(frozen=True)
class BenchmarkResult:
    engine: str
    metrics: Mapping[str, float]
    notes: str = ""


def validate_result(result: BenchmarkResult) -> None:
    """Reject incomplete or non-normalized benchmark evidence."""
    missing = [name for name in REQUIRED_METRICS if name not in result.metrics]
    if missing:
        raise ValueError(f"missing benchmark metrics: {', '.join(missing)}")

    unexpected = [name for name in result.metrics if name not in REQUIRED_METRICS]
    if unexpected:
        raise ValueError(f"unknown benchmark metrics: {', '.join(unexpected)}")

    for name, value in result.metrics.items():
        if not isinstance(value, (int, float)):
            raise TypeError(f"metric {name!r} must be numeric")
        if not 0.0 <= float(value) <= 10.0:
            raise ValueError(f"metric {name!r} must be normalized to 0..10")


def score_result(
    result: BenchmarkResult,
    weights: Mapping[str, float] = DEFAULT_WEIGHTS,
) -> float:
    """Return a weighted 0..10 score after strict evidence validation."""
    validate_result(result)

    if set(weights) != set(REQUIRED_METRICS):
        raise ValueError("weights must cover exactly the required metrics")

    total_weight = sum(float(value) for value in weights.values())
    if abs(total_weight - 1.0) > 1e-9:
        raise ValueError("benchmark weights must sum to 1.0")
    if any(float(value) < 0 for value in weights.values()):
        raise ValueError("benchmark weights cannot be negative")

    return sum(float(result.metrics[name]) * float(weights[name]) for name in REQUIRED_METRICS)


def compare_results(left: BenchmarkResult, right: BenchmarkResult) -> dict[str, object]:
    """Compare two complete results without silently declaring a winner."""
    left_score = score_result(left)
    right_score = score_result(right)
    delta = left_score - right_score

    if abs(delta) < 0.25:
        outcome = "too_close_to_call"
    elif delta > 0:
        outcome = left.engine
    else:
        outcome = right.engine

    return {
        "left_engine": left.engine,
        "left_score": round(left_score, 3),
        "right_engine": right.engine,
        "right_score": round(right_score, 3),
        "score_delta": round(abs(delta), 3),
        "outcome": outcome,
    }
