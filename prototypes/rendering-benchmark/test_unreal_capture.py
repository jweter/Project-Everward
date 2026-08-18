from pathlib import Path
import unittest


ROOT = Path(__file__).parent
SOURCE = ROOT / "unreal" / "Source" / "EverwardBenchmark"
ADAPTER_CPP = SOURCE / "BenchmarkAdapter.cpp"
CAPTURE_CPP = SOURCE / "BenchmarkCaptureSessionComponent.cpp"
CAPTURE_HEADER = SOURCE / "BenchmarkCaptureSessionComponent.h"


class UnrealBenchmarkCaptureTests(unittest.TestCase):
    def source(self):
        return (
            ADAPTER_CPP.read_text(encoding="utf-8")
            + CAPTURE_HEADER.read_text(encoding="utf-8")
            + CAPTURE_CPP.read_text(encoding="utf-8")
        )

    def test_capture_component_is_attached_to_benchmark_adapter(self):
        source = self.source()
        self.assertIn("UBenchmarkCaptureSessionComponent", source)
        self.assertIn('CreateDefaultSubobject<UBenchmarkCaptureSessionComponent>(TEXT("CaptureSession"))', source)

    def test_capture_uses_canonical_handoff_for_identity_and_duration(self):
        source = CAPTURE_CPP.read_text(encoding="utf-8")
        self.assertIn('TEXT("benchmark_handoff.json")', source)
        self.assertIn('GetStringField(TEXT("scenario_name"))', source)
        self.assertIn('GetIntegerField(TEXT("scenario_version"))', source)
        self.assertIn('GetNumberField(TEXT("duration_seconds"))', source)
        self.assertNotIn('TEXT("icy-asteroid-mining")', source)

    def test_capture_has_warmup_then_restarts_canonical_playback(self):
        source = self.source()
        self.assertIn("WarmupSeconds = 5.0", source)
        self.assertIn("WarmupElapsedSeconds >= WarmupSeconds", source)
        self.assertIn("Adapter->RestartCanonicalPlayback()", source)

    def test_capture_records_engine_native_cpu_and_process_memory_evidence(self):
        source = CAPTURE_CPP.read_text(encoding="utf-8")
        self.assertIn("GGameThreadTime", source)
        self.assertIn("FPlatformTime::ToMilliseconds", source)
        self.assertIn("FPlatformMemory::GetStats()", source)
        self.assertIn("PeakUsedPhysical", source)
        self.assertIn('SetNumberField(TEXT("cpu_frame_time_ms"), P50Ms)', source)
        self.assertIn('SetNumberField(TEXT("peak_memory_mib"), PeakMemoryMiB)', source)

    def test_capture_does_not_fabricate_gpu_measurement(self):
        source = CAPTURE_CPP.read_text(encoding="utf-8")
        self.assertIn('TEXT("gpu_frame_time_ms")', source)
        self.assertIn("GPU frame time remains manual", source)
        self.assertNotIn('SetNumberField(TEXT("gpu_frame_time_ms")', source)

    def test_observation_keeps_remaining_manual_evidence_explicit(self):
        source = CAPTURE_CPP.read_text(encoding="utf-8")
        for field in (
            "project_settings",
            "gpu_frame_time_ms",
            "implementation_hours",
            "build_size_mib",
            "screenshots",
            "notes",
        ):
            self.assertIn(f'TEXT("{field}")', source)
        self.assertIn('TEXT("manual_evidence_still_required")', source)

    def test_capture_writes_observation_outside_source_tree(self):
        source = CAPTURE_CPP.read_text(encoding="utf-8")
        self.assertIn("FPaths::ProjectSavedDir()", source)
        self.assertIn('TEXT("unreal_capture_observation.json")', source)


if __name__ == "__main__":
    unittest.main()
