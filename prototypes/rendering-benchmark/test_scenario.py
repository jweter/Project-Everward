import importlib.util
import json
import sys
import unittest
from pathlib import Path


HERE = Path(__file__).resolve().parent
SPEC = importlib.util.spec_from_file_location("everward_rendering_scenario", HERE / "scenario.py")
MODULE = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = MODULE
assert SPEC.loader is not None
SPEC.loader.exec_module(MODULE)


class ScenarioContractTests(unittest.TestCase):
    def setUp(self):
        self.data = json.loads((HERE / "scenario.json").read_text(encoding="utf-8"))

    def test_repository_scenario_is_valid(self):
        MODULE.validate_scenario(self.data)

    def test_load_scenario_round_trips_repository_file(self):
        self.assertEqual(MODULE.load_scenario(HERE / "scenario.json"), self.data)

    def test_missing_scene_feature_list_is_rejected(self):
        broken = dict(self.data)
        broken.pop("required_scene_features")
        with self.assertRaisesRegex(ValueError, "missing scenario fields"):
            MODULE.validate_scenario(broken)

    def test_invalid_resolution_is_rejected(self):
        broken = dict(self.data)
        broken["target_resolution"] = [2560, 0]
        with self.assertRaisesRegex(ValueError, "target_resolution"):
            MODULE.validate_scenario(broken)

    def test_capture_schema_cannot_silently_drop_raw_measurements(self):
        broken = dict(self.data)
        broken["required_capture"] = [
            item for item in self.data["required_capture"] if item != "gpu_frame_time_ms"
        ]
        with self.assertRaisesRegex(ValueError, "invalid required_capture"):
            MODULE.validate_scenario(broken)

    def test_duplicate_fairness_rule_is_rejected(self):
        broken = dict(self.data)
        broken["fairness_rules"] = list(self.data["fairness_rules"]) + [self.data["fairness_rules"][0]]
        with self.assertRaisesRegex(ValueError, "entries must be unique"):
            MODULE.validate_scenario(broken)


if __name__ == "__main__":
    unittest.main()
