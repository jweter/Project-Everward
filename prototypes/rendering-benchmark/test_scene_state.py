import importlib.util
import json
from pathlib import Path
import sys
import unittest


ROOT = Path(__file__).parent
for module_name in ("playback", "scene_state"):
    if module_name in sys.modules:
        continue
    spec = importlib.util.spec_from_file_location(module_name, ROOT / f"{module_name}.py")
    module = importlib.util.module_from_spec(spec)
    sys.modules[module_name] = module
    assert spec.loader is not None
    spec.loader.exec_module(module)

scene_state = sys.modules["scene_state"]


class RenderingSceneStateTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.scenario = json.loads((ROOT / "scenario.json").read_text(encoding="utf-8"))
        cls.manifest = scene_state.load_manifest(ROOT / "scene_manifest.json")

    def test_manifest_is_bound_to_canonical_scenario(self):
        scene_state.validate_manifest(self.manifest, self.scenario)
        self.assertEqual(set(self.manifest["cameras"]), set(self.scenario["camera_sequence"]))
        self.assertEqual(
            set(self.manifest["feature_bindings"]),
            set(self.scenario["required_scene_features"]),
        )

    def test_scene_state_replays_identically(self):
        first = scene_state.scene_state(self.scenario, self.manifest, 57.25)
        second = scene_state.scene_state(self.scenario, self.manifest, 57.25)
        self.assertEqual(first, second)

    def test_camera_state_tracks_playback_boundaries(self):
        before = scene_state.scene_state(self.scenario, self.manifest, 39.999)
        boundary = scene_state.scene_state(self.scenario, self.manifest, 40.0)
        self.assertEqual(before["camera"]["stage"], "local_mining_closeup")
        self.assertEqual(boundary["camera"]["stage"], "probe_and_asteroid_medium")
        self.assertEqual(boundary["camera"]["stage_progress"], 0.0)

    def test_animation_phases_are_normalized(self):
        state = scene_state.scene_state(self.scenario, self.manifest, 119.5)
        for phase in state["animation"].values():
            self.assertGreaterEqual(phase, 0.0)
            self.assertLess(phase, 1.0)

    def test_hud_uses_accelerated_simulation_truth(self):
        state = scene_state.scene_state(self.scenario, self.manifest, 1.0)
        self.assertEqual(
            state["hud"]["time_acceleration"],
            self.scenario["simulation_seconds_per_real_second"],
        )
        self.assertAlmostEqual(
            state["hud"]["simulation_days_elapsed"],
            self.scenario["simulation_seconds_per_real_second"] / 86400.0,
        )

    def test_manifest_rejects_camera_drift(self):
        altered = json.loads(json.dumps(self.manifest))
        altered["cameras"].pop("planetary_context_wide")
        with self.assertRaisesRegex(ValueError, "cameras must exactly match"):
            scene_state.validate_manifest(altered, self.scenario)

    def test_animation_period_fields_are_exactly_the_three_expected_fields(self):
        self.assertEqual(
            set(scene_state.ANIMATION_PERIOD_FIELDS),
            {
                "mining_mechanism_period_seconds",
                "debris_pulse_period_seconds",
                "asteroid_rotation_period_seconds",
            },
        )

    def test_every_declared_animation_period_field_rejects_a_non_positive_value(self):
        for field in scene_state.ANIMATION_PERIOD_FIELDS:
            for bad_value in (0, -1.5):
                with self.subTest(field=field, bad_value=bad_value):
                    broken = json.loads(json.dumps(self.manifest))
                    broken["animation"][field] = bad_value
                    with self.assertRaisesRegex(ValueError, f"animation.{field} must be positive"):
                        scene_state.validate_manifest(broken, self.scenario)

    def test_every_declared_animation_period_field_rejects_a_missing_value(self):
        for field in scene_state.ANIMATION_PERIOD_FIELDS:
            with self.subTest(field=field):
                broken = json.loads(json.dumps(self.manifest))
                del broken["animation"][field]
                with self.assertRaisesRegex(ValueError, f"animation.{field} must be positive"):
                    scene_state.validate_manifest(broken, self.scenario)


if __name__ == "__main__":
    unittest.main()
