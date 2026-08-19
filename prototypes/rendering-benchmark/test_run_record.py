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

    def test_every_declared_numeric_field_rejects_a_negative_value(self):
        # NUMERIC_NONNEGATIVE_FIELDS lists five distinct capture fields, but the
        # test above only ever exercised "implementation_hours". A field being
        # silently dropped from that set (typo, refactor, merge conflict) would
        # let a negative measurement for that field pass validation unnoticed,
        # so every declared field must be checked independently, not just one.
        for field in sorted(run_record.NUMERIC_NONNEGATIVE_FIELDS):
            with self.subTest(field=field):
                broken = dict(self.record)
                broken["capture"] = dict(self.record["capture"])
                broken["capture"][field] = -1
                with self.assertRaisesRegex(ValueError, f"capture field {field!r} cannot be negative"):
                    run_record.validate_run_record(broken, self.scenario)

    def test_every_declared_numeric_field_rejects_a_non_numeric_value(self):
        for field in sorted(run_record.NUMERIC_NONNEGATIVE_FIELDS):
            with self.subTest(field=field):
                broken = dict(self.record)
                broken["capture"] = dict(self.record["capture"])
                broken["capture"][field] = "not-a-number"
                with self.assertRaisesRegex(TypeError, f"capture field {field!r} must be numeric"):
                    run_record.validate_run_record(broken, self.scenario)

    def test_declared_numeric_fields_are_exactly_the_five_expected_capture_fields(self):
        # Pins the set itself so a field being added/removed from
        # NUMERIC_NONNEGATIVE_FIELDS is a deliberate, visible change.
        self.assertEqual(
            run_record.NUMERIC_NONNEGATIVE_FIELDS,
            {
                "cpu_frame_time_ms",
                "gpu_frame_time_ms",
                "peak_memory_mib",
                "implementation_hours",
                "build_size_mib",
            },
        )

    def test_every_declared_string_field_rejects_a_non_string_value(self):
        # STRING_CAPTURE_FIELDS lists five distinct capture fields
        # (engine_version, os_version, cpu_model, gpu_model, notes), but no
        # existing test exercised the "must be a string" branch for any of
        # them. A field silently dropped from that set (typo, refactor, merge
        # conflict) could let a non-string value for that field pass
        # validation unnoticed, letting non-comparable evidence (e.g. a
        # numeric cpu_model) feed the Phase 1 engine-decision packet.
        for field in run_record.STRING_CAPTURE_FIELDS:
            with self.subTest(field=field):
                broken = dict(self.record)
                broken["capture"] = dict(self.record["capture"])
                broken["capture"][field] = 12345
                with self.assertRaisesRegex(TypeError, f"capture field {field!r} must be a string"):
                    run_record.validate_run_record(broken, self.scenario)

    def test_every_non_notes_string_field_rejects_an_empty_value(self):
        # Every STRING_CAPTURE_FIELDS entry except "notes" must additionally
        # be non-empty. Only "notes" is deliberately exempt (see
        # test_notes_field_may_be_empty), so each of the other four fields
        # must be checked independently rather than relying on one example.
        for field in run_record.STRING_CAPTURE_FIELDS:
            if field == "notes":
                continue
            with self.subTest(field=field):
                broken = dict(self.record)
                broken["capture"] = dict(self.record["capture"])
                broken["capture"][field] = "   "
                with self.assertRaisesRegex(ValueError, f"capture field {field!r} must be non-empty"):
                    run_record.validate_run_record(broken, self.scenario)

    def test_notes_field_may_be_empty(self):
        # notes is deliberately exempt from the non-empty check: a benchmark
        # run with nothing extra to report is legitimate decision-grade
        # evidence, unlike a run missing its engine/OS/hardware identity.
        allowed = dict(self.record)
        allowed["capture"] = dict(self.record["capture"])
        allowed["capture"]["notes"] = ""
        run_record.validate_run_record(allowed, self.scenario)

    def test_declared_string_fields_are_exactly_the_five_expected_capture_fields(self):
        # Pins the set itself so a field being added/removed from
        # STRING_CAPTURE_FIELDS is a deliberate, visible change; the loop
        # tests above alone cannot detect a field being removed from it.
        self.assertEqual(
            run_record.STRING_CAPTURE_FIELDS,
            ("engine_version", "os_version", "cpu_model", "gpu_model", "notes"),
        )

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
