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


if __name__ == "__main__":
    unittest.main()
