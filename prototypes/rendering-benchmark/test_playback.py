import json
import sys
import unittest
from pathlib import Path


HERE = Path(__file__).resolve().parent
if str(HERE) not in sys.path:
    sys.path.insert(0, str(HERE))

import playback  # noqa: E402
import scenario  # noqa: E402


class RenderingPlaybackTests(unittest.TestCase):
    def setUp(self):
        self.scenario = json.loads((HERE / "scenario.json").read_text(encoding="utf-8"))
        scenario.validate_scenario(self.scenario)

    def test_camera_stage_boundaries_are_exact(self):
        self.assertEqual(playback.playback_state(self.scenario, 0)["camera_stage"], "local_mining_closeup")
        self.assertEqual(playback.playback_state(self.scenario, 39.999)["camera_stage"], "local_mining_closeup")
        self.assertEqual(playback.playback_state(self.scenario, 40)["camera_stage"], "probe_and_asteroid_medium")
        self.assertEqual(playback.playback_state(self.scenario, 80)["camera_stage"], "planetary_context_wide")
        self.assertEqual(playback.playback_state(self.scenario, 120)["camera_stage"], "planetary_context_wide")

    def test_accelerated_simulation_time_is_shared_truth(self):
        state = playback.playback_state(self.scenario, 60)
        self.assertEqual(state["elapsed_simulation_seconds"], 60 * 86400)

    def test_stage_progress_resets_at_transition(self):
        before = playback.playback_state(self.scenario, 39.999)
        after = playback.playback_state(self.scenario, 40)
        self.assertGreater(before["camera_stage_progress"], 0.99)
        self.assertEqual(after["camera_stage_progress"], 0.0)

    def test_end_of_capture_finishes_final_stage(self):
        state = playback.playback_state(self.scenario, self.scenario["duration_seconds"])
        self.assertEqual(state["camera_stage_progress"], 1.0)

    def test_out_of_range_time_is_rejected(self):
        with self.assertRaisesRegex(ValueError, "benchmark duration"):
            playback.playback_state(self.scenario, -0.001)
        with self.assertRaisesRegex(ValueError, "benchmark duration"):
            playback.playback_state(self.scenario, 120.001)


if __name__ == "__main__":
    unittest.main()
