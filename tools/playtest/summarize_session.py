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
from pathlib import Path
from statistics import mean


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

    if markers:
        for marker in markers:
            lines.append(
                f"- {marker.get('timestamp_utc', 'unknown')} — {marker.get('details', '')}"
            )
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
