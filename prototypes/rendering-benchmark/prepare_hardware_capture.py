"""Prepare a complete, evidence-preserving workspace for Unreal hardware capture."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any

from capture_readiness import audit_capture_readiness
from export_handoff import build_handoff_bundle
from prepare_capture import prepare_capture_files
from scenario import load_scenario
from scene_state import load_manifest


def prepare_hardware_capture_workspace(
    root: Path,
    output_dir: Path,
    *,
    overwrite: bool = False,
) -> dict[str, Any]:
    root = Path(root)
    output_dir = Path(output_dir)
    scenario_path = root / "scenario.json"
    manifest_path = root / "scene_manifest.json"

    scenario = load_scenario(scenario_path)
    manifest = load_manifest(manifest_path)
    output_dir.mkdir(parents=True, exist_ok=True)

    handoff_path = output_dir / "handoff.json"
    if handoff_path.exists() and not overwrite:
        raise FileExistsError(f"refusing to overwrite existing handoff: {handoff_path}")

    handoff = build_handoff_bundle(scenario, manifest)
    handoff_path.write_text(json.dumps(handoff, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    records = prepare_capture_files(scenario_path, output_dir, overwrite=overwrite)
    readiness = audit_capture_readiness(root, handoff_path)

    summary = {
        "scenario_name": scenario["name"],
        "scenario_version": scenario["scenario_version"],
        "engine": "unreal",
        "handoff": str(handoff_path),
        "evidence_scaffold": str(records["unreal"]),
        "ready_for_hardware_capture": readiness["ready_for_hardware_capture"],
        "blockers": readiness["blockers"],
        "manual_evidence_after_capture": readiness["manual_evidence_after_capture"]["unreal"],
        "next_step": (
            "Run the canonical Unreal benchmark scene and fill only real measured evidence."
            if readiness["ready_for_hardware_capture"]
            else "Resolve every readiness blocker before running the Unreal benchmark."
        ),
    }
    summary_path = output_dir / "capture-preparation-summary.json"
    summary_path.write_text(json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    summary["summary"] = str(summary_path)
    return summary


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).parent)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--overwrite", action="store_true")
    args = parser.parse_args(argv)

    summary = prepare_hardware_capture_workspace(args.root, args.output_dir, overwrite=args.overwrite)
    print(json.dumps(summary, indent=2, sort_keys=True))
    return 0 if summary["ready_for_hardware_capture"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
