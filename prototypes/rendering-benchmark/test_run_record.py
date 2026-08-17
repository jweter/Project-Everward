import json
import sys
import unittest
from pathlib import Path


HERE = Path(__file__).resolve().parent
if str(HERE) not in sys.path:
    sys.path.insert(0, str(HERE))

import run_record  # noqa: E402
import scenario  # noqa: E402


class RunRecordTests(unittest.TestCase):
    def setUp(self):
        self.scenario = json.loads((HERE / "scenario.json").read_text(encoding="utf-8"))
        self.record = {
            "record_version": 1,
            "engine": "godot",
            "scenario_version": self.scenario["scenario_version"],
            "scenario_name": self.scenario["name"],
            "captured_at_utc": "2026-08-17T20:00:00Z",
            "capture": {
                "engine_version": "4.x",
                "os_version": "test-os",
                "cpu_model": "test-cpu",
                "gpu_model": "test-gpu",
                "ram_gib": 16,
                "project_settings": {"renderer": "forward_plus"},
                "cpu_frame_time_ms": 8.2,
                "gpu_frame_time_ms": 9.1,
                "peak_memory_mib": 1024,
                "implementation_hours": 12.5,
                "build_size_mib": 350,
                "screenshots": ["evidence/godot/local-machinery.png"],
                "notes": "Representative test fixture only.",
            },
        }

    def test_complete_record_is_valid(self):
        run_record.validate_run_record(self.record, self.scenario)

    def test_engine_is_limited_to_candidates_under_evaluation(self):
        broken = dict(self.record)
        broken["engine"] = "other"
        with self.assertRaisesRegex(ValueError, "engine must be"):
            run_record.validate_run_record(broken, self.scenario)

    def test_scenario_mismatch_is_rejected(self):
        broken = dict(self.record)
        broken["scenario_version"] = 999
        with self.assertRaisesRegex(ValueError, "scenario_version"):
            run_record.validate_run_record(broken, self.scenario)

    def test_missing_raw_capture_field_is_rejected(self):
        broken = dict(self.record)
        broken["capture"] = dict(self.record["capture"])
        broken["capture"].pop("gpu_frame_time_ms")
        with self.assertRaisesRegex(ValueError, "invalid capture fields"):
            run_record.validate_run_record(broken, self.scenario)

    def test_negative_measurement_is_rejected(self):
        broken = dict(self.record)
        broken["capture"] = dict(self.record["capture"])
        broken["capture"]["implementation_hours"] = -1
        with self.assertRaisesRegex(ValueError, "cannot be negative"):
            run_record.validate_run_record(broken, self.scenario)

    def test_screenshot_evidence_cannot_be_empty(self):
        broken = dict(self.record)
        broken["capture"] = dict(self.record["capture"])
        broken["capture"]["screenshots"] = []
        with self.assertRaisesRegex(ValueError, "screenshots"):
            run_record.validate_run_record(broken, self.scenario)

    def test_required_capture_contract_remains_exactly_aligned(self):
        self.assertEqual(set(self.record["capture"]), scenario.REQUIRED_CAPTURE_FIELDS)


if __name__ == "__main__":
    unittest.main()
