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

    def test_camera_stage_durations_must_match_sequence(self):
        broken = dict(self.data)
        broken["camera_stage_durations_seconds"] = [60, 60]
        with self.assertRaisesRegex(ValueError, "one positive integer per camera stage"):
            MODULE.validate_scenario(broken)

    def test_camera_stage_durations_must_cover_full_capture(self):
        broken = dict(self.data)
        broken["camera_stage_durations_seconds"] = [30, 30, 30]
        with self.assertRaisesRegex(ValueError, "sum to duration_seconds"):
            MODULE.validate_scenario(broken)

    def test_simulation_time_scale_must_be_positive(self):
        broken = dict(self.data)
        broken["simulation_seconds_per_real_second"] = 0
        with self.assertRaisesRegex(ValueError, "simulation_seconds_per_real_second"):
            MODULE.validate_scenario(broken)

    def test_every_declared_positive_integer_field_rejects_zero_and_negative(self):
        for field in MODULE.POSITIVE_INTEGER_FIELDS:
            for bad_value in (0, -1):
                with self.subTest(field=field, bad_value=bad_value):
                    broken = dict(self.data)
                    broken[field] = bad_value
                    with self.assertRaisesRegex(ValueError, f"{field} must be a positive integer"):
                        MODULE.validate_scenario(broken)

    def test_every_declared_positive_integer_field_rejects_a_non_integer_value(self):
        for field in MODULE.POSITIVE_INTEGER_FIELDS:
            with self.subTest(field=field):
                broken = dict(self.data)
                broken[field] = 12.5
                with self.assertRaisesRegex(ValueError, f"{field} must be a positive integer"):
                    MODULE.validate_scenario(broken)

    def test_positive_integer_fields_are_exactly_the_three_expected_fields(self):
        self.assertEqual(
            set(MODULE.POSITIVE_INTEGER_FIELDS),
            {"target_fps", "duration_seconds", "simulation_seconds_per_real_second"},
        )

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

    def test_every_declared_list_field_rejects_an_empty_list(self):
        for field in MODULE.UNIQUE_STRING_LIST_FIELDS:
            with self.subTest(field=field):
                broken = dict(self.data)
                broken[field] = []
                with self.assertRaisesRegex(ValueError, f"{field} must be a non-empty list"):
                    MODULE.validate_scenario(broken)

    def test_every_declared_list_field_rejects_a_blank_entry(self):
        for field in MODULE.UNIQUE_STRING_LIST_FIELDS:
            with self.subTest(field=field):
                broken = dict(self.data)
                broken[field] = list(self.data[field]) + ["   "]
                with self.assertRaisesRegex(ValueError, f"{field} entries must be non-empty strings"):
                    MODULE.validate_scenario(broken)

    def test_every_declared_list_field_rejects_a_duplicate_entry(self):
        for field in MODULE.UNIQUE_STRING_LIST_FIELDS:
            with self.subTest(field=field):
                broken = dict(self.data)
                broken[field] = list(self.data[field]) + [self.data[field][0]]
                with self.assertRaisesRegex(ValueError, f"{field} entries must be unique"):
                    MODULE.validate_scenario(broken)

    def test_unique_string_list_fields_are_exactly_the_three_expected_fields(self):
        self.assertEqual(
            set(MODULE.UNIQUE_STRING_LIST_FIELDS),
            {"camera_sequence", "required_scene_features", "fairness_rules"},
        )


if __name__ == "__main__":
    unittest.main()
