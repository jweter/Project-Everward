from __future__ import annotations

import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
UNREAL = ROOT / "unreal"
SOURCE = UNREAL / "Source" / "Everward"
SIM_TYPES = ROOT / "src" / "simulation" / "include" / "everward" / "simulation" / "types.hpp"


class Phase2FirstRunEnvironmentTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.environment_h = (SOURCE / "EverwardPhase2TestEnvironment.h").read_text(encoding="utf-8")
        cls.environment_cpp = (SOURCE / "EverwardPhase2TestEnvironment.cpp").read_text(encoding="utf-8")
        cls.game_mode_h = (SOURCE / "EverwardGameMode.h").read_text(encoding="utf-8")
        cls.game_mode_cpp = (SOURCE / "EverwardGameMode.cpp").read_text(encoding="utf-8")
        cls.controller = (SOURCE / "EverwardPlayerController.cpp").read_text(encoding="utf-8")
        cls.adapter_source = (SOURCE / "ProbeSimulationAdapter.cpp").read_text(encoding="utf-8")
        cls.pawn_h = (SOURCE / "EverwardProbePawn.h").read_text(encoding="utf-8")
        cls.pawn_cpp = (SOURCE / "EverwardProbePawn.cpp").read_text(encoding="utf-8")
        cls.sim_types = SIM_TYPES.read_text(encoding="utf-8")
        cls.input_config = (UNREAL / "Config" / "DefaultInput.ini").read_text(encoding="utf-8")

    def test_environment_has_one_visible_bootstrap_scan_target(self) -> None:
        self.assertIn('BootstrapScanTargetId = TEXT("phase2-test-target-001")', self.environment_h)
        self.assertIn('BootstrapBodyCenterXMeters = 50.0', self.environment_h)
        self.assertIn('BootstrapBodyRadiusMeters = 2.0', self.environment_h)
        self.assertIn('CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BootstrapScanTarget"))', self.environment_cpp)
        self.assertIn('BootstrapBodyCenterXMeters * 100.0', self.environment_cpp)
        self.assertIn('SCAN-001 // UNSURVEYED RESOURCE BODY', self.environment_cpp)
        self.assertIn('SCAN COMPLETE // %s', self.environment_cpp)
        self.assertIn('SetCollisionEnabled(ECollisionEnabled::QueryOnly)', self.environment_cpp)
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

    def test_camera_orbit_and_zoom_are_configured_for_prime_body_scale(self) -> None:
        self.assertIn('AxisName="EverwardLookYaw"', self.input_config)
        self.assertIn('AxisName="EverwardLookPitch"', self.input_config)
        self.assertIn('AxisName="EverwardCameraZoom"', self.input_config)
        self.assertIn('BindAxis(TEXT("EverwardLookYaw")', self.controller)
        self.assertIn('BindAxis(TEXT("EverwardLookPitch")', self.controller)
        self.assertIn('BindAxis(TEXT("EverwardCameraZoom")', self.controller)
        self.assertIn('void AdjustCameraZoom(float DeltaCentimeters);', self.pawn_h)
        self.assertIn('CameraBoom->TargetArmLength = 2200.0f;', self.pawn_cpp)
        self.assertIn('MinCameraDistanceCentimeters = 1200.0f', self.pawn_h)
        self.assertIn('MaxCameraDistanceCentimeters = 5000.0f', self.pawn_h)
        self.assertIn('CameraBoom->bUsePawnControlRotation = true;', self.pawn_cpp)
        self.assertIn('CameraBoom->bDoCollisionTest = false;', self.pawn_cpp)

    def test_phase2_translation_is_probe_relative_on_all_three_local_axes(self) -> None:
        for key in ('EKeys::W', 'EKeys::S', 'EKeys::A', 'EKeys::D', 'EKeys::Q', 'EKeys::E'):
            self.assertIn(key, self.controller)
        self.assertIn('FVector(VelocityAdjustmentMetersPerSecond, 0.0, 0.0)', self.controller)
        self.assertIn('FVector(0.0, VelocityAdjustmentMetersPerSecond, 0.0)', self.controller)
        self.assertIn('FVector(0.0, 0.0, VelocityAdjustmentMetersPerSecond)', self.controller)
        self.assertIn('CommandAdjustLocalVelocityMetersPerSecond(DeltaLocalVelocity)', self.controller)

    def test_phase2_attitude_controls_cover_yaw_pitch_and_roll(self) -> None:
        for key in ('EKeys::J', 'EKeys::L', 'EKeys::I', 'EKeys::K', 'EKeys::U', 'EKeys::O'):
            self.assertIn(key, self.controller)
        self.assertIn('CommandAdjustAttitudeDegrees(DeltaAttitude)', self.controller)
        self.assertIn('Owner->SetActorRotation(PresentationAttitude', self.adapter_source)

    def test_prime_gen1_blockout_has_required_visible_systems(self) -> None:
        direct_components = (
            'ProbeMesh',
            'CoreHousing',
            'ReactorHousing',
            'MainEngine',
            'ForwardSensor',
            'PortRadiator',
            'StarboardRadiator',
            'PortManeuverPod',
            'StarboardManeuverPod',
            'DorsalMarker',
        )
        for component in direct_components:
            self.assertIn(component, self.pawn_h)
            self.assertIn(f'{component} = CreateDefaultSubobject', self.pawn_cpp)

        for component in (
            'PortShoulder', 'PortUpperArm', 'PortForearm', 'PortToolHead',
            'StarboardShoulder', 'StarboardUpperArm', 'StarboardForearm', 'StarboardToolHead',
        ):
            self.assertIn(component, self.pawn_h)

        for label in (
            'PrimeCentralTube',
            'ComputationCoreSleeve',
            'PowerReactorSleeve',
            'MainPropulsionAssembly',
            'ForwardScienceSensor',
            'PortThermalWing',
            'StarboardThermalWing',
            'PortManeuveringPod',
            'StarboardManeuveringPod',
            'DorsalSensorMast',
        ):
            self.assertIn(f'TEXT("{label}")', self.pawn_cpp)

        for generic_arm_label in (
            'TEXT("%sShoulder")',
            'TEXT("%sUpperArm")',
            'TEXT("%sForearm")',
            'TEXT("%sMiningToolHead")',
        ):
            self.assertIn(generic_arm_label, self.pawn_cpp)
        self.assertIn('ConfigureArm(true);', self.pawn_cpp)
        self.assertIn('ConfigureArm(false);', self.pawn_cpp)
        self.assertIn('UpdateManipulatorVisuals();', self.pawn_cpp)

        self.assertIn('ForwardSensor->SetRelativeLocation(FVector(620.0, 0.0, 0.0))', self.pawn_cpp)
        self.assertIn('MainEngine->SetRelativeLocation(FVector(-620.0, 0.0, 0.0))', self.pawn_cpp)
        self.assertIn('PortRadiator->SetRelativeLocation(FVector(-30.0, -195.0, 0.0))', self.pawn_cpp)
        self.assertIn('StarboardRadiator->SetRelativeLocation(FVector(-30.0, 195.0, 0.0))', self.pawn_cpp)

    def test_probe_collision_envelope_tracks_prime_blockout_scale(self) -> None:
        self.assertIn('USphereComponent', self.pawn_h)
        self.assertIn('ProbeCollisionEnvelope', self.pawn_h)
        self.assertIn('TEXT("ProbeCollisionEnvelope")', self.pawn_cpp)
        self.assertIn('SetSphereRadius(800.0f)', self.pawn_cpp)
        self.assertIn('collision_envelope_radius_m{8.0}', self.sim_types)
        self.assertIn('Component->SetCollisionEnabled(ECollisionEnabled::NoCollision)', self.pawn_cpp)
        self.assertNotIn('SetSimulatePhysics(true)', self.pawn_cpp)

    def test_r_key_rights_probe_toward_camera_in_clunky_authoritative_steps(self) -> None:
        self.assertIn('EKeys::R', self.pawn_cpp)
        self.assertIn('BeginOrCancelCameraAlignedRighting', self.pawn_cpp)
        self.assertIn('Controller->GetControlRotation().GetNormalized()', self.pawn_cpp)
        self.assertIn('CameraAlignedRightingTarget = FRotator(', self.pawn_cpp)
        self.assertIn('ViewRotation.Pitch', self.pawn_cpp)
        self.assertIn('ViewRotation.Yaw', self.pawn_cpp)
        self.assertIn('RightingDegreesPerSecond = 36.0f', self.pawn_h)
        self.assertIn('RightingCommandIntervalSeconds = 0.10f', self.pawn_h)
        self.assertIn('FMath::FindDeltaAngleDegrees', self.pawn_cpp)
        self.assertIn('CommandAdjustAttitudeDegrees(Step)', self.pawn_cpp)
        self.assertNotIn('SetActorRotation(', self.pawn_cpp)

    def test_test_environment_never_bypasses_simulation_adapter_boundary(self) -> None:
        self.assertNotIn('everward::simulation', self.environment_h)
        self.assertNotIn('everward::simulation', self.environment_cpp)
        self.assertNotIn('SimulationCore', self.environment_h)
        self.assertNotIn('SimulationCore', self.environment_cpp)


if __name__ == "__main__":
    unittest.main()
