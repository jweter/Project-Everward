#!/usr/bin/env python3
"""Summarize one Everward Unreal playtest session into a small Markdown report.

Raw sessions are intentionally local evidence. This utility produces the compact,
reviewable artifact that may be copied into the repository or attached to an issue.
It uses only the Python standard library.
"""

from __future__ import annotations

import argparse
import csv
import json
import math
import sys
from pathlib import Path
from statistics import mean

sys.path.insert(0, str(Path(__file__).resolve().parent))

from issue_marker_dedupe import dedupe_issue_marker_details  # noqa: E402
from verdict_rollup import rollup_playtest_verdict  # noqa: E402

KNOWN_CLASSIFICATIONS = {"PASS", "PARTIAL", "FAIL", "BLOCKER"}


def load_json(path: Path) -> dict:
    if not path.exists():
        return {}
    return json.loads(path.read_text(encoding="utf-8"))


def load_events(path: Path) -> list[dict]:
    if not path.exists():
        return []
    events: list[dict] = []
    for line in path.read_text(encoding="utf-8").splitlines():
        if line.strip():
            events.append(json.loads(line))
    return events


def percentile(values: list[float], fraction: float) -> float | None:
    if not values:
        return None
    ordered = sorted(values)
    index = max(0, min(len(ordered) - 1, math.ceil(fraction * len(ordered)) - 1))
    return ordered[index]


def load_telemetry(path: Path) -> dict[str, float | int | None]:
    if not path.exists():
        return {"samples": 0, "avg_fps": None, "one_percent_low_fps": None, "max_frame_ms": None}

    fps_values: list[float] = []
    frame_values: list[float] = []
    with path.open("r", encoding="utf-8-sig", newline="") as handle:
        for row in csv.DictReader(handle):
            try:
                fps_values.append(float(row["fps"]))
                frame_values.append(float(row["frame_ms"]))
            except (KeyError, TypeError, ValueError):
                continue

    if not fps_values:
        return {"samples": 0, "avg_fps": None, "one_percent_low_fps": None, "max_frame_ms": None}

    # A practical approximation using the 1st percentile of sampled instantaneous FPS.
    one_percent_low = percentile(fps_values, 0.01)
    return {
        "samples": len(fps_values),
        "avg_fps": mean(fps_values),
        "one_percent_low_fps": one_percent_low,
        "max_frame_ms": max(frame_values) if frame_values else None,
    }


def fmt(value: float | int | None, digits: int = 1) -> str:
    if value is None:
        return "n/a"
    if isinstance(value, int):
        return str(value)
    return f"{value:.{digits}f}"


def normalize_classification(marker: dict) -> str:
    """Return the marker's canonical classification, or UNCLASSIFIED.

    Absent, blank, non-string, or unrecognized values are UNCLASSIFIED
    rather than defaulting to PASS.
    """
    value = marker.get("classification")
    if not isinstance(value, str):
        return "UNCLASSIFIED"
    canonical = value.strip().upper()
    return canonical if canonical in KNOWN_CLASSIFICATIONS else "UNCLASSIFIED"


def has_verified_completion_signal(metadata: dict, events: list[dict]) -> bool:
    """Return True only when the recorder itself attests the session finished."""
    if metadata.get("status") != "complete":
        return False
    return any(event.get("event_type") == "session_ended" for event in events)


def compute_overall_verdict(markers: list[dict], completion_signal: bool) -> str:
    """Return the session's overall verdict.

    No issue markers alone is not evidence of a passing playtest, so an
    empty marker list is PENDING. Known BLOCKER/FAIL findings are preserved
    even when other markers are unclassified or completion evidence is
    absent. A PASS requires every marker classified, a verified completion
    signal, and no greater severity than the rollup helper reports.
    """
    if not markers:
        return "PENDING"
    classifications = [normalize_classification(marker) for marker in markers]
    recognized = [c for c in classifications if c != "UNCLASSIFIED"]
    severity_verdict = rollup_playtest_verdict(recognized)
    if severity_verdict in ("BLOCKER", "FAIL"):
        return severity_verdict
    if "UNCLASSIFIED" in classifications or not completion_signal:
        return "PENDING"
    return severity_verdict


def build_report(session_dir: Path) -> str:
    metadata = load_json(session_dir / "session.json")
    events = load_events(session_dir / "events.jsonl")
    telemetry = load_telemetry(session_dir / "telemetry.csv")

    markers = [event for event in events if event.get("event_type") == "issue_marker"]
    gameplay_events = [
        event
        for event in events
        if event.get("event_type") not in {"session_started", "session_ended", "issue_marker"}
    ]

    completion_signal = has_verified_completion_signal(metadata, events)
    overall_verdict = compute_overall_verdict(markers, completion_signal)
    deduped_marker_details = dedupe_issue_marker_details(
        [marker.get("details", "") for marker in markers]
    )
    # The dedupe helper retains only the first original details string.
    # Preserve that same event's time for recorder/gameplay correlation.
    first_marker_timestamp_by_detail: dict[str, str] = {}
    for marker in markers:
        detail = marker.get("details", "")
        normalized_detail = detail.strip().lower()
        if normalized_detail and normalized_detail not in first_marker_timestamp_by_detail:
            first_marker_timestamp_by_detail[normalized_detail] = marker.get(
                "timestamp_utc", "unknown"
            )

    lines = [
        f"# Everward Playtest Report — {metadata.get('session_id', session_dir.name)}",
        "",
        "> Validation evidence only. This report is not a development phase gate.",
        "",
        "## Session",
        "",
        f"- Status: **{metadata.get('status', 'unknown')}**",
        f"- Started UTC: {metadata.get('started_utc', 'unknown')}",
        f"- Ended UTC: {metadata.get('ended_utc') or 'unknown'}",
        f"- Engine: {metadata.get('engine_version', 'unknown')}",
        f"- Build configuration: {metadata.get('build_configuration', 'unknown')}",
        f"- Issue markers: **{len(markers)}**",
        f"- Overall verdict: **{overall_verdict}**",
        "",
        "## Performance sample",
        "",
        f"- Samples: {telemetry['samples']}",
        f"- Average sampled FPS: {fmt(telemetry['avg_fps'])}",
        f"- Approx. 1% low sampled FPS: {fmt(telemetry['one_percent_low_fps'])}",
        f"- Worst sampled frame: {fmt(telemetry['max_frame_ms'])} ms",
        "",
        "## Issue markers",
        "",
    ]

    if deduped_marker_details:
        for detail in deduped_marker_details:
            timestamp = first_marker_timestamp_by_detail[detail.strip().lower()]
            lines.append(f"- {timestamp} — {detail}")
    else:
        lines.append("- None recorded.")

    lines.extend(["", "## Instrumented gameplay events", ""])
    if gameplay_events:
        for event in gameplay_events[-50:]:
            lines.append(
                f"- {event.get('timestamp_utc', 'unknown')} — "
                f"`{event.get('event_type', 'unknown')}` — {event.get('details', '')}"
            )
    else:
        lines.append("- No feature-specific events have been instrumented in this session yet.")

    lines.extend(
        [
            "",
            "## Development handling",
            "",
            "Classify findings as PASS, PARTIAL, FAIL, or BLOCKER. Only BLOCKER findings "
            "should stop dependent development; unrelated roadmap work continues.",
            "",
        ]
    )
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("session_dir", type=Path, help="Saved/Playtests/<session-id> directory")
    parser.add_argument("--output", type=Path, help="Markdown output path")
    args = parser.parse_args()

    session_dir = args.session_dir.resolve()
    if not session_dir.is_dir():
        parser.error(f"session directory does not exist: {session_dir}")

    report = build_report(session_dir)
    output = args.output or session_dir / "playtest_report.md"
    output.write_text(report, encoding="utf-8")
    print(output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
