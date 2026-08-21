"""Prepare the canonical Unreal Phase 1 rendering evidence scaffold."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any

from evidence_template import create_evidence_template
from scenario import load_scenario

PRODUCTION_ENGINE = "unreal"


def prepare_capture_files(scenario_path: Path, output_dir: Path, *, overwrite: bool = False) -> dict[str, Path]:
    """Create the scenario-bound Unreal evidence template without fabricating data."""
    scenario: dict[str, Any] = load_scenario(scenario_path)
    output_dir.mkdir(parents=True, exist_ok=True)

    destination = output_dir / f"{PRODUCTION_ENGINE}-run-record.json"
    if destination.exists() and not overwrite:
        raise FileExistsError(f"refusing to overwrite existing evidence file: {destination}")

    payload = create_evidence_template(PRODUCTION_ENGINE, scenario)
    destination.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    return {PRODUCTION_ENGINE: destination}


def build_summary(scenario_path: Path, written: dict[str, Path]) -> dict[str, Any]:
    scenario = load_scenario(scenario_path)
    return {
        "scenario_name": scenario["name"],
        "scenario_version": scenario["scenario_version"],
        "target_resolution": scenario["target_resolution"],
        "target_fps": scenario["target_fps"],
        "duration_seconds": scenario["duration_seconds"],
        "engine": PRODUCTION_ENGINE,
        "evidence": str(written[PRODUCTION_ENGINE]),
        "next_step": "Run the canonical Unreal scene and replace template blanks only with measured evidence.",
    }


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--scenario", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--overwrite", action="store_true")
    args = parser.parse_args(argv)

    written = prepare_capture_files(args.scenario, args.output_dir, overwrite=args.overwrite)
    print(json.dumps(build_summary(args.scenario, written), indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
