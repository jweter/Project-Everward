from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
SIM = ROOT / "src/simulation/include/everward/simulation"
SOURCE = ROOT / "unreal/Source/Everward"


class Phase2ManipulatorGraspSurfaceTests(unittest.TestCase):
    """Slice 7's "grasp or dock with a simple object" minimum interaction
    (PHASE2_VERTICAL_SLICE_PLAN.md): the first sub-slice past read-only reach
    telemetry. Introduces exactly one new piece of authoritative state
    (ManipulatorArmState::grasped_target_body_id) gated by the same fixed
    reach envelope manipulator_reach.hpp already reports. No move/dock
    physics is introduced. Exercised as a source contract the same way
    manipulator_reach's own integration layer is
    (see test_phase2_manipulator_reach_surface.py)."""

    def setUp(self) -> None:
        self.manipulator = (SIM / "manipulator.hpp").read_text(encoding="utf-8")
        self.manipulator_grasp = (SIM / "manipulator_grasp.hpp").read_text(encoding="utf-8")
        self.adapter_h = (SOURCE / "ProbeSimulationAdapter.h").read_text(encoding="utf-8")
        self.adapter_cpp = (SOURCE / "ProbeSimulationAdapter.cpp").read_text(encoding="utf-8")
        self.hud_cpp = (SOURCE / "EverwardHUD.cpp").read_text(encoding="utf-8")
        self.controller_h = (SOURCE / "EverwardPlayerController.h").read_text(encoding="utf-8")
        self.controller_cpp = (SOURCE / "EverwardPlayerController.cpp").read_text(encoding="utf-8")
        self.cmake = (ROOT / "src/simulation/CMakeLists.txt").read_text(encoding="utf-8")

    def test_rig_owns_grasp_state_and_its_own_mechanical_invariants(self) -> None:
        self.assertIn("std::string grasped_target_body_id", self.manipulator)
        self.assertIn("void begin_grasp(ManipulatorArmId id, const std::string& target_body_id)", self.manipulator)
        self.assertIn("void release_grasp(ManipulatorArmId id)", self.manipulator)
        self.assertIn("TargetGrasped", self.manipulator)
        self.assertIn("TargetReleased", self.manipulator)
        # Cannot stow while holding something, matching the existing
        # tool_attached guard.
        self.assertIn("release grasped target before stowing manipulator arm", self.manipulator)

    def test_grasp_gate_reuses_reach_status_rather_than_a_second_notion_of_close_enough(self) -> None:
        self.assertIn("everward/simulation/manipulator_reach.hpp", self.manipulator_grasp)
        self.assertIn("manipulator_reach_status(", self.manipulator_grasp)
        self.assertIn("!reach.has_value() || !reach->in_reach", self.manipulator_grasp)
        self.assertIn("rig.begin_grasp(id, selected_target_body_id)", self.manipulator_grasp)
        self.assertNotIn("kMaxWristRangeToSurfaceM", self.manipulator_grasp)
        self.assertNotIn("#include \"CoreMinimal.h\"", self.manipulator_grasp)
        self.assertNotIn("USTRUCT", self.manipulator_grasp)

    def test_runtime_overload_reads_live_authoritative_state_without_caching(self) -> None:
        self.assertIn("const DamageAwareProbeRuntime& runtime", self.manipulator_grasp)
        self.assertIn("runtime.snapshot()", self.manipulator_grasp)
        self.assertIn("runtime.selected_target_status()", self.manipulator_grasp)
        self.assertIn("runtime.static_bodies()", self.manipulator_grasp)

    def test_cmake_registers_manipulator_grasp_tests(self) -> None:
        self.assertIn("manipulator_grasp_tests.cpp", self.cmake)
        self.assertIn("everward_manipulator_grasp_tests", self.cmake)

    def test_unreal_adapter_exposes_grasp_commands_without_owning_the_gate(self) -> None:
        self.assertIn("bTargetGrasped", self.adapter_h)
        self.assertIn("GraspedTargetId", self.adapter_h)
        self.assertIn("CommandGraspSelectedTarget", self.adapter_h)
        self.assertIn("CommandReleaseGraspedTarget", self.adapter_h)

        self.assertIn("everward/simulation/manipulator_grasp.hpp", self.adapter_cpp)
        self.assertIn("everward::simulation::attempt_grasp_selected_target(*Manipulators, *Core", self.adapter_cpp)
        # release-with-consequence (PHASE2_MANIPULATOR_RELEASE_TEST.md) moved
        # the adapter off a direct, unconditional Manipulators->release_grasp
        # call and onto the same gated-wrapper pattern grasp itself uses.
        self.assertIn("everward/simulation/manipulator_release.hpp", self.adapter_cpp)
        self.assertIn("everward::simulation::attempt_release_grasped_target(*Manipulators, *Core", self.adapter_cpp)
        # The proximity/mechanical decision stays in the engine-independent
        # modules; the adapter only forwards the outcome rather than
        # re-deriving it (e.g. reimplementing begin_grasp's own guards).
        self.assertNotIn("already grasping", self.adapter_cpp)

    def test_hud_shows_grasp_state_on_the_arm_line(self) -> None:
        self.assertIn("bTargetGrasped", self.hud_cpp)
        self.assertIn("GraspedTargetId", self.hud_cpp)
        self.assertIn("HOLDING", self.hud_cpp)
        self.assertIn("GRASP", self.hud_cpp)

    def test_input_binds_grasp_toggle_to_the_hud_selected_arm(self) -> None:
        self.assertIn("void ToggleManipulatorGrasp()", self.controller_h)
        self.assertIn("EKeys::F, IE_Pressed, this, &AEverwardPlayerController::ToggleManipulatorGrasp", self.controller_cpp)
        self.assertIn("GetSelectedManipulatorArmIndex()", self.controller_cpp)
        self.assertIn("CommandGraspSelectedTarget", self.controller_cpp)
        self.assertIn("CommandReleaseGraspedTarget", self.controller_cpp)


if __name__ == "__main__":
    unittest.main()
