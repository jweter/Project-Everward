import importlib.util
from pathlib import Path
import sys
import unittest


ROOT = Path(__file__).parent
for module_name in ("benchmark", "scenario", "run_record", "normalization"):
    if module_name in sys.modules:
        continue
    module_path = ROOT / f"{module_name}.py"
    spec = importlib.util.spec_from_file_location(module_name, module_path)
    module = importlib.util.module_from_spec(spec)
    sys.modules[module_name] = module
    assert spec.loader is not None
    spec.loader.exec_module(module)

normalization = sys.modules["normalization"]


class RenderingNormalizationTests(unittest.TestCase):
    def scenario(self):
        return {
            "scenario_version": 1,
            "name": "icy_asteroid_mining_v1",
        }

    def run_record(self, engine, cpu=10.0, gpu=12.0, memory=1000.0, hours=20.0, build=500.0):
        return {
            "record_version": 1,
            "engine": engine,
            "scenario_version": 1,
            "scenario_name": "icy_asteroid_mining_v1",
            "captured_at_utc": "2026-08-17T20:00:00Z",
            "capture": {
                "engine_version": "test",
                "os_version": "test",
                "cpu_model": "test",
                "gpu_model": "test",
                "ram_gib": 32,
                "cpu_frame_time_ms": cpu,
                "gpu_frame_time_ms": gpu,
                "peak_memory_mib": memory,
                "implementation_hours": hours,
                "build_size_mib": build,
                "screenshots": ["capture.png"],
                "project_settings": {"quality": "benchmark"},
                "notes": "",
            },
        }

    def assessments(self, score=8.0):
        return {
            metric: normalization.QualitativeAssessment(score, f"evidence for {metric}")
            for metric in normalization.QUALITATIVE_METRICS
        }

    def test_contract_partitions_every_benchmark_metric_once(self):
        benchmark = sys.modules["benchmark"]
        quantitative = set(normalization.QUANTITATIVE_SOURCES)
        qualitative = set(normalization.QUALITATIVE_METRICS)
        self.assertFalse(quantitative & qualitative)
        self.assertEqual(set(benchmark.REQUIRED_METRICS), quantitative | qualitative)

    def test_lower_measurement_scores_ten_and_peer_scores_proportionally(self):
        self.assertEqual(normalization.normalize_lower_is_better(5.0, 10.0), 10.0)
        self.assertEqual(normalization.normalize_lower_is_better(10.0, 5.0), 5.0)

    def test_equal_measurements_tie_at_ten(self):
        self.assertEqual(normalization.normalize_lower_is_better(7.0, 7.0), 10.0)

    def test_missing_qualitative_evidence_is_rejected(self):
        assessments = self.assessments()
        metric = normalization.QUALITATIVE_METRICS[0]
        assessments[metric] = normalization.QualitativeAssessment(8.0, "")
        with self.assertRaisesRegex(ValueError, "requires non-empty evidence"):
            normalization.validate_qualitative_assessments(assessments)

    def test_pair_normalization_derives_objective_scores_and_keeps_audit(self):
        left = self.run_record("godot", cpu=8.0, gpu=10.0, memory=900.0, hours=12.0, build=400.0)
        right = self.run_record("unreal", cpu=10.0, gpu=20.0, memory=1800.0, hours=24.0, build=800.0)

        left_result, right_result, audit = normalization.normalize_pair(
            self.scenario(), left, right, self.assessments(8.0), self.assessments(7.0)
        )

        self.assertEqual(left_result.metrics["cpu_frame_time"], 10.0)
        self.assertEqual(right_result.metrics["cpu_frame_time"], 8.0)
        self.assertEqual(left_result.metrics["gpu_frame_time"], 10.0)
        self.assertEqual(right_result.metrics["gpu_frame_time"], 5.0)
        self.assertEqual(audit["quantitative"]["gpu_frame_time"]["capture_field"], "gpu_frame_time_ms")
        first_qualitative = normalization.QUALITATIVE_METRICS[0]
        self.assertIn("evidence", audit["qualitative"][first_qualitative]["left_evidence"])

    def test_same_engine_comparison_is_rejected(self):
        left = self.run_record("godot")
        right = self.run_record("godot")
        with self.assertRaisesRegex(ValueError, "two different engines"):
            normalization.normalize_pair(
                self.scenario(), left, right, self.assessments(), self.assessments()
            )


if __name__ == "__main__":
    unittest.main()
