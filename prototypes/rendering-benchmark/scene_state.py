"""Renderer-neutral canonical scene state for the Phase 1 rendering benchmark.

The simulation/benchmark layer owns scene truth. Godot and Unreal should consume
this state and represent it visually rather than independently inventing object
placement, animation phase, camera timing, or HUD telemetry.
"""

from __future__ import annotations

import json
from pathlib import Path
from typing import Any, Mapping

from playback import playback_state


ROOT = Path(__file__).parent


def load_manifest(path: str | Path = ROOT / "scene_manifest.json") -> dict[str, Any]:
    manifest = json.loads(Path(path).read_text(encoding="utf-8"))
    validate_manifest(manifest)
    return manifest


def validate_manifest(manifest: Mapping[str, Any], scenario: Mapping[str, Any] | None = None) -> None:
    if manifest.get("manifest_version") != 1:
        raise ValueError("unsupported scene manifest version")

    objects = manifest.get("objects")
    cameras = manifest.get("cameras")
    animation = manifest.get("animation")
    bindings = manifest.get("feature_bindings")
    if not isinstance(objects, Mapping) or not objects:
        raise ValueError("scene manifest requires objects")
    if not isinstance(cameras, Mapping) or not cameras:
        raise ValueError("scene manifest requires cameras")
    if not isinstance(animation, Mapping) or not animation:
        raise ValueError("scene manifest requires animation")
    if not isinstance(bindings, Mapping):
        raise ValueError("scene manifest requires feature bindings")

    for key in ("mining_mechanism_period_seconds", "debris_pulse_period_seconds", "asteroid_rotation_period_seconds"):
        value = animation.get(key)
        if not isinstance(value, (int, float)) or value <= 0:
            raise ValueError(f"animation.{key} must be positive")

    if scenario is not None:
        if manifest.get("scenario_name") != scenario.get("name"):
            raise ValueError("scene manifest scenario_name does not match scenario")
        if manifest.get("scenario_version") != scenario.get("scenario_version"):
            raise ValueError("scene manifest scenario_version does not match scenario")
        if set(cameras) != set(scenario.get("camera_sequence", [])):
            raise ValueError("scene manifest cameras must exactly match scenario camera sequence")
        required_features = set(scenario.get("required_scene_features", []))
        if set(bindings) != required_features:
            raise ValueError("scene manifest feature bindings must exactly match required scene features")


def _phase(elapsed_seconds: float, period_seconds: float) -> float:
    return (elapsed_seconds % period_seconds) / period_seconds


def scene_state(
    scenario: Mapping[str, Any],
    manifest: Mapping[str, Any],
    elapsed_real_seconds: float,
) -> dict[str, Any]:
    """Return deterministic scene truth at one point in benchmark playback."""
    validate_manifest(manifest, scenario)
    playback = playback_state(scenario, elapsed_real_seconds)
    animation = manifest["animation"]
    camera = manifest["cameras"][playback["camera_stage"]]

    mining_phase = _phase(elapsed_real_seconds, float(animation["mining_mechanism_period_seconds"]))
    debris_phase = _phase(elapsed_real_seconds, float(animation["debris_pulse_period_seconds"]))
    asteroid_rotation_phase = _phase(elapsed_real_seconds, float(animation["asteroid_rotation_period_seconds"]))

    return {
        "scenario_name": playback["scenario_name"],
        "scenario_version": playback["scenario_version"],
        "elapsed_real_seconds": playback["elapsed_real_seconds"],
        "elapsed_simulation_seconds": playback["elapsed_simulation_seconds"],
        "camera": {
            "stage": playback["camera_stage"],
            "stage_index": playback["camera_stage_index"],
            "stage_progress": playback["camera_stage_progress"],
            **camera,
        },
        "objects": manifest["objects"],
        "animation": {
            "mining_mechanism_phase": mining_phase,
            "debris_pulse_phase": debris_phase,
            "asteroid_rotation_phase": asteroid_rotation_phase,
        },
        "hud": {
            "simulation_days_elapsed": playback["elapsed_simulation_seconds"] / 86400.0,
            "camera_stage": playback["camera_stage"],
            "time_acceleration": scenario["simulation_seconds_per_real_second"],
            "benchmark_elapsed_seconds": elapsed_real_seconds,
        },
    }
