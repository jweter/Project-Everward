import json
from pathlib import Path
import sys
import tempfile
import unittest

ROOT = Path(__file__).parent
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

import finalize_engine_decision as finalizer  # noqa: E402
import scenario as scenario_module  # noqa: E402


class RenderingDecisionFinalizerTests(unittest.TestCase):
    def build_record(self):
        scenario = json.loads((ROOT / "scenario.json").read_text(encoding="utf-8"))
        return {
            "record_version": 1,
            "engine": "unreal",
            "scenario_version": scenario["scenario_version"],
            "scenario_name": scenario["name"],
            "captured_at_utc": "2026-08-21T02:12:57Z",
            "capture": {
                "engine_version": "5.8.1",
                "os_version": "Windows",
                "cpu_model": "i7-13700H",
                "gpu_model": "Intel Iris Xe Graphics",
                "ram_gib": 15.7,
                "project_settings": {"resolution": "2560x1440"},
                "cpu_frame_time_ms": 9.35,
                "gpu_frame_time_ms": 65.3,
                "peak_memory_mib": 4712.0,
                "implementation_hours": 1.0,
                "build_size_mib": 1.0,
                "screenshots": ["evidence/unreal/capture.png"],
                "notes": "test fixture",
            },
        }

    def test_finalize_emits_decision_ready_unreal_artifact(self):
        with tempfile.TemporaryDirectory() as tmp:
            record_path = Path(tmp) / "unreal.json"
            record_path.write_text(json.dumps(self.build_record()), encoding="utf-8")
            artifact = finalizer.finalize(ROOT / "scenario.json", record_path)
        self.assertEqual(artifact["artifact_type"], "everward_phase1_engine_decision")
        self.assertEqual(artifact["artifact_version"], 2)
        self.assertEqual(artifact["decision_packet"]["status"], "decision_ready")
        self.assertEqual(artifact["decision_packet"]["recommendation"], "unreal")
        self.assertEqual(artifact["decision_packet"]["engine_version"], "5.8.1")

    def test_incomplete_run_record_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            record = self.build_record()
            del record["capture"]["gpu_frame_time_ms"]
            record_path = Path(tmp) / "unreal.json"
            record_path.write_text(json.dumps(record), encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "invalid capture fields"):
                finalizer.finalize(ROOT / "scenario.json", record_path)

    def test_non_unreal_record_is_rejected_by_run_record_contract(self):
        with tempfile.TemporaryDirectory() as tmp:
            record = self.build_record()
            record["engine"] = "other"
            record_path = Path(tmp) / "other.json"
            record_path.write_text(json.dumps(record), encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "engine must be 'unreal'"):
                finalizer.finalize(ROOT / "scenario.json", record_path)


if __name__ == "__main__":
    unittest.main()
