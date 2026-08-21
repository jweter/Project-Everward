"""Finalize Phase 1 Unreal validation evidence into one auditable artifact.

Everward's production renderer is Unreal Engine. This command does not compare
renderers or reopen the engine decision. It validates a complete, scenario-bound
Unreal run record and emits the decision-ready artifact consumed by the Phase 1
exit gate.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any

from run_record import load_run_record
from scenario import load_scenario

PRODUCTION_ENGINE = "unreal"


def finalize(scenario_path: str | Path, unreal_record_path: str | Path) -> dict[str, Any]:
    scenario = load_scenario(scenario_path)
    record = load_run_record(unreal_record_path, scenario_path)
    if record["engine"] != PRODUCTION_ENGINE:
        raise ValueError("Phase 1 production validation requires an Unreal run record")

    packet = {
        "status": "decision_ready",
        "recommendation": PRODUCTION_ENGINE,
        "decision_basis": "accepted Unreal production direction validated by complete measured runtime evidence",
        "scenario_name": scenario["name"],
        "scenario_version": scenario["scenario_version"],
        "engine_version": record["capture"]["engine_version"],
        "captured_at_utc": record["captured_at_utc"],
    }
    return {
        "artifact_version": 2,
        "artifact_type": "everward_phase1_engine_decision",
        "inputs": {
            "scenario": Path(scenario_path).as_posix(),
            "unreal_run_record": Path(unreal_record_path).as_posix(),
        },
        "decision_packet": packet,
    }


def main() -> None:
    parser = argparse.ArgumentParser(description="Finalize Everward Phase 1 Unreal validation evidence")
    parser.add_argument("--scenario", required=True)
    parser.add_argument("--unreal-record", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()

    artifact = finalize(args.scenario, args.unreal_record)
    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(artifact, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"wrote {output}: {artifact['decision_packet']['status']}")


if __name__ == "__main__":
    main()
