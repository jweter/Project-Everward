import importlib.util
import json
from pathlib import Path
import sys
import tempfile
import unittest

ROOT = Path(__file__).parent
for module_name in ("scenario", "evidence_template", "prepare_capture"):
    if module_name in sys.modules:
        continue
    module_path = ROOT / f"{module_name}.py"
    spec = importlib.util.spec_from_file_location(module_name, module_path)
    module = importlib.util.module_from_spec(spec)
    sys.modules[module_name] = module
    assert spec.loader is not None
    spec.loader.exec_module(module)

prepare_capture = sys.modules["prepare_capture"]


class RenderingCapturePreparationTests(unittest.TestCase):
    def test_prepares_unreal_template_bound_to_canonical_scenario(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            output_dir = Path(temp_dir) / "capture"
            written = prepare_capture.prepare_capture_files(ROOT / "scenario.json", output_dir)
            self.assertEqual(set(written), {"unreal"})
            scenario = json.loads((ROOT / "scenario.json").read_text(encoding="utf-8"))
            payload = json.loads(written["unreal"].read_text(encoding="utf-8"))
            self.assertEqual(payload["engine"], "unreal")
            self.assertEqual(payload["scenario_name"], scenario["name"])
            self.assertEqual(payload["scenario_version"], scenario["scenario_version"])
            self.assertEqual(payload["captured_at_utc"], "")
            self.assertEqual(payload["capture"]["screenshots"], [])

    def test_refuses_to_overwrite_existing_capture_evidence_by_default(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            output_dir = Path(temp_dir) / "capture"
            prepare_capture.prepare_capture_files(ROOT / "scenario.json", output_dir)
            with self.assertRaisesRegex(FileExistsError, "refusing to overwrite"):
                prepare_capture.prepare_capture_files(ROOT / "scenario.json", output_dir)

    def test_cli_summary_reports_canonical_capture_target(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            output_dir = Path(temp_dir) / "capture"
            written = prepare_capture.prepare_capture_files(ROOT / "scenario.json", output_dir)
            summary = prepare_capture.build_summary(ROOT / "scenario.json", written)
        self.assertEqual(summary["scenario_name"], "icy-asteroid-mining")
        self.assertEqual(summary["target_resolution"], [2560, 1440])
        self.assertEqual(summary["target_fps"], 60)
        self.assertEqual(summary["duration_seconds"], 120)
        self.assertEqual(summary["engine"], "unreal")


if __name__ == "__main__":
    unittest.main()
