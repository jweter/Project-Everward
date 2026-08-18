"""Export canonical renderer-neutral benchmark state for engine implementations.

Godot and Unreal should consume this bundle as input evidence rather than
independently recreating benchmark truth. The bundle intentionally contains
only deterministic scenario/scene state and no engine-specific presentation.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any, Iterable, Mapping

from scenario import load_scenario
from scene_state import load_manifest, scene_state


DEFAULT_SAMPLE_TIMES = (0.0, 20.0, 39.999, 40.0, 60.0, 79.999, 80.0, 100.0, 119.999)


def build_handoff_bundle(
    scenario: Mapping[str, Any],
    manifest: Mapping[str, Any],
    sample_times: Iterable[float] = DEFAULT_SAMPLE_TIMES,
) -> dict[str, Any]:
    duration = float(scenario["duration_seconds"])
    times = tuple(float(value) for value in sample_times)
    if not times:
        raise ValueError("sample_times cannot be empty")
    if any(value < 0 or value >= duration for value in times):
        raise ValueError("sample times must be within benchmark duration")
    if tuple(sorted(times)) != times:
        raise ValueError("sample times must be monotonically increasing")
    if len(set(times)) != len(times):
        raise ValueError("sample times must be unique")

    frames = [scene_state(scenario, manifest, value) for value in times]
    return {
        "handoff_version": 2,
        "scenario_name": scenario["name"],
        "scenario_version": scenario["scenario_version"],
        "duration_seconds": scenario["duration_seconds"],
        "target_resolution": scenario["target_resolution"],
        "target_fps": scenario["target_fps"],
        "simulation_seconds_per_real_second": scenario["simulation_seconds_per_real_second"],
        "camera_sequence": scenario["camera_sequence"],
        "camera_stage_durations_seconds": scenario["camera_stage_durations_seconds"],
        "required_scene_features": scenario["required_scene_features"],
        "objects": manifest["objects"],
        "cameras": manifest["cameras"],
        "animation_periods_seconds": {
            "mining_mechanism": manifest["animation"]["mining_mechanism_period_seconds"],
            "debris_pulse": manifest["animation"]["debris_pulse_period_seconds"],
            "asteroid_rotation": manifest["animation"]["asteroid_rotation_period_seconds"],
        },
        "feature_bindings": manifest["feature_bindings"],
        "sample_times_seconds": list(times),
        "frames": frames,
    }


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Export the canonical Everward rendering benchmark handoff bundle")
    parser.add_argument("--scenario", default=str(Path(__file__).parent / "scenario.json"))
    parser.add_argument("--manifest", default=str(Path(__file__).parent / "scene_manifest.json"))
    parser.add_argument("--output", required=True)
    args = parser.parse_args(argv)

    scenario = load_scenario(args.scenario)
    manifest = load_manifest(args.manifest)
    bundle = build_handoff_bundle(scenario, manifest)
    Path(args.output).write_text(json.dumps(bundle, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
