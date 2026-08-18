import json
from pathlib import Path
import unittest


ROOT = Path(__file__).parent
GODOT = ROOT / "godot"


class GodotBenchmarkAdapterContractTests(unittest.TestCase):
    def test_project_uses_canonical_capture_resolution(self):
        scenario = json.loads((ROOT / "scenario.json").read_text(encoding="utf-8"))
        project = (GODOT / "project.godot").read_text(encoding="utf-8")
        width, height = scenario["target_resolution"]
        self.assertIn(f"window/size/viewport_width={width}", project)
        self.assertIn(f"window/size/viewport_height={height}", project)

    def test_adapter_consumes_self_contained_handoff_contract(self):
        adapter = (GODOT / "benchmark_adapter.gd").read_text(encoding="utf-8")
        required_keys = (
            "handoff_version",
            "scenario_name",
            "scenario_version",
            "duration_seconds",
            "target_resolution",
            "target_fps",
            "simulation_seconds_per_real_second",
            "camera_sequence",
            "camera_stage_durations_seconds",
            "required_scene_features",
            "objects",
            "cameras",
            "animation_periods_seconds",
            "feature_bindings",
        )
        for key in required_keys:
            with self.subTest(key=key):
                self.assertIn(f'"{key}"', adapter)
        self.assertIn("EXPECTED_HANDOFF_VERSION := 2", adapter)
        self.assertIn('EXPECTED_SCENARIO_NAME := "icy-asteroid-mining"', adapter)

    def test_scene_exposes_nodes_required_by_adapter(self):
        scene = (GODOT / "main.tscn").read_text(encoding="utf-8")
        for node_name in ("Asteroid", "Probe", "Planet", "StarLight", "BenchmarkCamera", "HUD", "Telemetry"):
            with self.subTest(node=node_name):
                self.assertIn(f'name="{node_name}"', scene)

    def test_adapter_derives_playback_from_handoff_not_local_constants(self):
        adapter = (GODOT / "benchmark_adapter.gd").read_text(encoding="utf-8")
        self.assertIn('handoff["camera_sequence"]', adapter)
        self.assertIn('handoff["camera_stage_durations_seconds"]', adapter)
        self.assertIn('handoff["animation_periods_seconds"]', adapter)
        self.assertIn('handoff["simulation_seconds_per_real_second"]', adapter)
        self.assertNotIn("86400", adapter)
        self.assertNotIn("40.0", adapter)


if __name__ == "__main__":
    unittest.main()
