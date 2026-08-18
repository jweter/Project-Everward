import importlib.util
import json
from pathlib import Path
import sys
import tempfile
import unittest


ROOT = Path(__file__).parent
for module_name in (
    "benchmark",
    "scenario",
    "playback",
    "scene_state",
    "export_handoff",
    "evidence_template",
    "prepare_capture",
    "capture_readiness",
    "prepare_hardware_capture",
):
    if module_name in sys.modules:
        continue
    module_path = ROOT / f"{module_name}.py"
    spec = importlib.util.spec_from_file_location(module_name, module_path)
    module = importlib.util.module_from_spec(spec)
    sys.modules[module_name] = module
    assert spec.loader is not None
    spec.loader.exec_module(module)

prepare_hardware_capture = sys.modules["prepare_hardware_capture"]


class HardwareCapturePreparationTests(unittest.TestCase):
    def test_real_repository_layout_prepares_capture_workspace(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            output_dir = Path(temp_dir) / "capture"
            summary = prepare_hardware_capture.prepare_hardware_capture_workspace(ROOT, output_dir)

            self.assertTrue(summary["ready_for_hardware_capture"])
            self.assertEqual(summary["blockers"], [])
            self.assertTrue((output_dir / "handoff.json").is_file())
            self.assertTrue((output_dir / "godot-run-record.json").is_file())
            self.assertTrue((output_dir / "unreal-run-record.json").is_file())
            self.assertTrue((output_dir / "capture-preparation-summary.json").is_file())

            scenario = json.loads((ROOT / "scenario.json").read_text(encoding="utf-8"))
            handoff = json.loads((output_dir / "handoff.json").read_text(encoding="utf-8"))
            godot = json.loads((output_dir / "godot-run-record.json").read_text(encoding="utf-8"))
            unreal = json.loads((output_dir / "unreal-run-record.json").read_text(encoding="utf-8"))

            for payload in (handoff, godot, unreal):
                self.assertEqual(payload["scenario_name"], scenario["name"])
                self.assertEqual(payload["scenario_version"], scenario["scenario_version"])

    def test_existing_handoff_is_not_overwritten_by_default(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            output_dir = Path(temp_dir) / "capture"
            output_dir.mkdir()
            handoff = output_dir / "handoff.json"
            handoff.write_text('{"preserve": true}\n', encoding="utf-8")

            with self.assertRaisesRegex(FileExistsError, "refusing to overwrite existing handoff"):
                prepare_hardware_capture.prepare_hardware_capture_workspace(ROOT, output_dir)

            self.assertEqual(handoff.read_text(encoding="utf-8"), '{"preserve": true}\n')

    def test_existing_engine_evidence_is_not_overwritten_by_default(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            output_dir = Path(temp_dir) / "capture"
            output_dir.mkdir()
            existing = output_dir / "godot-run-record.json"
            existing.write_text('{"measured": true}\n', encoding="utf-8")

            with self.assertRaisesRegex(FileExistsError, "refusing to overwrite existing evidence file"):
                prepare_hardware_capture.prepare_hardware_capture_workspace(ROOT, output_dir)

            self.assertEqual(existing.read_text(encoding="utf-8"), '{"measured": true}\n')


if __name__ == "__main__":
    unittest.main()
