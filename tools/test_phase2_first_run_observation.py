from __future__ import annotations

import copy
import json
import unittest
from pathlib import Path

from validate_phase2_first_run_observation import REQUIRED_CHECKS, validate_observation


ROOT = Path(__file__).resolve().parents[1]
TEMPLATE = ROOT / "playtests" / "phase2" / "first_run_observation.template.json"


class Phase2FirstRunObservationTests(unittest.TestCase):
    def setUp(self) -> None:
        self.template = json.loads(TEMPLATE.read_text(encoding="utf-8"))

    def test_template_is_valid_not_tested_observation(self) -> None:
        self.assertEqual(validate_observation(self.template), [])

    def test_complete_pass_is_valid(self) -> None:
        observation = copy.deepcopy(self.template)
        observation["overall_result"] = "pass"
        for check_id in REQUIRED_CHECKS:
            observation["checks"][check_id]["status"] = "pass"
        for key in observation["subjective_ratings"]:
            observation["subjective_ratings"][key] = 3
        self.assertEqual(validate_observation(observation), [])

    def test_failure_forces_failed_overall(self) -> None:
        observation = copy.deepcopy(self.template)
        for check_id in REQUIRED_CHECKS:
            observation["checks"][check_id]["status"] = "pass"
        observation["checks"]["scan_cancel"]["status"] = "fail"
        observation["overall_result"] = "pass"
        errors = validate_observation(observation)
        self.assertTrue(any("imply 'fail'" in error for error in errors))

    def test_not_tested_check_forces_partial_overall(self) -> None:
        observation = copy.deepcopy(self.template)
        for check_id in REQUIRED_CHECKS:
            observation["checks"][check_id]["status"] = "pass"
        observation["checks"]["movement_z"]["status"] = "not_tested"
        observation["overall_result"] = "pass"
        errors = validate_observation(observation)
        self.assertTrue(any("imply 'partial'" in error for error in errors))

    def test_subjective_ratings_are_one_to_five_or_null(self) -> None:
        observation = copy.deepcopy(self.template)
        observation["subjective_ratings"]["embodiment"] = 6
        errors = validate_observation(observation)
        self.assertTrue(any("subjective_ratings.embodiment" in error for error in errors))


if __name__ == "__main__":
    unittest.main()
