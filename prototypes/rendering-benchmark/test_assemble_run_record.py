import importlib.util
import json
from pathlib import Path
import sys
import tempfile
import unittest


ROOT = Path(__file__).parent
for module_name in ("scenario", "run_record", "assemble_run_record"):
    if module_name in sys.modules:
        continue
    spec = importlib.util.spec_from_file_location(module_name, ROOT / f"{module_name}.py")
    module = importlib.util.module_from_spec(spec)
    sys.modules[module_name] = module
    assert spec.loader is not None
    spec.loader.exec_module(module)

assembler = sys.modules["assemble_run_record"]
scenario_module = sys.modules["scenario"]
SCENARIO = scenario_module.load_scenario(ROOT / "scenario.json")


class AssembleRunRecordTests(unittest.TestCase):
    def observation(self, engine="godot", include_peak=False):
        prefill = {
            "engine_version": "test-engine",
            "os_version": "test-os",
            "cpu_model": "test-cpu",
            "gpu_model": "test-gpu",
            "ram_gib": 32.0,
            "cpu_frame_time_ms": 8.0,
        }
        manual = [
            "project_settings",
            "gpu_frame_time_ms",
            "implementation_hours",
            "build_size_mib",
            "screenshots",
            "notes",
        ]
        if include_peak:
            prefill["peak_memory_mib"] = 2048.0
        else:
            manual.append("peak_memory_mib")
        return {
            "observation_version": 1,
            "engine": engine,
            "scenario_name": SCENARIO["name"],
            "scenario_version": SCENARIO["scenario_version"],
            "captured_at_utc": "2026-08-18T17:00:00Z",
            "run_record_prefill": prefill,
            "manual_evidence_still_required": manual,
        }

    def manual(self, include_peak=False):
        evidence = {
            "project_settings": {"quality": "benchmark"},
            "gpu_frame_time_ms": 9.0,
            "implementation_hours": 12.5,
            "build_size_mib": 650.0,
            "screenshots": ["wide.png", "medium.png", "close.png"],
            "notes": "Captured under canonical runbook conditions.",
        }
        if not include_peak:
            evidence["peak_memory_mib"] = 1900.0
        return evidence

    def test_godot_manual_peak_memory_completes_valid_record(self):
        record = assembler.assemble_run_record(
            self.observation("godot", include_peak=False),
            self.manual(include_peak=False),
            SCENARIO,
        )
        self.assertEqual(record["engine"], "godot")
        self.assertEqual(record["capture"]["cpu_frame_time_ms"], 8.0)
        self.assertEqual(record["capture"]["peak_memory_mib"], 1900.0)

    def test_unreal_measured_peak_memory_is_preserved(self):
        record = assembler.assemble_run_record(
            self.observation("unreal", include_peak=True),
            self.manual(include_peak=True),
            SCENARIO,
        )
        self.assertEqual(record["capture"]["peak_memory_mib"], 2048.0)

    def test_manual_evidence_cannot_override_measured_prefill(self):
        manual = self.manual(include_peak=True)
        manual["cpu_frame_time_ms"] = 1.0
        with self.assertRaisesRegex(ValueError, "exactly match observation requirements"):
            assembler.assemble_run_record(
                self.observation("unreal", include_peak=True), manual, SCENARIO
            )

    def test_observation_scenario_drift_is_rejected(self):
        observation = self.observation()
        observation["scenario_version"] += 1
        with self.assertRaisesRegex(ValueError, "scenario_version does not match"):
            assembler.assemble_run_record(observation, self.manual(), SCENARIO)

    def test_incomplete_evidence_partition_is_rejected(self):
        observation = self.observation()
        observation["manual_evidence_still_required"].remove("gpu_frame_time_ms")
        with self.assertRaisesRegex(ValueError, "does not cover run-record contract"):
            assembler.validate_observation(observation, SCENARIO)

    def test_cli_output_is_reproducible_and_valid(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            temp = Path(temp_dir)
            observation_path = temp / "observation.json"
            manual_path = temp / "manual.json"
            output_path = temp / "run-record.json"
            observation_path.write_text(json.dumps(self.observation()), encoding="utf-8")
            manual_path.write_text(json.dumps(self.manual()), encoding="utf-8")

            args = [
                "--scenario", str(ROOT / "scenario.json"),
                "--observation", str(observation_path),
                "--manual-evidence", str(manual_path),
                "--output", str(output_path),
            ]
            self.assertEqual(assembler.main(args), 0)
            first = output_path.read_text(encoding="utf-8")
            self.assertEqual(assembler.main(args), 0)
            second = output_path.read_text(encoding="utf-8")

        self.assertEqual(first, second)
        self.assertEqual(json.loads(first)["scenario_name"], SCENARIO["name"])


if __name__ == "__main__":
    unittest.main()
