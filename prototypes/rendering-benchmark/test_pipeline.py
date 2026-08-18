import importlib.util
import json
from pathlib import Path
import sys
import tempfile
import unittest


ROOT = Path(__file__).parent
SCENARIO = json.loads((ROOT / "scenario.json").read_text(encoding="utf-8"))
for module_name in (
    "benchmark",
    "scenario",
    "run_record",
    "normalization",
    "decision_packet",
    "pipeline",
):
    if module_name in sys.modules:
        continue
    module_path = ROOT / f"{module_name}.py"
    spec = importlib.util.spec_from_file_location(module_name, module_path)
    module = importlib.util.module_from_spec(spec)
    sys.modules[module_name] = module
    assert spec.loader is not None
    spec.loader.exec_module(module)

normalization = sys.modules["normalization"]
pipeline = sys.modules["pipeline"]


class RenderingBenchmarkPipelineTests(unittest.TestCase):
    def record(self, engine, cpu=10.0, gpu=12.0, memory=1000.0, hours=20.0, build=500.0):
        return {
            "record_version": 1,
            "engine": engine,
            "scenario_version": SCENARIO["scenario_version"],
            "scenario_name": SCENARIO["name"],
            "captured_at_utc": "2026-08-18T00:00:00Z",
            "capture": {
                "engine_version": "test",
                "os_version": "test",
                "cpu_model": "test",
                "gpu_model": "test",
                "ram_gib": 32,
                "project_settings": {"quality": "benchmark"},
                "cpu_frame_time_ms": cpu,
                "gpu_frame_time_ms": gpu,
                "peak_memory_mib": memory,
                "implementation_hours": hours,
                "build_size_mib": build,
                "screenshots": ["capture.png"],
                "notes": "test fixture",
            },
        }

    def qualitative(self, score):
        return {
            metric: {"score": score, "evidence": f"evidence for {metric}"}
            for metric in normalization.QUALITATIVE_METRICS
        }

    def write_json(self, directory, name, payload):
        path = Path(directory) / name
        path.write_text(json.dumps(payload), encoding="utf-8")
        return path

    def test_build_packet_from_files_produces_decision_ready_result(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            left_record = self.write_json(
                temp_dir,
                "godot.json",
                self.record("godot", cpu=7.0, gpu=8.0, memory=800.0, hours=10.0, build=300.0),
            )
            right_record = self.write_json(
                temp_dir,
                "unreal.json",
                self.record("unreal", cpu=14.0, gpu=16.0, memory=1600.0, hours=20.0, build=600.0),
            )
            left_qualitative = self.write_json(temp_dir, "godot-qual.json", self.qualitative(9.0))
            right_qualitative = self.write_json(temp_dir, "unreal-qual.json", self.qualitative(6.0))

            packet = pipeline.build_packet_from_files(
                ROOT / "scenario.json",
                left_record,
                right_record,
                left_qualitative,
                right_qualitative,
            )

        self.assertEqual(packet["status"], "decision_ready")
        self.assertEqual(packet["recommendation"], "godot")
        self.assertEqual(packet["scenario_name"], SCENARIO["name"])

    def test_missing_qualitative_metric_is_rejected_before_scoring(self):
        assessments = self.qualitative(8.0)
        assessments.pop(next(iter(normalization.QUALITATIVE_METRICS)))
        with tempfile.TemporaryDirectory() as temp_dir:
            path = self.write_json(temp_dir, "incomplete.json", assessments)
            with self.assertRaisesRegex(ValueError, "missing qualitative metrics"):
                pipeline.load_qualitative_assessments(path)

    def test_cli_writes_reproducible_json_packet(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            left_record = self.write_json(temp_dir, "godot.json", self.record("godot"))
            right_record = self.write_json(temp_dir, "unreal.json", self.record("unreal"))
            left_qualitative = self.write_json(temp_dir, "godot-qual.json", self.qualitative(8.0))
            right_qualitative = self.write_json(temp_dir, "unreal-qual.json", self.qualitative(8.0))
            output = Path(temp_dir) / "decision.json"

            exit_code = pipeline.main(
                [
                    "--scenario",
                    str(ROOT / "scenario.json"),
                    "--left-record",
                    str(left_record),
                    "--right-record",
                    str(right_record),
                    "--left-qualitative",
                    str(left_qualitative),
                    "--right-qualitative",
                    str(right_qualitative),
                    "--output",
                    str(output),
                ]
            )
            first = output.read_text(encoding="utf-8")
            pipeline.main(
                [
                    "--scenario",
                    str(ROOT / "scenario.json"),
                    "--left-record",
                    str(left_record),
                    "--right-record",
                    str(right_record),
                    "--left-qualitative",
                    str(left_qualitative),
                    "--right-qualitative",
                    str(right_qualitative),
                    "--output",
                    str(output),
                ]
            )
            second = output.read_text(encoding="utf-8")

        self.assertEqual(exit_code, 0)
        self.assertEqual(first, second)
        self.assertEqual(json.loads(first)["status"], "additional_evidence_required")


if __name__ == "__main__":
    unittest.main()
