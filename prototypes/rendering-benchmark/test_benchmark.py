import importlib.util
from pathlib import Path
import sys
import unittest


MODULE_PATH = Path(__file__).with_name("benchmark.py")
spec = importlib.util.spec_from_file_location("rendering_benchmark", MODULE_PATH)
benchmark = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = benchmark
assert spec.loader is not None
spec.loader.exec_module(benchmark)


class RenderingBenchmarkTests(unittest.TestCase):
    def complete_metrics(self, value=8.0):
        return {name: value for name in benchmark.REQUIRED_METRICS}

    def test_default_weights_cover_contract_and_sum_to_one(self):
        self.assertEqual(set(benchmark.REQUIRED_METRICS), set(benchmark.DEFAULT_WEIGHTS))
        self.assertAlmostEqual(sum(benchmark.DEFAULT_WEIGHTS.values()), 1.0)

    def test_score_accepts_complete_normalized_evidence(self):
        result = benchmark.BenchmarkResult("Godot", self.complete_metrics(8.0))
        self.assertAlmostEqual(benchmark.score_result(result), 8.0)

    def test_missing_metric_is_rejected(self):
        metrics = self.complete_metrics()
        metrics.pop("large_coordinate_behavior")
        with self.assertRaisesRegex(ValueError, "missing benchmark metrics"):
            benchmark.validate_result(benchmark.BenchmarkResult("Unreal", metrics))

    def test_out_of_range_metric_is_rejected(self):
        metrics = self.complete_metrics()
        metrics["visual_fidelity"] = 10.1
        with self.assertRaisesRegex(ValueError, "0..10"):
            benchmark.validate_result(benchmark.BenchmarkResult("Unreal", metrics))

    def test_close_results_do_not_force_engine_decision(self):
        godot = benchmark.BenchmarkResult("Godot", self.complete_metrics(8.0))
        unreal = benchmark.BenchmarkResult("Unreal", self.complete_metrics(8.1))
        comparison = benchmark.compare_results(godot, unreal)
        self.assertEqual(comparison["outcome"], "too_close_to_call")

    def test_material_score_difference_reports_higher_engine(self):
        godot = benchmark.BenchmarkResult("Godot", self.complete_metrics(7.0))
        unreal = benchmark.BenchmarkResult("Unreal", self.complete_metrics(9.0))
        comparison = benchmark.compare_results(godot, unreal)
        self.assertEqual(comparison["outcome"], "Unreal")


if __name__ == "__main__":
    unittest.main()
