from pathlib import Path
import unittest


ROOT = Path(__file__).parent
SCENE = (ROOT / "godot" / "main.tscn").read_text(encoding="utf-8")
ADAPTER = (ROOT / "godot" / "benchmark_adapter.gd").read_text(encoding="utf-8")


class GodotVisualContractTests(unittest.TestCase):
    def test_scene_contains_required_presentation_systems(self):
        required_markers = {
            "stellar directional illumination": 'type="DirectionalLight3D"',
            "large planet backdrop": 'name="Planet" type="MeshInstance3D"',
            "icy asteroid surface": 'Material_ice',
            "moving mining machinery": 'name="MiningArm" type="MeshInstance3D"',
            "particle debris": 'type="GPUParticles3D"',
            "environmental volumetrics": 'volumetric_fog_enabled = true',
            "interactive HUD telemetry": 'name="Telemetry" type="Label"',
        }
        for feature, marker in required_markers.items():
            with self.subTest(feature=feature):
                self.assertIn(marker, SCENE)

    def test_mining_motion_is_driven_by_canonical_animation_period(self):
        self.assertIn('periods["mining_mechanism"]', ADAPTER)
        self.assertIn("mining_arm.rotation.x", ADAPTER)
        self.assertIn("mining_arm.position.z", ADAPTER)

    def test_debris_is_bound_to_probe_mining_location(self):
        self.assertIn("mining_debris.position = probe.position", ADAPTER)

    def test_scene_keeps_simulation_truth_outside_renderer(self):
        self.assertIn('HANDOFF_PATH := "res://benchmark_handoff.json"', ADAPTER)
        self.assertNotIn("RandomNumberGenerator", ADAPTER)
        self.assertNotIn("randf", ADAPTER)
        self.assertNotIn("randi", ADAPTER)


if __name__ == "__main__":
    unittest.main()
