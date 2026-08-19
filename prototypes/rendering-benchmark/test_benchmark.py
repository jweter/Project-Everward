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

    def test_score_applies_each_metrics_own_declared_weight(self):
        # A uniform metrics vector (see test above) makes score_result collapse to
        # `value * sum(weights)`, which is invariant no matter which weight is
        # paired with which metric: it cannot detect a metric being scored with
        # the wrong DEFAULT_WEIGHTS entry (e.g. two weights swapped between
        # metrics, or a future refactor iterating metrics/weights out of step).
        # This uses thirteen distinct per-metric values, matched against a
        # weighted sum computed independently of benchmark.py (literal weights
        # and values, summed here rather than via DEFAULT_WEIGHTS/score_result's
        # own iteration), so a wrong metric<->weight pairing changes the result.
        metrics = {
            "visual_fidelity": 9.0,
            "cpu_frame_time": 3.0,
            "gpu_frame_time": 6.0,
            "memory_use": 1.0,
            "scene_complexity": 10.0,
            "hud_effort": 4.0,
            "procedural_workflow": 8.0,
            "simulation_integration": 2.0,
            "large_coordinate_behavior": 7.0,
            "save_load_implications": 5.0,
            "build_distribution_complexity": 0.0,
            "developer_iteration_speed": 6.5,
            "commercial_licensing": 3.5,
        }
        self.assertEqual(set(metrics), set(benchmark.REQUIRED_METRICS))

        result = benchmark.BenchmarkResult("Godot", metrics)

        # Independently computed: (metric value * that metric's declared
        # DEFAULT_WEIGHTS weight), summed. Verified by hand and with a standalone
        # script against the current DEFAULT_WEIGHTS values; written as a literal
        # so it does not share computation with score_result().
        expected = (
            9.0 * 0.14  # visual_fidelity
            + 3.0 * 0.08  # cpu_frame_time
            + 6.0 * 0.08  # gpu_frame_time
            + 1.0 * 0.05  # memory_use
            + 10.0 * 0.04  # scene_complexity
            + 4.0 * 0.06  # hud_effort
            + 8.0 * 0.08  # procedural_workflow
            + 2.0 * 0.12  # simulation_integration
            + 7.0 * 0.10  # large_coordinate_behavior
            + 5.0 * 0.05  # save_load_implications
            + 0.0 * 0.05  # build_distribution_complexity
            + 6.5 * 0.10  # developer_iteration_speed
            + 3.5 * 0.05  # commercial_licensing
        )
        self.assertAlmostEqual(expected, 5.325)
        self.assertAlmostEqual(benchmark.score_result(result), expected)

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
