from pathlib import Path
import unittest


ROOT = Path(__file__).parent
SCENE = (ROOT / "godot" / "main.tscn").read_text(encoding="utf-8")
ADAPTER = (ROOT / "godot" / "benchmark_adapter.gd").read_text(encoding="utf-8")
CAPTURE = (ROOT / "godot" / "capture_session.gd").read_text(encoding="utf-8")


class GodotCaptureContractTests(unittest.TestCase):
    def test_scene_wires_capture_session_as_separate_instrumentation_node(self):
        self.assertIn('path="res://capture_session.gd"', SCENE)
        self.assertIn('name="CaptureSession" type="Node"', SCENE)

    def test_capture_restarts_canonical_playback_after_warmup(self):
        self.assertIn("WARMUP_SECONDS := 5.0", CAPTURE)
        self.assertIn('adapter.restart_canonical_playback()', CAPTURE)
        self.assertIn("func restart_canonical_playback()", ADAPTER)
        self.assertIn("elapsed_real_seconds = 0.0", ADAPTER)

    def test_cpu_measurement_uses_engine_monitor_and_preserves_distribution(self):
        self.assertIn("Performance.get_monitor(Performance.TIME_PROCESS)", CAPTURE)
        for statistic in ('"mean"', '"p50"', '"p95"', '"max"'):
            with self.subTest(statistic=statistic):
                self.assertIn(statistic, CAPTURE)

    def test_hardware_metadata_comes_from_runtime_not_hardcoded_fixture(self):
        required_runtime_sources = (
            "Engine.get_version_info()",
            "OS.get_version()",
            "OS.get_processor_name()",
            "OS.get_memory_info()",
            "RenderingServer.get_video_adapter_name()",
        )
        for source in required_runtime_sources:
            with self.subTest(source=source):
                self.assertIn(source, CAPTURE)

    def test_instrumentation_does_not_invent_unavailable_decision_grade_metrics(self):
        self.assertIn('"gpu_frame_time_ms"', CAPTURE)
        self.assertIn('"peak_memory_mib"', CAPTURE)
        self.assertIn('"manual_evidence_still_required"', CAPTURE)
        prefill = CAPTURE.split('"run_record_prefill": {', 1)[1].split('},\n        "manual_evidence_still_required"', 1)[0]
        self.assertNotIn('"gpu_frame_time_ms"', prefill)
        self.assertNotIn('"peak_memory_mib"', prefill)

    def test_capture_output_is_observation_not_validated_run_record(self):
        self.assertIn('OUTPUT_PATH := "user://godot_capture_observation.json"', CAPTURE)
        self.assertIn('"observation_version": 1', CAPTURE)
        self.assertIn('"measurement_notes"', CAPTURE)


if __name__ == "__main__":
    unittest.main()
