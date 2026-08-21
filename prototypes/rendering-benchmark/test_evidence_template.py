from pathlib import Path
import unittest

from evidence_template import create_evidence_template, missing_evidence_fields
from scenario import load_scenario, REQUIRED_CAPTURE_FIELDS

ROOT = Path(__file__).parent
SCENARIO = load_scenario(ROOT / "scenario.json")


class EvidenceTemplateTests(unittest.TestCase):
    def complete_record(self):
        record = create_evidence_template("unreal", SCENARIO)
        record["captured_at_utc"] = "2026-08-21T02:12:57Z"
        record["capture"].update(
            {
                "engine_version": "5.8.1",
                "os_version": "Windows",
                "cpu_model": "i7-13700H",
                "gpu_model": "Intel Iris Xe Graphics",
                "ram_gib": 16,
                "project_settings": {"renderer": "deferred"},
                "cpu_frame_time_ms": 9.35,
                "gpu_frame_time_ms": 65.3,
                "peak_memory_mib": 4712,
                "implementation_hours": 1,
                "build_size_mib": 512,
                "screenshots": ["frame_0001.png"],
                "notes": "",
            }
        )
        self.assertEqual(set(record["capture"]), REQUIRED_CAPTURE_FIELDS)
        return record

    def test_unreal_receives_complete_capture_contract(self):
        unreal = create_evidence_template("unreal", SCENARIO)
        self.assertEqual(set(unreal["capture"]), REQUIRED_CAPTURE_FIELDS)

    def test_template_is_bound_to_canonical_scenario(self):
        record = create_evidence_template("unreal", SCENARIO)
        self.assertEqual(record["scenario_version"], SCENARIO["scenario_version"])
        self.assertEqual(record["scenario_name"], SCENARIO["name"])

    def test_missing_evidence_is_explicit(self):
        record = create_evidence_template("unreal", SCENARIO)
        missing = missing_evidence_fields(record)
        self.assertIn("captured_at_utc", missing)
        self.assertIn("capture.cpu_frame_time_ms", missing)
        self.assertIn("capture.gpu_frame_time_ms", missing)
        self.assertIn("capture.project_settings", missing)
        self.assertIn("capture.screenshots", missing)
        self.assertNotIn("capture.notes", missing)

    def test_rejects_non_unreal_engine(self):
        with self.assertRaisesRegex(ValueError, "engine must be 'unreal'"):
            create_evidence_template("other", SCENARIO)

    def test_fully_populated_record_has_no_missing_fields(self):
        self.assertEqual(missing_evidence_fields(self.complete_record()), [])

    def test_every_declared_capture_field_is_flagged_when_absent_or_none(self):
        for field in sorted(REQUIRED_CAPTURE_FIELDS):
            with self.subTest(field=field, case="absent"):
                record = self.complete_record()
                del record["capture"][field]
                self.assertIn(f"capture.{field}", missing_evidence_fields(record))
            with self.subTest(field=field, case="none"):
                record = self.complete_record()
                record["capture"][field] = None
                self.assertIn(f"capture.{field}", missing_evidence_fields(record))

    def test_blank_value_is_flagged_for_every_field_except_notes(self):
        for field in sorted(REQUIRED_CAPTURE_FIELDS):
            with self.subTest(field=field):
                record = self.complete_record()
                record["capture"][field] = "   "
                missing = missing_evidence_fields(record)
                if field == "notes":
                    self.assertNotIn("capture.notes", missing)
                else:
                    self.assertIn(f"capture.{field}", missing)

    def test_capture_that_is_not_a_mapping_is_reported_as_missing(self):
        for bad_capture in (None, [], "not-a-mapping"):
            with self.subTest(bad_capture=bad_capture):
                record = self.complete_record()
                record["capture"] = bad_capture
                self.assertEqual(missing_evidence_fields(record), ["capture"])


if __name__ == "__main__":
    unittest.main()
