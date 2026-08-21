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
    def observation(self):
        return {
            "observation_version": 1,
            "engine": "unreal",
            "scenario_name": SCENARIO["name"],
            "scenario_version": SCENARIO["scenario_version"],
            "captured_at_utc": "2026-08-21T02:12:57Z",
            "run_record_prefill": {
                "engine_version": "5.8.1",
                "os_version": "Windows",
                "cpu_model": "i7-13700H",
                "gpu_model": "Intel Iris Xe Graphics",
                "ram_gib": 15.7,
                "cpu_frame_time_ms": 9.35,
                "peak_memory_mib": 4712.0,
            },
            "manual_evidence_still_required": [
                "project_settings",
                "gpu_frame_time_ms",
                "implementation_hours",
                "build_size_mib",
                "screenshots",
                "notes",
            ],
        }

    def manual(self):
        return {
            "project_settings": {"resolution": "2560x1440"},
            "gpu_frame_time_ms": 65.3,
            "implementation_hours": 1.0,
            "build_size_mib": 650.0,
            "screenshots": ["wide.png", "medium.png", "close.png"],
            "notes": "Captured under canonical runbook conditions.",
        }

    def test_unreal_measured_peak_memory_is_preserved(self):
        record = assembler.assemble_run_record(self.observation(), self.manual(), SCENARIO)
        self.assertEqual(record["engine"], "unreal")
        self.assertEqual(record["capture"]["cpu_frame_time_ms"], 9.35)
        self.assertEqual(record["capture"]["peak_memory_mib"], 4712.0)

    def test_non_unreal_observation_is_rejected(self):
        observation = self.observation()
        observation["engine"] = "other"
        with self.assertRaisesRegex(ValueError, "observation engine must be 'unreal'"):
            assembler.validate_observation(observation, SCENARIO)

    def test_manual_evidence_cannot_override_measured_prefill(self):
        manual = self.manual()
        manual["cpu_frame_time_ms"] = 1.0
        with self.assertRaisesRegex(ValueError, "exactly match observation requirements"):
            assembler.assemble_run_record(self.observation(), manual, SCENARIO)

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

    def test_every_declared_observation_field_is_required(self):
        for field in sorted(assembler.REQUIRED_OBSERVATION_FIELDS):
            with self.subTest(field=field):
                observation = self.observation()
                del observation[field]
                with self.assertRaisesRegex(ValueError, "missing observation fields"):
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
        self.assertEqual(json.loads(first)["engine"], "unreal")


if __name__ == "__main__":
    unittest.main()
