"""Audit whether Everward may leave Phase 1 and begin One Probe.

This gate is intentionally conservative. Repository structure can prove that the
technical prototypes exist, but production gameplay remains blocked until a
real rendering benchmark decision artifact exists, is decision-ready, and
validates Unreal Engine as the accepted production direction.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any


PROTOTYPE_REQUIREMENTS = {
    "simulation_clock": "simulation-clock",
    "procedural_system": "procedural-system",
    "coordinate_scale": "coordinate-scale",
    "headless_simulation": "headless-simulation",
    "rendering_benchmark": "rendering-benchmark",
}

REQUIRED_PRODUCTION_ENGINE = "unreal"


def _load_json(path: Path) -> dict[str, Any]:
    data = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(data, dict):
        raise ValueError(f"{path} must contain a JSON object")
    return data


def audit_phase1_exit(prototypes_root: Path, decision_artifact: Path | None = None) -> dict[str, Any]:
    prototypes_root = Path(prototypes_root)
    blockers: list[str] = []
    prototypes: dict[str, dict[str, Any]] = {}

    for name, relative in PROTOTYPE_REQUIREMENTS.items():
        path = prototypes_root / relative
        present = path.is_dir() and any(path.iterdir())
        prototypes[name] = {"path": str(path), "present": present}
        if not present:
            blockers.append(f"missing or empty Phase 1 prototype: {relative}")

    decision_status: dict[str, Any] = {
        "present": False,
        "decision_ready": False,
        "required_production_engine": REQUIRED_PRODUCTION_ENGINE,
    }
    if decision_artifact is None:
        blockers.append("real rendering benchmark decision artifact not supplied")
    else:
        decision_artifact = Path(decision_artifact)
        decision_status["path"] = str(decision_artifact)
        decision_status["present"] = decision_artifact.is_file()
        if not decision_artifact.is_file():
            blockers.append("rendering benchmark decision artifact does not exist")
        else:
            try:
                artifact = _load_json(decision_artifact)
                if artifact.get("artifact_type") != "everward_phase1_engine_decision":
                    blockers.append("invalid Phase 1 engine decision artifact type")
                packet = artifact.get("decision_packet")
                if not isinstance(packet, dict):
                    blockers.append("engine decision artifact is missing decision_packet")
                else:
                    status = packet.get("status")
                    recommendation = packet.get("recommendation")
                    decision_status["status"] = status
                    decision_status["recommendation"] = recommendation
                    if status != "decision_ready":
                        blockers.append("engine benchmark is not decision-ready")
                    elif recommendation != REQUIRED_PRODUCTION_ENGINE:
                        blockers.append(
                            "engine benchmark does not validate the accepted Unreal production direction; "
                            "resolve the Unreal blocker or explicitly reconsider ADR-0001 before Phase 2"
                        )
                    else:
                        decision_status["decision_ready"] = True
            except (OSError, json.JSONDecodeError, ValueError) as exc:
                blockers.append(f"invalid rendering benchmark decision artifact: {exc}")

    return {
        "phase1_complete": not blockers,
        "phase2_one_probe_authorized": not blockers,
        "blockers": blockers,
        "prototypes": prototypes,
        "engine_decision": decision_status,
        "next_step": (
            "Begin Phase 2 — One Probe in Unreal Engine."
            if not blockers
            else "Resolve all Phase 1 blockers; do not begin production gameplay architecture yet."
        ),
    }


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--prototypes-root", type=Path, default=Path(__file__).parent)
    parser.add_argument("--engine-decision", type=Path)
    args = parser.parse_args(argv)
    report = audit_phase1_exit(args.prototypes_root, args.engine_decision)
    print(json.dumps(report, indent=2, sort_keys=True))
    return 0 if report["phase1_complete"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
