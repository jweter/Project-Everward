from __future__ import annotations

import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
UNREAL = ROOT / "unreal"
SOURCE = UNREAL / "Source" / "Everward"


class Phase2FirstRunEnvironmentTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.environment_h = (SOURCE / "EverwardPhase2TestEnvironment.h").read_text(encoding="utf-8")
        cls.environment_cpp = (SOURCE / "EverwardPhase2TestEnvironment.cpp").read_text(encoding="utf-8")
        cls.game_mode_h = (SOURCE / "EverwardGameMode.h").read_text(encoding="utf-8")
        cls.game_mode_cpp = (SOURCE / "EverwardGameMode.cpp").read_text(encoding="utf-8")
        cls.controller = (SOURCE / "EverwardPlayerController.cpp").read_text(encoding="utf-8")
        cls.pawn_h = (SOURCE / "EverwardProbePawn.h").read_text(encoding="utf-8")
        cls.pawn_cpp = (SOURCE / "EverwardProbePawn.cpp").read_text(encoding="utf-8")
        cls.input_config = (UNREAL / "Config" / "DefaultInput.ini").read_text(encoding="utf-8")

    def test_environment_has_one_visible_bootstrap_scan_target(self) -> None:
        self.assertIn('BootstrapScanTargetId = TEXT("phase2-test-target-001")', self.environment_h)
        self.assertIn('CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BootstrapScanTarget"))', self.environment_cpp)
        self.assertIn('FVector(5000.0, 0.0, 0.0)', self.environment_cpp)
        self.assertIn('PHASE-2 TARGET // SCAN-001', self.environment_cpp)
        self.assertIn('UPointLightComponent', self.environment_cpp)

    def test_environment_has_spatial_references_for_motion_perception(self) -> None:
        self.assertIn('SpatialReference_%02d', self.environment_cpp)
        self.assertGreaterEqual(self.environment_cpp.count('FVector('), 7)

    def test_game_mode_creates_environment_and_deterministic_player_start(self) -> None:
        self.assertIn('virtual void InitGame', self.game_mode_h)
        self.assertIn('ChoosePlayerStart_Implementation', self.game_mode_h)
        self.assertIn('SpawnActor<APlayerStart>', self.game_mode_cpp)
        self.assertIn('SpawnActor<AEverwardPhase2TestEnvironment>', self.game_mode_cpp)
        self.assertIn('return Phase2PlayerStart;', self.game_mode_cpp)

    def test_scan_command_uses_the_visible_environment_target_id(self) -> None:
        self.assertIn('#include "EverwardPhase2TestEnvironment.h"', self.controller)
        self.assertIn('AEverwardPhase2TestEnvironment::BootstrapScanTargetId', self.controller)
        self.assertNotIn('phase2-bootstrap-target', self.controller)

    def test_camera_orbit_and_zoom_are_configured(self) -> None:
        self.assertIn('AxisName="EverwardLookYaw"', self.input_config)
        self.assertIn('AxisName="EverwardLookPitch"', self.input_config)
        self.assertIn('AxisName="EverwardCameraZoom"', self.input_config)
        self.assertIn('BindAxis(TEXT("EverwardLookYaw")', self.controller)
        self.assertIn('BindAxis(TEXT("EverwardLookPitch")', self.controller)
        self.assertIn('BindAxis(TEXT("EverwardCameraZoom")', self.controller)
        self.assertIn('void AdjustCameraZoom(float DeltaCentimeters);', self.pawn_h)
        self.assertIn('CameraBoom->bUsePawnControlRotation = true;', self.pawn_cpp)
        self.assertIn('CameraBoom->bDoCollisionTest = false;', self.pawn_cpp)

    def test_phase2_translation_exposes_all_three_world_axes(self) -> None:
        for key in ('EKeys::W', 'EKeys::S', 'EKeys::A', 'EKeys::D', 'EKeys::Q', 'EKeys::E'):
            self.assertIn(key, self.controller)
        self.assertIn('FVector(VelocityAdjustmentMetersPerSecond, 0.0, 0.0)', self.controller)
        self.assertIn('FVector(0.0, VelocityAdjustmentMetersPerSecond, 0.0)', self.controller)
        self.assertIn('FVector(0.0, 0.0, VelocityAdjustmentMetersPerSecond)', self.controller)
        self.assertIn('CommandSetVelocityMetersPerSecond(RequestedVelocity)', self.controller)

    def test_test_environment_never_bypasses_simulation_adapter_boundary(self) -> None:
        self.assertNotIn('everward::simulation', self.environment_h)
        self.assertNotIn('everward::simulation', self.environment_cpp)
        self.assertNotIn('SimulationCore', self.environment_h)
        self.assertNotIn('SimulationCore', self.environment_cpp)


if __name__ == "__main__":
    unittest.main()
