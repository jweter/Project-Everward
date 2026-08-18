import importlib.util
import json
from pathlib import Path
import sys
import tempfile
import unittest


ROOT = Path(__file__).parent
for module_name in ("playback", "scenario", "scene_state", "export_handoff"):
    if module_name in sys.modules:
        continue
    spec = importlib.util.spec_from_file_location(module_name, ROOT / f"{module_name}.py")
    module = importlib.util.module_from_spec(spec)
    sys.modules[module_name] = module
    assert spec.loader is not None
    spec.loader.exec_module(module)

scenario_module = sys.modules["scenario"]
scene_state_module = sys.modules["scene_state"]
export_handoff = sys.modules["export_handoff"]


class EngineHandoffBundleTests(unittest.TestCase):
    def setUp(self):
        self.scenario = scenario_module.load_scenario(ROOT / "scenario.json")
        self.manifest = scene_state_module.load_manifest(ROOT / "scene_manifest.json")

    def test_default_bundle_is_deterministic(self):
        first = export_handoff.build_handoff_bundle(self.scenario, self.manifest)
        second = export_handoff.build_handoff_bundle(self.scenario, self.manifest)
        self.assertEqual(first, second)

    def test_bundle_covers_each_camera_stage_transition(self):
        bundle = export_handoff.build_handoff_bundle(self.scenario, self.manifest)
        stages = [frame["camera"]["stage"] for frame in bundle["frames"]]
        self.assertIn("local_mining_closeup", stages)
        self.assertIn("probe_and_asteroid_medium", stages)
        self.assertIn("planetary_context_wide", stages)
        self.assertEqual(bundle["frames"][3]["elapsed_real_seconds"], 40.0)
        self.assertEqual(bundle["frames"][6]["elapsed_real_seconds"], 80.0)

    def test_handoff_identity_matches_canonical_scenario(self):
        bundle = export_handoff.build_handoff_bundle(self.scenario, self.manifest)
        self.assertEqual(bundle["scenario_name"], self.scenario["name"])
        self.assertEqual(bundle["scenario_version"], self.scenario["scenario_version"])
        self.assertEqual(bundle["target_resolution"], self.scenario["target_resolution"])
        self.assertEqual(bundle["target_fps"], self.scenario["target_fps"])

    def test_rejects_out_of_range_or_unsorted_samples(self):
        with self.assertRaisesRegex(ValueError, "within benchmark duration"):
            export_handoff.build_handoff_bundle(self.scenario, self.manifest, [0.0, 120.0])
        with self.assertRaisesRegex(ValueError, "monotonically increasing"):
            export_handoff.build_handoff_bundle(self.scenario, self.manifest, [10.0, 5.0])

    def test_cli_writes_reproducible_json(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            output = Path(temp_dir) / "handoff.json"
            args = [
                "--scenario", str(ROOT / "scenario.json"),
                "--manifest", str(ROOT / "scene_manifest.json"),
                "--output", str(output),
            ]
            self.assertEqual(export_handoff.main(args), 0)
            first = output.read_text(encoding="utf-8")
            self.assertEqual(export_handoff.main(args), 0)
            second = output.read_text(encoding="utf-8")
        self.assertEqual(first, second)
        parsed = json.loads(first)
        self.assertEqual(parsed["handoff_version"], 1)
        self.assertEqual(len(parsed["frames"]), len(export_handoff.DEFAULT_SAMPLE_TIMES))


if __name__ == "__main__":
    unittest.main()
