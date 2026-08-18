"""Audit whether Prototype C is structurally ready for real hardware capture.

This module intentionally evaluates only repository and handoff readiness. It does
not fabricate benchmark measurements or claim that either engine has been run.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any


ENGINE_REQUIRED_FILES = {
    "godot": (
        "godot/project.godot",
        "godot/main.tscn",
        "godot/benchmark_adapter.gd",
        "godot/capture_session.gd",
    ),
    "unreal": (
        "unreal/EverwardBenchmark.uproject",
        "unreal/Source/EverwardBenchmark/EverwardBenchmark.Build.cs",
        "unreal/Source/EverwardBenchmark/BenchmarkAdapter.h",
        "unreal/Source/EverwardBenchmark/BenchmarkAdapter.cpp",
        "unreal/Source/EverwardBenchmark/BenchmarkCaptureSession.h",
        "unreal/Source/EverwardBenchmark/BenchmarkCaptureSession.cpp",
    ),
}

PIPELINE_REQUIRED_FILES = (
    "scenario.json",
    "scene_manifest.json",
    "export_handoff.py",
    "prepare_capture.py",
    "assemble_run_record.py",
    "pipeline.py",
)

EXPECTED_HANDOFF_VERSION = 2


def _load_json(path: Path) -> dict[str, Any]:
    data = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(data, dict):
        raise ValueError(f"{path} must contain a JSON object")
    return data


def audit_capture_readiness(root: Path, handoff_path: Path | None = None) -> dict[str, Any]:
    """Return an evidence-preserving readiness report for the rendering capture."""
    root = Path(root)
    handoff_path = Path(handoff_path) if handoff_path else root / "handoff.json"

    blockers: list[str] = []
    warnings: list[str] = []

    for relative in PIPELINE_REQUIRED_FILES:
        if not (root / relative).is_file():
            blockers.append(f"missing pipeline file: {relative}")

    engine_status: dict[str, Any] = {}
    for engine, required in ENGINE_REQUIRED_FILES.items():
        missing = [relative for relative in required if not (root / relative).is_file()]
        engine_status[engine] = {
            "source_ready": not missing,
            "missing_files": missing,
        }
        blockers.extend(f"{engine}: missing source file: {relative}" for relative in missing)

    scenario = None
    scenario_path = root / "scenario.json"
    if scenario_path.is_file():
        try:
            scenario = _load_json(scenario_path)
        except (OSError, json.JSONDecodeError, ValueError) as exc:
            blockers.append(f"invalid scenario.json: {exc}")

    handoff_status: dict[str, Any] = {
        "path": str(handoff_path),
        "present": handoff_path.is_file(),
        "valid_identity": False,
    }
    if not handoff_path.is_file():
        blockers.append("handoff.json has not been generated; run export_handoff.py before capture")
    else:
        try:
            handoff = _load_json(handoff_path)
            handoff_status["handoff_version"] = handoff.get("handoff_version")
            handoff_status["scenario_name"] = handoff.get("scenario_name")
            handoff_status["scenario_version"] = handoff.get("scenario_version")
            if handoff.get("handoff_version") != EXPECTED_HANDOFF_VERSION:
                blockers.append(
                    f"handoff version must be {EXPECTED_HANDOFF_VERSION}, got {handoff.get('handoff_version')!r}"
                )
            elif scenario is not None and (
                handoff.get("scenario_name") != scenario.get("name")
                or handoff.get("scenario_version") != scenario.get("scenario_version")
            ):
                blockers.append("handoff scenario identity does not match canonical scenario.json")
            else:
                handoff_status["valid_identity"] = True
        except (OSError, json.JSONDecodeError, ValueError) as exc:
            blockers.append(f"invalid handoff.json: {exc}")

    if not blockers:
        warnings.append(
            "Repository is structurally ready for hardware capture; real engine execution and measured evidence are still required."
        )

    return {
        "ready_for_hardware_capture": not blockers,
        "blockers": blockers,
        "warnings": warnings,
        "engines": engine_status,
        "handoff": handoff_status,
        "manual_evidence_after_capture": {
            "godot": [
                "gpu_frame_time_ms",
                "peak_memory_mib",
                "implementation_hours",
                "build_size_mib",
                "screenshots",
                "project_settings",
                "notes",
            ],
            "unreal": [
                "gpu_frame_time_ms",
                "implementation_hours",
                "build_size_mib",
                "screenshots",
                "project_settings",
                "notes",
            ],
        },
    }


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).parent)
    parser.add_argument("--handoff", type=Path)
    args = parser.parse_args(argv)

    report = audit_capture_readiness(args.root, args.handoff)
    print(json.dumps(report, indent=2, sort_keys=True))
    return 0 if report["ready_for_hardware_capture"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
