from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
SIM = ROOT / "src/simulation/include/everward/simulation"
SOURCE = ROOT / "unreal/Source/Everward"


class Phase2ManipulatorReachSurfaceTests(unittest.TestCase):
    """Slice 7's "align a manipulator" minimum interaction
    (PHASE2_VERTICAL_SLICE_PLAN.md): read-only telemetry over whether a
    deployed arm's wrist is within a fixed reach envelope of the currently
    selected physical target's surface. No grasp/attach/dock state is
    introduced. Exercised as a source contract the same way the rest of the
    Unreal integration layer is (see test_phase2_manipulator_arm_surface.py
    and test_phase2_target_selection_surface.py)."""

    def setUp(self) -> None:
        self.manipulator_reach = (SIM / "manipulator_reach.hpp").read_text(encoding="utf-8")
        self.manipulator_hull_contact = (SIM / "manipulator_hull_contact.hpp").read_text(encoding="utf-8")
        self.adapter_h = (SOURCE / "ProbeSimulationAdapter.h").read_text(encoding="utf-8")
        self.adapter_cpp = (SOURCE / "ProbeSimulationAdapter.cpp").read_text(encoding="utf-8")
        self.hud_cpp = (SOURCE / "EverwardHUD.cpp").read_text(encoding="utf-8")
        self.cmake = (ROOT / "src/simulation/CMakeLists.txt").read_text(encoding="utf-8")

    def test_reach_math_is_engine_independent_and_reuses_existing_geometry(self) -> None:
        self.assertIn("struct ManipulatorReachStatus", self.manipulator_reach)
        self.assertIn("manipulator_reach_status(", self.manipulator_reach)
        self.assertIn("everward/simulation/manipulator_hull_contact.hpp", self.manipulator_reach)
        self.assertIn("everward/simulation/target_selection.hpp", self.manipulator_reach)
        # No new local-to-world convention, forward-kinematics model, or
        # range formula: reuse manipulator_arm_contact_samples,
        # rotate_local_contact_offset, and surface_range_to_body exactly as
        # manipulator_hull_contact.hpp's own environment guard does.
        self.assertIn("manipulator_arm_contact_samples(", self.manipulator_reach)
        self.assertIn("rotate_local_contact_offset(", self.manipulator_reach)
        self.assertIn("surface_range_to_body(", self.manipulator_reach)
        self.assertNotIn("#include \"CoreMinimal.h\"", self.manipulator_reach)
        self.assertNotIn("UENUM", self.manipulator_reach)
        self.assertNotIn("USTRUCT", self.manipulator_reach)

    def test_reach_fails_closed_rather_than_fabricating_a_result(self) -> None:
        self.assertIn("std::optional<ManipulatorReachStatus>", self.manipulator_reach)
        self.assertIn("if (selected_target_body_id.empty()) return std::nullopt;", self.manipulator_reach)
        self.assertIn(
            "if (!arm_state.is_deployed || arm_state.is_stowing || arm_state.is_deploying) return std::nullopt;",
            self.manipulator_reach,
        )
        self.assertIn("if (target == bodies.end()) return std::nullopt;", self.manipulator_reach)

    def test_reach_envelope_is_a_fixed_small_constant(self) -> None:
        self.assertIn("struct ManipulatorReachEnvelopeMeters", self.manipulator_reach)
        self.assertIn("kMaxWristRangeToSurfaceM", self.manipulator_reach)

    def test_runtime_overload_reads_live_authoritative_state_without_caching(self) -> None:
        self.assertIn("const DamageAwareProbeRuntime& runtime", self.manipulator_reach)
        self.assertIn("runtime.snapshot()", self.manipulator_reach)
        self.assertIn("runtime.selected_target_status()", self.manipulator_reach)
        self.assertIn("runtime.static_bodies()", self.manipulator_reach)

    def test_cmake_registers_manipulator_reach_tests(self) -> None:
        self.assertIn("manipulator_reach_tests.cpp", self.cmake)
        self.assertIn("everward_manipulator_reach_tests", self.cmake)

    def test_unreal_adapter_exposes_reach_accessor_without_owning_the_math(self) -> None:
        self.assertIn("struct EVERWARD_API FEverwardManipulatorReachStatus", self.adapter_h)
        for field in ("bHasResult", "bInReach", "WristRangeToSurfaceMeters", "RemainingDistanceMeters"):
            self.assertIn(field, self.adapter_h)
        self.assertIn("GetManipulatorReachStatus", self.adapter_h)

        self.assertIn("everward/simulation/manipulator_reach.hpp", self.adapter_cpp)
        self.assertIn("everward::simulation::manipulator_reach_status(*Core", self.adapter_cpp)
        self.assertIn("Manipulators->arm(SimulationArmId)", self.adapter_cpp)
        # The accessor must not reimplement the fixed-envelope comparison,
        # the forward kinematics, or the fail-closed gating itself -- all of
        # that stays in the engine-independent module.
        self.assertNotIn("kMaxWristRangeToSurfaceM", self.adapter_cpp)
        self.assertNotIn("manipulator_arm_contact_samples(", self.adapter_cpp)

    def test_hud_renders_reach_for_the_selected_arm_only_with_a_target_selected(self) -> None:
        self.assertIn("GetManipulatorReachStatus", self.hud_cpp)
        self.assertIn("bShowReachRow", self.hud_cpp)
        self.assertIn("TargetSelection.bHasSelection", self.hud_cpp)
        self.assertIn("ReachStatus.bHasResult", self.hud_cpp)
        self.assertIn("ReachStatus.bInReach", self.hud_cpp)
        self.assertIn("RemainingDistanceMeters", self.hud_cpp)
        self.assertIn("WristRangeToSurfaceMeters", self.hud_cpp)
        self.assertNotIn("everward/simulation", self.hud_cpp)


if __name__ == "__main__":
    unittest.main()
