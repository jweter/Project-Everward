import importlib.util
from pathlib import Path
import sys
import unittest


ROOT = Path(__file__).parent
for module_name in ("scenario", "run_record", "capture_pair"):
    if module_name in sys.modules:
        continue
    module_path = ROOT / f"{module_name}.py"
    spec = importlib.util.spec_from_file_location(module_name, module_path)
    module = importlib.util.module_from_spec(spec)
    sys.modules[module_name] = module
    assert spec.loader is not None
    spec.loader.exec_module(module)

capture_pair = sys.modules["capture_pair"]


class CapturePairTests(unittest.TestCase):
    def scenario(self):
        return {"scenario_version": 2, "name": "icy_asteroid_mining_v1"}

    def record(self, engine, **capture_overrides):
        capture = {
            "engine_version": "test",
            "os_version": "Windows 11",
            "cpu_model": "CPU",
            "gpu_model": "GPU",
            "ram_gib": 32,
            "project_settings": {"quality": "benchmark"},
            "cpu_frame_time_ms": 10.0,
            "gpu_frame_time_ms": 12.0,
            "peak_memory_mib": 1000.0,
            "implementation_hours": 20.0,
            "build_size_mib": 500.0,
            "screenshots": ["capture.png"],
            "notes": "",
        }
        capture.update(capture_overrides)
        return {
            "record_version": 1,
            "engine": engine,
            "scenario_version": 2,
            "scenario_name": "icy_asteroid_mining_v1",
            "captured_at_utc": "2026-08-18T20:00:00Z",
            "capture": capture,
        }

    def provenance(self, revision="abc123", handoff="deadbeef"):
        return {"source_revision": revision, "handoff_sha256": handoff}

    def test_matching_pair_is_comparable(self):
        result = capture_pair.validate_capture_pair(
            self.scenario(),
            self.record("godot"),
            self.record("unreal"),
            self.provenance(),
            self.provenance(),
        )
        self.assertEqual(result["status"], "comparable")
        self.assertEqual(result["source_revision"], "abc123")
        self.assertEqual(result["hardware"]["gpu_model"], "GPU")

    def test_source_revision_mismatch_is_rejected(self):
        with self.assertRaisesRegex(ValueError, "source revisions do not match"):
            capture_pair.validate_capture_pair(
                self.scenario(),
                self.record("godot"),
                self.record("unreal"),
                self.provenance(revision="left"),
                self.provenance(revision="right"),
            )

    def test_handoff_hash_mismatch_is_rejected(self):
        with self.assertRaisesRegex(ValueError, "handoff hashes do not match"):
            capture_pair.validate_capture_pair(
                self.scenario(),
                self.record("godot"),
                self.record("unreal"),
                self.provenance(handoff="left"),
                self.provenance(handoff="right"),
            )

    def test_hardware_mismatch_is_rejected(self):
        with self.assertRaisesRegex(ValueError, "benchmark hardware does not match: gpu_model"):
            capture_pair.validate_capture_pair(
                self.scenario(),
                self.record("godot"),
                self.record("unreal", gpu_model="Different GPU"),
                self.provenance(),
                self.provenance(),
            )

    def test_incomplete_provenance_is_rejected(self):
        with self.assertRaisesRegex(ValueError, "invalid left provenance fields"):
            capture_pair.validate_capture_pair(
                self.scenario(),
                self.record("godot"),
                self.record("unreal"),
                {"source_revision": "abc123"},
                self.provenance(),
            )


if __name__ == "__main__":
    unittest.main()
