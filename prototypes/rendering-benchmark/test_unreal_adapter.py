import json
from pathlib import Path
import unittest


ROOT = Path(__file__).parent
UNREAL = ROOT / "unreal"
ADAPTER = UNREAL / "Source" / "EverwardBenchmark" / "BenchmarkAdapter.cpp"
HEADER = UNREAL / "Source" / "EverwardBenchmark" / "BenchmarkAdapter.h"
PROJECT = UNREAL / "EverwardBenchmark.uproject"
SCENARIO = ROOT / "scenario.json"


class UnrealBenchmarkAdapterTests(unittest.TestCase):
    def test_project_declares_runtime_module(self):
        project = json.loads(PROJECT.read_text(encoding="utf-8"))
        self.assertEqual(project["Modules"][0]["Name"], "EverwardBenchmark")
        self.assertEqual(project["Modules"][0]["Type"], "Runtime")

    def test_adapter_binds_canonical_handoff_identity(self):
        source = HEADER.read_text(encoding="utf-8") + ADAPTER.read_text(encoding="utf-8")
        scenario = json.loads(SCENARIO.read_text(encoding="utf-8"))
        self.assertIn("ExpectedHandoffVersion = 2", source)
        self.assertIn(f'ExpectedScenarioName = TEXT("{scenario["name"]}")', source)
        self.assertIn('TEXT("benchmark_handoff.json")', source)

    def test_adapter_requires_continuous_playback_contract(self):
        source = ADAPTER.read_text(encoding="utf-8")
        for key in (
            "duration_seconds",
            "simulation_seconds_per_real_second",
            "camera_sequence",
            "camera_stage_durations_seconds",
            "objects",
            "cameras",
            "animation_periods_seconds",
            "feature_bindings",
        ):
            self.assertIn(f'TEXT("{key}")', source)

    def test_adapter_preserves_engine_neutral_scene_truth(self):
        source = ADAPTER.read_text(encoding="utf-8")
        self.assertIn('GetObjectField(TEXT("objects"))', source)
        self.assertIn('GetObjectField(TEXT("cameras"))', source)
        self.assertIn('GetObjectField(TEXT("animation_periods_seconds"))', source)
        self.assertNotIn("FMath::Rand", source)
        self.assertNotIn("FRandomStream", source)

    def test_canonical_meter_units_are_converted_explicitly(self):
        source = HEADER.read_text(encoding="utf-8") + ADAPTER.read_text(encoding="utf-8")
        self.assertIn("MetersToCentimeters = 100.0", source)
        self.assertIn("* MetersToCentimeters", source)

    def test_restart_resets_deterministic_playback(self):
        source = ADAPTER.read_text(encoding="utf-8")
        self.assertIn("ElapsedRealSeconds = 0.0", source)
        self.assertIn("ApplyCameraStage(CameraStageAt(0.0))", source)
        self.assertIn("ApplyDeterministicAnimation(0.0)", source)
        self.assertIn("UpdateTelemetry(0.0)", source)

    def test_visual_shell_exercises_required_presentation_systems(self):
        source = HEADER.read_text(encoding="utf-8") + ADAPTER.read_text(encoding="utf-8")
        for token in (
            "UDirectionalLightComponent",
            "UExponentialHeightFogComponent",
            "UInstancedStaticMeshComponent",
            "SetVolumetricFog(true)",
            "DebrisParticleCount = 96",
            "Telemetry->SetText",
        ):
            self.assertIn(token, source)

    def test_visual_shell_assigns_engine_geometry_for_major_scene_objects(self):
        source = ADAPTER.read_text(encoding="utf-8")
        for engine_mesh in (
            "/Engine/BasicShapes/Sphere.Sphere",
            "/Engine/BasicShapes/Cube.Cube",
            "/Engine/BasicShapes/Cylinder.Cylinder",
        ):
            self.assertIn(engine_mesh, source)
        for component in ("Asteroid", "Probe", "MiningArm", "Planet", "DebrisParticles"):
            self.assertIn(f"{component}->SetStaticMesh", source)

    def test_debris_field_is_deterministic_and_rng_free(self):
        source = ADAPTER.read_text(encoding="utf-8")
        self.assertIn("PopulateDeterministicDebris", source)
        self.assertIn("Index * 37", source)
        self.assertIn("Index * 19", source)
        self.assertNotIn("FMath::Rand", source)
        self.assertNotIn("FRandomStream", source)


if __name__ == "__main__":
    unittest.main()
