import importlib.util
import json
from pathlib import Path
import sys
import tempfile
import unittest


ROOT = Path(__file__).parent
MODULE_PATH = ROOT / "capture_readiness.py"
SPEC = importlib.util.spec_from_file_location("capture_readiness", MODULE_PATH)
MODULE = importlib.util.module_from_spec(SPEC)
sys.modules["capture_readiness"] = MODULE
assert SPEC.loader is not None
SPEC.loader.exec_module(MODULE)


class CaptureReadinessTests(unittest.TestCase):
    def build_fixture(self, directory: Path) -> Path:
        root = directory / "rendering-benchmark"
        root.mkdir()
        scenario = {"scenario_version": 2, "name": "icy-asteroid-mining"}
        (root / "scenario.json").write_text(json.dumps(scenario), encoding="utf-8")
        for relative in MODULE.PIPELINE_REQUIRED_FILES:
            path = root / relative
            if path.exists():
                continue
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text("# fixture\n", encoding="utf-8")
        for required in MODULE.ENGINE_REQUIRED_FILES.values():
            for relative in required:
                path = root / relative
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text("fixture\n", encoding="utf-8")
        handoff = {
            "handoff_version": 2,
            "scenario_name": "icy-asteroid-mining",
            "scenario_version": 2,
        }
        (root / "handoff.json").write_text(json.dumps(handoff), encoding="utf-8")
        return root

    def test_required_engine_paths_match_real_repository_layout(self):
        for engine, required in MODULE.ENGINE_REQUIRED_FILES.items():
            for relative in required:
                with self.subTest(engine=engine, relative=relative):
                    self.assertTrue((ROOT / relative).is_file(), f"stale readiness path: {engine}: {relative}")

    def test_complete_repository_is_ready_for_hardware_capture(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            root = self.build_fixture(Path(temp_dir))
            report = MODULE.audit_capture_readiness(root)
        self.assertTrue(report["ready_for_hardware_capture"])
        self.assertEqual(report["blockers"], [])
        self.assertTrue(report["engines"]["godot"]["source_ready"])
        self.assertTrue(report["engines"]["unreal"]["source_ready"])

    def test_missing_engine_source_blocks_capture(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            root = self.build_fixture(Path(temp_dir))
            missing = root / MODULE.ENGINE_REQUIRED_FILES["godot"][0]
            missing.unlink()
            report = MODULE.audit_capture_readiness(root)
        self.assertFalse(report["ready_for_hardware_capture"])
        self.assertIn(str(MODULE.ENGINE_REQUIRED_FILES["godot"][0]), " ".join(report["blockers"]))

    def test_missing_handoff_blocks_capture_without_inventing_evidence(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            root = self.build_fixture(Path(temp_dir))
            (root / "handoff.json").unlink()
            report = MODULE.audit_capture_readiness(root)
        self.assertFalse(report["ready_for_hardware_capture"])
        self.assertTrue(any("export_handoff.py" in blocker for blocker in report["blockers"]))
        self.assertIn("gpu_frame_time_ms", report["manual_evidence_after_capture"]["godot"])

    def test_stale_handoff_scenario_identity_is_rejected(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            root = self.build_fixture(Path(temp_dir))
            stale = {
                "handoff_version": 2,
                "scenario_name": "old-scene",
                "scenario_version": 1,
            }
            (root / "handoff.json").write_text(json.dumps(stale), encoding="utf-8")
            report = MODULE.audit_capture_readiness(root)
        self.assertFalse(report["ready_for_hardware_capture"])
        self.assertIn("handoff scenario identity does not match canonical scenario.json", report["blockers"])

    def test_unreal_does_not_request_peak_memory_manual_measurement(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            root = self.build_fixture(Path(temp_dir))
            report = MODULE.audit_capture_readiness(root)
        self.assertNotIn("peak_memory_mib", report["manual_evidence_after_capture"]["unreal"])
        self.assertIn("peak_memory_mib", report["manual_evidence_after_capture"]["godot"])


if __name__ == "__main__":
    unittest.main()
