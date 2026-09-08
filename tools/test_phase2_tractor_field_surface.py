from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "unreal/Source/Everward"


class Phase2TractorFieldSurfaceTests(unittest.TestCase):
    """Protect the first player-facing bridge over tractor_field.hpp."""

    @classmethod
    def setUpClass(cls) -> None:
        cls.adapter_h = (SOURCE / "ProbeSimulationAdapter.h").read_text(encoding="utf-8")
        cls.bridge_cpp = (SOURCE / "ProbeTractorFieldBridge.cpp").read_text(encoding="utf-8")
        cls.controller_tick = (SOURCE / "EverwardPlayerControllerInteractionTick.cpp").read_text(
            encoding="utf-8"
        )
        cls.environment_h = (SOURCE / "EverwardPhase2TestEnvironment.h").read_text(
            encoding="utf-8"
        )

    def test_adapter_exposes_tractor_status_and_commands(self) -> None:
        for token in (
            "FEverwardTractorFieldStatus",
            "GetTractorFieldStatus",
            "CommandEngageTractorField",
            "CommandDisengageTractorField",
            "AdvanceTractorField",
            "TractorTargetVelocitiesMetersPerSecond",
        ):
            self.assertIn(token, self.adapter_h)

    def test_bridge_reuses_engine_independent_tractor_physics(self) -> None:
        self.assertIn('everward/simulation/tractor_field.hpp', self.bridge_cpp)
        self.assertIn("TractorFieldSystem", self.bridge_cpp)
        self.assertIn("Field.step(", self.bridge_cpp)
        self.assertIn("FixedStepSeconds", self.bridge_cpp)

    def test_bridge_mutates_authoritative_simulation_state_not_actor_position(self) -> None:
        self.assertIn("Core->set_velocity_mps", self.bridge_cpp)
        self.assertIn("Core->update_static_sphere_body_position", self.bridge_cpp)
        self.assertNotIn("SetActorLocation", self.bridge_cpp)
        self.assertNotIn("SetWorldLocation", self.bridge_cpp)

    def test_tractor_range_is_surface_to_surface(self) -> None:
        self.assertIn(
            "Probe.collision_envelope_radius_m + Body->radius_m",
            self.bridge_cpp,
        )
        self.assertIn("GetTractorFieldStatus();", self.bridge_cpp)
        self.assertNotIn(
            "Selection.surface_range_m > TractorMaxSurfaceRangeMeters",
            self.bridge_cpp,
        )

    def test_phase2_targets_cover_light_heavy_and_equal_mass_behaviors(self) -> None:
        self.assertIn("BootstrapBodyMassKilograms = 500.0", self.environment_h)
        self.assertIn("ReferenceTarget1MassKilograms = 10000.0", self.environment_h)
        self.assertIn("ReferenceTarget2MassKilograms = 2500.0", self.environment_h)

    def test_player_holds_b_to_couple_and_release(self) -> None:
        self.assertIn("WasInputKeyJustPressed(EKeys::B)", self.controller_tick)
        self.assertIn("WasInputKeyJustReleased(EKeys::B)", self.controller_tick)
        self.assertIn("CommandEngageTractorField(1000.0)", self.controller_tick)
        self.assertIn("CommandDisengageTractorField", self.controller_tick)
        self.assertIn("AdvanceTractorField(DeltaSeconds)", self.controller_tick)

    def test_coupling_has_visible_world_feedback_and_discoverable_control(self) -> None:
        self.assertIn("GetTractorFieldStatus", self.controller_tick)
        self.assertIn("GetStaticBodyPositionMeters", self.controller_tick)
        self.assertIn("DrawDebugLine", self.controller_tick)
        self.assertIn("DrawDebugSphere", self.controller_tick)
        self.assertIn("TRACTOR COUPLED", self.controller_tick)
        self.assertIn("HOLD [B]", self.controller_tick)

    def test_bridge_preserves_zero_g_target_drift_after_release(self) -> None:
        self.assertIn("target retains zero-g drift", self.bridge_cpp)
        self.assertIn("TractorTargetVelocitiesMetersPerSecond", self.bridge_cpp)
        self.assertIn("Entry.Value * FixedStepSeconds", self.bridge_cpp)


if __name__ == "__main__":
    unittest.main()
