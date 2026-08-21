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
        handoff = {"handoff_version": 2, "scenario_name": "icy-asteroid-mining", "scenario_version": 2}
        (root / "handoff.json").write_text(json.dumps(handoff), encoding="utf-8")
        return root

    def test_required_engine_paths_match_real_repository_layout(self):
        self.assertEqual(set(MODULE.ENGINE_REQUIRED_FILES), {"unreal"})
        for relative in MODULE.ENGINE_REQUIRED_FILES["unreal"]:
            self.assertTrue((ROOT / relative).is_file(), f"stale readiness path: unreal: {relative}")

    def test_complete_repository_is_ready_for_hardware_capture(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            report = MODULE.audit_capture_readiness(self.build_fixture(Path(temp_dir)))
        self.assertTrue(report["ready_for_hardware_capture"])
        self.assertEqual(report["blockers"], [])
        self.assertEqual(report["engine"], "unreal")
        self.assertTrue(report["engines"]["unreal"]["source_ready"])

    def test_missing_engine_source_blocks_capture(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            root = self.build_fixture(Path(temp_dir))
            missing = MODULE.ENGINE_REQUIRED_FILES["unreal"][0]
            (root / missing).unlink()
            report = MODULE.audit_capture_readiness(root)
        self.assertFalse(report["ready_for_hardware_capture"])
        self.assertIn(missing, " ".join(report["blockers"]))

    def test_every_declared_engine_required_file_missing_blocks_capture(self):
        for relative in MODULE.ENGINE_REQUIRED_FILES["unreal"]:
            with self.subTest(relative=relative):
                with tempfile.TemporaryDirectory() as temp_dir:
                    root = self.build_fixture(Path(temp_dir))
                    (root / relative).unlink()
                    report = MODULE.audit_capture_readiness(root)
                self.assertFalse(report["ready_for_hardware_capture"])
                self.assertEqual(report["engines"]["unreal"]["missing_files"], [relative])

    def test_every_declared_pipeline_file_missing_blocks_capture(self):
        for relative in MODULE.PIPELINE_REQUIRED_FILES:
            with self.subTest(relative=relative):
                with tempfile.TemporaryDirectory() as temp_dir:
                    root = self.build_fixture(Path(temp_dir))
                    (root / relative).unlink()
                    report = MODULE.audit_capture_readiness(root)
                self.assertFalse(report["ready_for_hardware_capture"])
                self.assertIn(f"missing pipeline file: {relative}", report["blockers"])

    def test_missing_handoff_blocks_capture_without_inventing_evidence(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            root = self.build_fixture(Path(temp_dir))
            (root / "handoff.json").unlink()
            report = MODULE.audit_capture_readiness(root)
        self.assertFalse(report["ready_for_hardware_capture"])
        self.assertIn("gpu_frame_time_ms", report["manual_evidence_after_capture"]["unreal"])

    def test_stale_handoff_scenario_identity_is_rejected(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            root = self.build_fixture(Path(temp_dir))
            (root / "handoff.json").write_text(json.dumps({"handoff_version": 2, "scenario_name": "old-scene", "scenario_version": 1}), encoding="utf-8")
            report = MODULE.audit_capture_readiness(root)
        self.assertFalse(report["ready_for_hardware_capture"])

    def test_unreal_does_not_request_peak_memory_manual_measurement(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            report = MODULE.audit_capture_readiness(self.build_fixture(Path(temp_dir)))
        self.assertNotIn("peak_memory_mib", report["manual_evidence_after_capture"]["unreal"])


if __name__ == "__main__":
    unittest.main()
