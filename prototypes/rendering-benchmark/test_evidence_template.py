from pathlib import Path
import unittest

from evidence_template import create_evidence_template, missing_evidence_fields
from scenario import load_scenario, REQUIRED_CAPTURE_FIELDS


ROOT = Path(__file__).parent
SCENARIO = load_scenario(ROOT / "scenario.json")


class EvidenceTemplateTests(unittest.TestCase):
    def complete_record(self):
        # A fully populated, decision-grade-shaped record: every declared
        # capture field carries a realistic non-empty value. Used to prove
        # missing_evidence_fields() correctly reports "nothing missing" once
        # real evidence is captured, and as the mutation base for the
        # per-field absent/None/blank tests below.
        record = create_evidence_template("unreal", SCENARIO)
        record["captured_at_utc"] = "2026-08-20T00:00:00Z"
        record["capture"].update(
            {
                "engine_version": "5.3.1",
                "os_version": "Ubuntu 22.04 LTS",
                "cpu_model": "AMD Ryzen 9 7950X",
                "gpu_model": "NVIDIA GeForce RTX 4090",
                "ram_gib": 64,
                "project_settings": {"renderer": "forward_plus"},
                "cpu_frame_time_ms": 8.2,
                "gpu_frame_time_ms": 10.4,
                "peak_memory_mib": 4096,
                "implementation_hours": 12,
                "build_size_mib": 512,
                "screenshots": ["frame_0001.png"],
                "notes": "",
            }
        )
        self.assertEqual(set(record["capture"]), REQUIRED_CAPTURE_FIELDS)
        return record

    def test_candidates_receive_identical_capture_fields(self):
        godot = create_evidence_template("godot", SCENARIO)
        unreal = create_evidence_template("unreal", SCENARIO)
        self.assertEqual(set(godot["capture"]), REQUIRED_CAPTURE_FIELDS)
        self.assertEqual(set(unreal["capture"]), REQUIRED_CAPTURE_FIELDS)
        self.assertEqual(set(godot["capture"]), set(unreal["capture"]))

    def test_template_is_bound_to_canonical_scenario(self):
        record = create_evidence_template("godot", SCENARIO)
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

    def test_rejects_unsupported_engine(self):
        with self.assertRaises(ValueError):
            create_evidence_template("other", SCENARIO)

    def test_a_fully_populated_record_has_no_missing_fields(self):
        # The inverse of test_missing_evidence_is_explicit: proves real,
        # fully captured evidence is recognized as complete rather than
        # merely proving that an empty scaffold is recognized as incomplete.
        self.assertEqual(missing_evidence_fields(self.complete_record()), [])

    def test_every_declared_capture_field_is_flagged_when_absent(self):
        # test_missing_evidence_is_explicit above only ever exercises the
        # freshly created scaffold, where every field already happens to be
        # None, "", {} or []. It never removes a capture key outright, so a
        # field silently dropped from the per-field loop's "field not in
        # capture" branch (typo, refactor, merge conflict) would not be
        # caught for any of the other twelve fields.
        for field in sorted(REQUIRED_CAPTURE_FIELDS):
            with self.subTest(field=field):
                record = self.complete_record()
                del record["capture"][field]
                self.assertIn(f"capture.{field}", missing_evidence_fields(record))

    def test_every_declared_capture_field_is_flagged_when_none(self):
        # Only cpu_frame_time_ms and gpu_frame_time_ms were ever asserted
        # missing via a None value (implicitly, from the default scaffold).
        # The other eleven fields' None handling was never independently
        # exercised.
        for field in sorted(REQUIRED_CAPTURE_FIELDS):
            with self.subTest(field=field):
                record = self.complete_record()
                record["capture"][field] = None
                self.assertIn(f"capture.{field}", missing_evidence_fields(record))

    def test_blank_value_is_flagged_for_every_field_except_notes(self):
        # The blank/whitespace-only string branch had zero per-field
        # coverage; "notes" is the one declared field deliberately exempt
        # from it (a run with no notes is legitimate evidence).
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
        # The `if not isinstance(capture, Mapping)` early-return branch had
        # no test coverage at all.
        for bad_capture in (None, [], "not-a-mapping"):
            with self.subTest(bad_capture=bad_capture):
                record = self.complete_record()
                record["capture"] = bad_capture
                self.assertEqual(missing_evidence_fields(record), ["capture"])


if __name__ == "__main__":
    unittest.main()
