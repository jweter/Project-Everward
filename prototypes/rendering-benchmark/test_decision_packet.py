import importlib.util
from pathlib import Path
import sys
import unittest


ROOT = Path(__file__).parent
for module_name in ("benchmark", "scenario", "run_record", "normalization", "decision_packet"):
    if module_name in sys.modules:
        continue
    module_path = ROOT / f"{module_name}.py"
    spec = importlib.util.spec_from_file_location(module_name, module_path)
    module = importlib.util.module_from_spec(spec)
    sys.modules[module_name] = module
    assert spec.loader is not None
    spec.loader.exec_module(module)

benchmark = sys.modules["benchmark"]
normalization = sys.modules["normalization"]
decision_packet = sys.modules["decision_packet"]


class RenderingDecisionPacketTests(unittest.TestCase):
    def scenario(self):
        return {"scenario_version": 1, "name": "icy_asteroid_mining_v1"}

    def record(self, engine, cpu=10.0, gpu=12.0, memory=1000.0, hours=20.0, build=500.0):
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

    def assessments(self, score):
        return {
            metric: normalization.QualitativeAssessment(score, f"evidence for {metric}")
            for metric in normalization.QUALITATIVE_METRICS
        }

    def test_clear_lead_is_marked_decision_ready(self):
        left = self.record("godot", cpu=7.0, gpu=8.0, memory=800.0, hours=10.0, build=300.0)
        right = self.record("unreal", cpu=14.0, gpu=16.0, memory=1600.0, hours=20.0, build=600.0)
        packet = decision_packet.build_decision_packet(
            self.scenario(), left, right, self.assessments(9.0), self.assessments(6.0)
        )

        self.assertEqual(packet["status"], "decision_ready")
        self.assertEqual(packet["recommendation"], "godot")
        self.assertEqual(packet["decision_log_fields"]["recommended_engine"], "godot")
        self.assertEqual(len(packet["top_differentiators"]), 5)

    def test_near_tie_requires_more_evidence_instead_of_forcing_choice(self):
        left = self.record("godot")
        right = self.record("unreal")
        packet = decision_packet.build_decision_packet(
            self.scenario(), left, right, self.assessments(8.0), self.assessments(8.0)
        )

        self.assertEqual(packet["comparison"]["outcome"], "too_close_to_call")
        self.assertEqual(packet["status"], "additional_evidence_required")
        self.assertIsNone(packet["recommendation"])

    def test_differentiators_are_sorted_by_weighted_effect(self):
        left = self.record("godot")
        right = self.record("unreal")
        left_assessments = self.assessments(8.0)
        right_assessments = self.assessments(8.0)
        left_assessments["visual_fidelity"] = normalization.QualitativeAssessment(10.0, "A/B screenshots")
        right_assessments["visual_fidelity"] = normalization.QualitativeAssessment(0.0, "A/B screenshots")

        packet = decision_packet.build_decision_packet(
            self.scenario(), left, right, left_assessments, right_assessments
        )

        self.assertEqual(packet["all_metric_deltas"][0]["metric"], "visual_fidelity")
        magnitudes = [row["absolute_weighted_delta"] for row in packet["all_metric_deltas"]]
        self.assertEqual(magnitudes, sorted(magnitudes, reverse=True))

    def test_metric_deltas_apply_each_metrics_own_declared_weight(self):
        # Every existing packet test above leaves at most one metric different
        # between left and right (the rest tie at an identical uniform score),
        # so a weighted_delta of 0 for every tied metric hides which
        # DEFAULT_WEIGHTS entry (if any) got paired with it. That means none of
        # them could detect decision_packet._metric_deltas() applying the wrong
        # metric's weight (e.g. two weights swapped, or a future refactor
        # iterating REQUIRED_METRICS/DEFAULT_WEIGHTS out of step) for any metric
        # that ties. This gives every one of the thirteen required metrics a
        # distinct, non-tied left/right delta, then checks each row's
        # "weighted_delta" against a value computed independently of
        # decision_packet.py: literal DEFAULT_WEIGHTS values (verified by hand
        # against benchmark.py) multiplied by the delta between the left/right
        # scores that normalize_pair() independently produces for that metric.
        left = self.record("godot", cpu=6.0, gpu=9.0, memory=1200.0, hours=8.0, build=250.0)
        right = self.record("unreal", cpu=9.0, gpu=6.0, memory=800.0, hours=16.0, build=500.0)

        def assessment(score):
            return normalization.QualitativeAssessment(score, "evidence")

        left_assessments = {
            "visual_fidelity": assessment(9.0),
            "scene_complexity": assessment(3.0),
            "hud_effort": assessment(7.0),
            "procedural_workflow": assessment(5.0),
            "simulation_integration": assessment(8.0),
            "large_coordinate_behavior": assessment(2.0),
            "save_load_implications": assessment(6.0),
            "commercial_licensing": assessment(4.0),
        }
        right_assessments = {
            "visual_fidelity": assessment(4.0),
            "scene_complexity": assessment(8.0),
            "hud_effort": assessment(1.0),
            "procedural_workflow": assessment(9.0),
            "simulation_integration": assessment(3.0),
            "large_coordinate_behavior": assessment(7.0),
            "save_load_implications": assessment(0.0),
            "commercial_licensing": assessment(10.0),
        }
        self.assertEqual(set(left_assessments), set(normalization.QUALITATIVE_METRICS))
        self.assertEqual(set(right_assessments), set(normalization.QUALITATIVE_METRICS))

        packet = decision_packet.build_decision_packet(
            self.scenario(), left, right, left_assessments, right_assessments
        )

        # normalize_pair() is the same primitive build_decision_packet() itself
        # calls to derive left/right per-metric scores, so re-deriving expected
        # left/right values through it (rather than through _metric_deltas())
        # still isolates the thing under test: whether _metric_deltas() then
        # pairs each metric's score delta with that metric's own declared
        # weight, not a swapped or mismatched one.
        left_result, right_result, _ = normalization.normalize_pair(
            self.scenario(), left, right, left_assessments, right_assessments
        )

        # Literal weights copied from benchmark.DEFAULT_WEIGHTS, independent of
        # decision_packet._metric_deltas()'s own dict lookup.
        literal_weights = {
            "visual_fidelity": 0.14,
            "cpu_frame_time": 0.08,
            "gpu_frame_time": 0.08,
            "memory_use": 0.05,
            "scene_complexity": 0.04,
            "hud_effort": 0.06,
            "procedural_workflow": 0.08,
            "simulation_integration": 0.12,
            "large_coordinate_behavior": 0.10,
            "save_load_implications": 0.05,
            "build_distribution_complexity": 0.05,
            "developer_iteration_speed": 0.10,
            "commercial_licensing": 0.05,
        }
        self.assertEqual(literal_weights, benchmark.DEFAULT_WEIGHTS)

        rows_by_metric = {row["metric"]: row for row in packet["all_metric_deltas"]}
        self.assertEqual(set(rows_by_metric), set(benchmark.REQUIRED_METRICS))

        for metric in benchmark.REQUIRED_METRICS:
            left_score = left_result.metrics[metric]
            right_score = right_result.metrics[metric]
            self.assertNotEqual(left_score, right_score, metric)
            expected_weighted_delta = (left_score - right_score) * literal_weights[metric]
            row = rows_by_metric[metric]
            self.assertEqual(row["weight"], literal_weights[metric], metric)
            self.assertAlmostEqual(row["weighted_delta"], round(expected_weighted_delta, 4), msg=metric)


if __name__ == "__main__":
    unittest.main()
