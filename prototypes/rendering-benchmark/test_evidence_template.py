from pathlib import Path
import unittest

from evidence_template import create_evidence_template, missing_evidence_fields
from scenario import load_scenario, REQUIRED_CAPTURE_FIELDS


ROOT = Path(__file__).parent
SCENARIO = load_scenario(ROOT / "scenario.json")


class EvidenceTemplateTests(unittest.TestCase):
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


if __name__ == "__main__":
    unittest.main()
