"""Fail-closed validation for a *future* agent-driven Unreal gameplay session.

This evaluates captured evidence; it never launches Unreal, uses an LLM, drives
inputs, edits the project, accesses the network, or claims visual quality.
No current session is certified merely because this validation tool exists.
"""
from __future__ import annotations

import argparse
import json
import math
import re
import sys
from pathlib import Path
from typing import Any

REPOSITORY = "jweter/Project-Everward"
SHA_PATTERN = re.compile(r"^[0-9a-f]{40}$")
PLAY_MODES = {"unreal_pie", "unreal_packaged"}
ASSERTION_SOURCES = {"unreal_functional_test", "unreal_automation_test"}
RESULTS = {"PASS", "FAIL", "REVIEW_REQUIRED", "ENVIRONMENT_FAILURE"}


def _safe_reason(reason: str) -> str:
    """Only use static, non-sensitive diagnostic text in result summaries."""
    return reason[:180]


def _load_json(path: Path) -> Any:
    with path.open("r", encoding="utf-8") as stream:
        return json.load(stream)


def _load_events(path: Path) -> list[dict[str, Any]]:
    output: list[dict[str, Any]] = []
    with path.open("r", encoding="utf-8") as stream:
        for line in stream:
            if not line.strip():
                continue
            item = json.loads(line)
            if not isinstance(item, dict):
                raise ValueError("event line is not an object")
            output.append(item)
            if len(output) > 100_000:
                raise ValueError("event count limit exceeded")
    return output


def evaluate(
    session: Any,
    events: Any,
    context: Any,
    scenario: Any,
    assertions: Any,
    *,
    expected_sha: str,
) -> dict[str, Any]:
    """Validate independent recorder/runtime assertions; never infer success."""
    report: dict[str, Any] = {
        "schema_version": 1,
        "status": "REVIEW_REQUIRED",
        "scenario_id": None,
        "tested_commit": None,
        "events_seen": 0,
        "assertions_passed": 0,
        "reason": "Required evidence unavailable",
        "product_accepted": False,
    }

    def finish(status: str, reason: str) -> dict[str, Any]:
        assert status in RESULTS
        report["status"] = status
        report["reason"] = _safe_reason(reason)
        return report

    if not all(isinstance(v, dict) for v in (session, context, scenario, assertions)):
        return finish("REVIEW_REQUIRED", "Evidence payload must contain JSON objects")
    if not isinstance(events, list) or not all(isinstance(v, dict) for v in events):
        return finish("REVIEW_REQUIRED", "Events must be an array of event objects")

    ident = scenario.get("id")
    if not isinstance(ident, str) or not re.fullmatch(r"[a-z][a-z0-9_-]{2,79}", ident):
        return finish("REVIEW_REQUIRED", "Invalid scenario identifier")
    report["scenario_id"] = ident
    required = scenario.get("required_events")
    req_assertions = scenario.get("required_assertions")
    if (not isinstance(required, list) or not required
            or not all(isinstance(v, str) and v for v in required)
            or not isinstance(req_assertions, list) or not req_assertions
            or not all(isinstance(v, str) and v for v in req_assertions)):
        return finish("REVIEW_REQUIRED", "Scenario must specify event and assertion requirements")

    if not isinstance(expected_sha, str) or not SHA_PATTERN.fullmatch(expected_sha):
        return finish("REVIEW_REQUIRED", "Caller did not provide an exact 40-character commit SHA")
    actual_sha = context.get("git_sha")
    if context.get("repository") != REPOSITORY or not isinstance(actual_sha, str):
        return finish("ENVIRONMENT_FAILURE", "Run repository identity is missing or unexpected")
    if not SHA_PATTERN.fullmatch(actual_sha) or actual_sha != expected_sha:
        return finish("ENVIRONMENT_FAILURE", "Tested build commit does not match requested SHA")
    report["tested_commit"] = actual_sha
    if context.get("execution_mode") not in PLAY_MODES:
        return finish("REVIEW_REQUIRED", "Only real Unreal PIE or packaged sessions qualify")
    if context.get("scenario_id") != ident:
        return finish("REVIEW_REQUIRED", "Scenario identifier mismatches run context")
    if not isinstance(context.get("map"), str) or not context["map"].startswith("/Game/"):
        return finish("REVIEW_REQUIRED", "Unreal test map identity is missing")
    if not isinstance(context.get("seed"), int) or isinstance(context["seed"], bool):
        return finish("REVIEW_REQUIRED", "Deterministic scenario seed is missing")

    if session.get("schema_version") != 1 or session.get("status") != "complete":
        return finish("REVIEW_REQUIRED", "Unreal recorder session is incomplete or unsupported")
    if session.get("session_id") != context.get("session_id"):
        return finish("REVIEW_REQUIRED", "Recorder session ID mismatches execution context")
    if not str(session.get("engine_version", "")).startswith("5.8."):
        return finish("REVIEW_REQUIRED", "Unreal Engine 5.8 runtime evidence missing")
    if session.get("project_name") != "Everward":
        return finish("REVIEW_REQUIRED", "Captured Unreal project is not Everward")

    prev_time = -1.0
    observed: list[str] = []
    for item in events:
        kind = item.get("event_type")
        seconds = item.get("elapsed_seconds")
        if not isinstance(kind, str) or not kind:
            return finish("REVIEW_REQUIRED", "Malformed event type")
        if (not isinstance(seconds, (int, float)) or isinstance(seconds, bool)
                or not math.isfinite(seconds) or seconds < prev_time or seconds < 0):
            return finish("REVIEW_REQUIRED", "Invalid or nonmonotonic event timestamps")
        prev_time = float(seconds)
        observed.append(kind)
    report["events_seen"] = len(observed)
    if not observed or "session_started" not in observed or "session_ended" not in observed:
        return finish("REVIEW_REQUIRED", "Real, completed gameplay event stream missing")
    if any(kind in observed for kind in ("automation_assertion_failed", "fatal_error")):
        return finish("FAIL", "Unreal gameplay reported an explicit failed assertion or fatal error")
    if "issue_marker" in observed:
        return finish("REVIEW_REQUIRED", "Game session contains an unresolved issue marker")

    # A later target occurrence must follow an earlier event; this prevents a
    # set-membership-only check from accepting nonsensical event ordering.
    cursor = 0
    for wanted in required:
        try:
            cursor = observed.index(wanted, cursor) + 1
        except ValueError:
            return finish("REVIEW_REQUIRED", "One or more ordered gameplay events are missing")

    if assertions.get("source_sha") != expected_sha or assertions.get("scenario_id") != ident:
        return finish("REVIEW_REQUIRED", "Authoritative assertion evidence does not match run")
    records = assertions.get("checks")
    if not isinstance(records, list):
        return finish("REVIEW_REQUIRED", "Authoritative gameplay assertion results are missing")
    seen_ids: set[str] = set()
    for check in records:
        if not isinstance(check, dict):
            return finish("REVIEW_REQUIRED", "Malformed gameplay assertion result")
        check_id = check.get("id")
        source = check.get("source")
        passed = check.get("passed")
        if (not isinstance(check_id, str) or not check_id or check_id in seen_ids
                or source not in ASSERTION_SOURCES or type(passed) is not bool):
            return finish("REVIEW_REQUIRED", "Malformed or duplicate authoritative assertion")
        seen_ids.add(check_id)
        if not passed:
            return finish("FAIL", "A genuine Unreal automation assertion reported failure")
    report["assertions_passed"] = len(seen_ids)
    if not all(check_id in seen_ids for check_id in req_assertions):
        return finish("REVIEW_REQUIRED", "Required authoritative assertions are missing")

    # Scope-limited engineering test success. It does not certify subjective
    # gameplay feel, art, footage fidelity or launch-ready Product Reality.
    return finish("PASS", "Captured event chain and named Unreal assertions agree")


def validate_files(
    session_dir: Path, context_file: Path, scenario_file: Path, *,
    expected_sha: str,
) -> dict[str, Any]:
    """No writes or arbitrary command execution: strictly reads local evidence."""
    try:
        if not session_dir.is_dir():
            raise ValueError("Unreal session directory is missing")
        inputs = {
            "session": _load_json(session_dir / "session.json"),
            "events": _load_events(session_dir / "events.jsonl"),
            "context": _load_json(context_file),
            "scenario": _load_json(scenario_file),
            "assertions": _load_json(session_dir / "authoritative_assertions.json"),
        }
        return evaluate(**inputs, expected_sha=expected_sha)
    except (OSError, ValueError, TypeError, json.JSONDecodeError):
        return {
            "schema_version": 1,
            "status": "REVIEW_REQUIRED",
            "scenario_id": None,
            "tested_commit": None,
            "events_seen": 0,
            "assertions_passed": 0,
            "reason": "Required evidence file missing or malformed",
            "product_accepted": False,
        }


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--session-dir", required=True, type=Path)
    parser.add_argument("--run-context", required=True, type=Path)
    parser.add_argument("--scenario", required=True, type=Path)
    parser.add_argument("--expected-sha", required=True)
    parser.add_argument("--report", type=Path,
                        help="Optional local-only JSON report path; existing reports are not overwritten")
    args = parser.parse_args(argv)
    result = validate_files(
        args.session_dir, args.run_context, args.scenario,
        expected_sha=args.expected_sha,
    )
    if args.report:
        try:
            with args.report.open("x", encoding="utf-8") as stream:
                json.dump(result, stream, indent=2, sort_keys=True)
                stream.write("\n")
        except OSError:
            print("REVIEW_REQUIRED: Cannot write a new local report", file=sys.stderr)
            return 2
    print(json.dumps(result, sort_keys=True))
    return 0 if result["status"] == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
