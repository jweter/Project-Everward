"""Deterministic playback helpers for the canonical rendering benchmark.

Both engine candidates should derive camera-stage transitions and accelerated
simulation time from the same scenario contract instead of duplicating timing
constants inside Godot or Unreal project code.
"""

from __future__ import annotations

from typing import Any, Mapping


def playback_state(scenario: Mapping[str, Any], elapsed_real_seconds: float) -> dict[str, Any]:
    """Return the canonical benchmark state at a real-time offset.

    The helper is renderer-neutral. It defines only benchmark timing truth;
    engines remain responsible for how the camera, HUD, machinery, and effects
    visually represent that state.
    """
    duration = float(scenario["duration_seconds"])
    if elapsed_real_seconds < 0 or elapsed_real_seconds > duration:
        raise ValueError("elapsed_real_seconds must be within the benchmark duration")

    camera_sequence = scenario["camera_sequence"]
    stage_durations = scenario["camera_stage_durations_seconds"]

    elapsed_before_stage = 0.0
    stage_index = len(camera_sequence) - 1
    stage_elapsed = float(stage_durations[-1])

    for index, stage_duration in enumerate(stage_durations):
        stage_end = elapsed_before_stage + float(stage_duration)
        if elapsed_real_seconds < stage_end or index == len(stage_durations) - 1:
            stage_index = index
            stage_elapsed = elapsed_real_seconds - elapsed_before_stage
            break
        elapsed_before_stage = stage_end

    stage_duration = float(stage_durations[stage_index])
    stage_progress = min(1.0, max(0.0, stage_elapsed / stage_duration))

    return {
        "scenario_name": scenario["name"],
        "scenario_version": scenario["scenario_version"],
        "elapsed_real_seconds": elapsed_real_seconds,
        "elapsed_simulation_seconds": elapsed_real_seconds
        * scenario["simulation_seconds_per_real_second"],
        "camera_stage_index": stage_index,
        "camera_stage": camera_sequence[stage_index],
        "camera_stage_progress": stage_progress,
    }
