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


if __name__ == "__main__":
    unittest.main()
