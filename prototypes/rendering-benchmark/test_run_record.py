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
            "engine": "unreal",
            "scenario_version": self.scenario["scenario_version"],
            "scenario_name": self.scenario["name"],
            "captured_at_utc": "2026-08-21T02:12:57Z",
            "capture": {
                "engine_version": "5.8.1",
                "os_version": "test-os",
                "cpu_model": "test-cpu",
                "gpu_model": "test-gpu",
                "ram_gib": 16,
                "project_settings": {"renderer": "deferred"},
                "cpu_frame_time_ms": 8.2,
                "gpu_frame_time_ms": 9.1,
                "peak_memory_mib": 1024,
                "implementation_hours": 12.5,
                "build_size_mib": 350,
                "screenshots": ["evidence/unreal/local-machinery.png"],
                "notes": "Representative test fixture only.",
            },
        }

    def test_complete_record_is_valid(self):
        run_record.validate_run_record(self.record, self.scenario)

    def test_engine_is_locked_to_unreal(self):
        broken = dict(self.record)
        broken["engine"] = "other"
        with self.assertRaisesRegex(ValueError, "engine must be 'unreal'"):
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

    def test_every_declared_numeric_field_rejects_negative_and_non_numeric_values(self):
        for field in sorted(run_record.NUMERIC_NONNEGATIVE_FIELDS):
            with self.subTest(field=field, case="negative"):
                broken = dict(self.record)
                broken["capture"] = dict(self.record["capture"])
                broken["capture"][field] = -1
                with self.assertRaisesRegex(ValueError, "cannot be negative"):
                    run_record.validate_run_record(broken, self.scenario)
            with self.subTest(field=field, case="type"):
                broken = dict(self.record)
                broken["capture"] = dict(self.record["capture"])
                broken["capture"][field] = "not-a-number"
                with self.assertRaisesRegex(TypeError, "must be numeric"):
                    run_record.validate_run_record(broken, self.scenario)

    def test_declared_numeric_fields_are_exactly_expected(self):
        self.assertEqual(
            run_record.NUMERIC_NONNEGATIVE_FIELDS,
            {"cpu_frame_time_ms", "gpu_frame_time_ms", "peak_memory_mib", "implementation_hours", "build_size_mib"},
        )

    def test_string_fields_are_validated(self):
        for field in run_record.STRING_CAPTURE_FIELDS:
            with self.subTest(field=field):
                broken = dict(self.record)
                broken["capture"] = dict(self.record["capture"])
                broken["capture"][field] = 12345
                with self.assertRaisesRegex(TypeError, "must be a string"):
                    run_record.validate_run_record(broken, self.scenario)

    def test_non_notes_string_fields_reject_empty_values(self):
        for field in run_record.STRING_CAPTURE_FIELDS:
            if field == "notes":
                continue
            with self.subTest(field=field):
                broken = dict(self.record)
                broken["capture"] = dict(self.record["capture"])
                broken["capture"][field] = "   "
                with self.assertRaisesRegex(ValueError, "must be non-empty"):
                    run_record.validate_run_record(broken, self.scenario)

    def test_notes_field_may_be_empty(self):
        allowed = dict(self.record)
        allowed["capture"] = dict(self.record["capture"])
        allowed["capture"]["notes"] = ""
        run_record.validate_run_record(allowed, self.scenario)

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
