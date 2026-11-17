from __future__ import annotations

import importlib.util
import json
import tempfile
import unittest
from pathlib import Path


MODULE_PATH = Path(__file__).parent / "playtest" / "summarize_session.py"
SPEC = importlib.util.spec_from_file_location("everward_playtest_summary", MODULE_PATH)
assert SPEC and SPEC.loader
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def _write_session(session_dir: Path, *, status: str, events: list[dict]) -> None:
    session_dir.joinpath("session.json").write_text(
        json.dumps({"session_id": "20260830T230000Z", "status": status}),
        encoding="utf-8",
    )
    session_dir.joinpath("events.jsonl").write_text(
        "\n".join(json.dumps(event) for event in events) + ("\n" if events else ""),
        encoding="utf-8",
    )


_COMPLETION_EVENTS = [
    {"timestamp_utc": "2026-08-30T23:00:00Z", "event_type": "session_started"},
    {"timestamp_utc": "2026-08-30T23:00:59Z", "event_type": "session_ended"},
]


class PlaytestSummaryTests(unittest.TestCase):
    def test_report_summarizes_markers_and_telemetry(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            session = Path(temp)
            (session / "session.json").write_text(
                json.dumps(
                    {
                        "session_id": "20260830T230000Z",
                        "status": "complete",
                        "started_utc": "2026-08-30T23:00:00Z",
                        "ended_utc": "2026-08-30T23:01:00Z",
                        "engine_version": "5.x",
                        "build_configuration": "Development",
                    }
                ),
                encoding="utf-8",
            )
            events = [
                {"timestamp_utc": "2026-08-30T23:00:05Z", "event_type": "scanner_target_selected", "details": "asteroid_01"},
                {"timestamp_utc": "2026-08-30T23:00:10Z", "event_type": "issue_marker", "details": "marker=1;note=F9 manual playtest marker"},
            ]
            (session / "events.jsonl").write_text(
                "\n".join(json.dumps(event) for event in events) + "\n",
                encoding="utf-8",
            )
            (session / "telemetry.csv").write_text(
                "timestamp_utc,elapsed_seconds,delta_seconds,fps,frame_ms,pawn_x,pawn_y,pawn_z,pawn_speed\n"
                '"2026-08-30T23:00:00Z",0.0,0.016,62.5,16.0,0,0,0,0\n'
                '"2026-08-30T23:00:01Z",1.0,0.020,50.0,20.0,0,0,0,1\n',
                encoding="utf-8",
            )

            report = MODULE.build_report(session)

            self.assertIn("Issue markers: **1**", report)
            self.assertIn("Average sampled FPS: 56.2", report)
            self.assertIn("scanner_target_selected", report)
            self.assertIn("not a development phase gate", report)

    def test_missing_optional_evidence_is_supported(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            report = MODULE.build_report(Path(temp))
            self.assertIn("Average sampled FPS: n/a", report)
            self.assertIn("None recorded", report)

    def test_duplicate_marker_details_are_deduped(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            session = Path(temp)
            events = [
                *_COMPLETION_EVENTS,
                {"event_type": "issue_marker", "details": "Ramp collision glitch"},
                {"event_type": "issue_marker", "details": "  ramp collision glitch  "},
                {"event_type": "issue_marker", "details": "Second distinct issue"},
            ]
            _write_session(session, status="complete", events=events)

            report = MODULE.build_report(session)

            self.assertEqual(report.count("Ramp collision glitch"), 1)
            self.assertIn("Second distinct issue", report)
            self.assertIn("Issue markers: **3**", report)

    def test_classified_blocker_outranks_unclassified_markers(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            session = Path(temp)
            events = [
                *_COMPLETION_EVENTS,
                {"event_type": "issue_marker", "details": "No repro steps yet"},
                {
                    "event_type": "issue_marker",
                    "details": "Ship crashes on launch",
                    "classification": "blocker",
                },
            ]
            _write_session(session, status="complete", events=events)

            report = MODULE.build_report(session)

            self.assertIn("Overall verdict: **BLOCKER**", report)

    def test_unclassified_markers_report_pending(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            session = Path(temp)
            events = [
                *_COMPLETION_EVENTS,
                {"event_type": "issue_marker", "details": "Unreviewed finding"},
            ]
            _write_session(session, status="complete", events=events)

            report = MODULE.build_report(session)

            self.assertIn("Overall verdict: **PENDING**", report)

    def test_unknown_classification_reports_pending(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            session = Path(temp)
            events = [
                *_COMPLETION_EVENTS,
                {
                    "event_type": "issue_marker",
                    "details": "Needs triage",
                    "classification": "needs-triage",
                },
            ]
            _write_session(session, status="complete", events=events)

            report = MODULE.build_report(session)

            self.assertIn("Overall verdict: **PENDING**", report)

    def test_no_markers_reports_pending(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            session = Path(temp)
            _write_session(session, status="complete", events=list(_COMPLETION_EVENTS))

            report = MODULE.build_report(session)

            self.assertIn("Overall verdict: **PENDING**", report)
            self.assertIn("None recorded", report)

    def test_complete_fully_classified_pass_session_reports_pass(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            session = Path(temp)
            events = [
                *_COMPLETION_EVENTS,
                {
                    "event_type": "issue_marker",
                    "details": "Minor VFX polish noted, no functional impact",
                    "classification": "pass",
                },
            ]
            _write_session(session, status="complete", events=events)

            report = MODULE.build_report(session)

            self.assertIn("Overall verdict: **PASS**", report)


if __name__ == "__main__":
    unittest.main()
