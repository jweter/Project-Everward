from __future__ import annotations

import json
import sys
from pathlib import Path


REQUIRED_CHECKS = (
    "unreal_cpp_build",
    "pie_launch",
    "probe_spawn",
    "test_environment_visible",
    "camera_orbit_zoom",
    "hud_system_discovery",
    "movement_x",
    "movement_y",
    "movement_z",
    "stop_command",
    "power_allocation",
    "scan_complete",
    "scan_cancel",
    "policy_install_clear",
    "policy_compute_gate",
    "manual_automation_shared_state",
)

CHECK_STATUSES = {"pass", "fail", "not_tested"}
OVERALL_RESULTS = {"pass", "fail", "partial", "not_tested"}
RATING_FIELDS = (
    "embodiment",
    "hud_clarity",
    "control_discoverability",
    "generation1_clunkiness",
    "movement_readability",
    "automation_comprehension",
    "desire_to_continue",
)


def validate_observation(data: dict) -> list[str]:
    errors: list[str] = []

    if data.get("observation_version") != 1:
        errors.append("observation_version must equal 1")
    if data.get("test_id") != "phase2-first-run":
        errors.append("test_id must equal 'phase2-first-run'")
    if not isinstance(data.get("git_commit"), str) or not data.get("git_commit"):
        errors.append("git_commit must be a non-empty string")
    if data.get("unreal_engine_version") != "5.8":
        errors.append("unreal_engine_version must equal '5.8' for this protocol")

    overall = data.get("overall_result")
    if overall not in OVERALL_RESULTS:
        errors.append(f"overall_result must be one of {sorted(OVERALL_RESULTS)}")

    checks = data.get("checks")
    if not isinstance(checks, dict):
        errors.append("checks must be an object")
        checks = {}

    for check_id in REQUIRED_CHECKS:
        check = checks.get(check_id)
        if not isinstance(check, dict):
            errors.append(f"missing required check: {check_id}")
            continue
        status = check.get("status")
        if status not in CHECK_STATUSES:
            errors.append(f"{check_id}.status must be one of {sorted(CHECK_STATUSES)}")
        if not isinstance(check.get("notes", ""), str):
            errors.append(f"{check_id}.notes must be a string")

    statuses = [checks.get(check_id, {}).get("status") for check_id in REQUIRED_CHECKS]
    expected_overall = "pass"
    if any(status == "fail" for status in statuses):
        expected_overall = "fail"
    elif any(status == "not_tested" for status in statuses):
        expected_overall = "partial"

    if overall not in {"not_tested", expected_overall}:
        errors.append(
            f"overall_result is {overall!r}, but required check statuses imply {expected_overall!r}"
        )
    if overall == "not_tested" and any(status != "not_tested" for status in statuses):
        errors.append("overall_result may be 'not_tested' only when every required check is not_tested")

    ratings = data.get("subjective_ratings")
    if not isinstance(ratings, dict):
        errors.append("subjective_ratings must be an object")
        ratings = {}
    for field in RATING_FIELDS:
        value = ratings.get(field)
        if value is not None and (not isinstance(value, int) or isinstance(value, bool) or not 1 <= value <= 5):
            errors.append(f"subjective_ratings.{field} must be null or an integer from 1 to 5")

    for field in ("warnings", "blockers", "evidence"):
        if not isinstance(data.get(field), list):
            errors.append(f"{field} must be a list")

    if not isinstance(data.get("playtest_notes", ""), str):
        errors.append("playtest_notes must be a string")

    return errors


def main(argv: list[str]) -> int:
    if len(argv) != 2:
        print("usage: python tools/validate_phase2_first_run_observation.py <observation.json>")
        return 2

    path = Path(argv[1])
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        print(f"invalid observation: {exc}")
        return 1

    errors = validate_observation(data)
    if errors:
        for error in errors:
            print(f"ERROR: {error}")
        return 1

    print(f"valid Phase 2 first-run observation: {path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
