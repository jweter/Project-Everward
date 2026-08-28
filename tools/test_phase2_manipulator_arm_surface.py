from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
SIM = ROOT / "src/simulation/include/everward/simulation"
SOURCE = ROOT / "unreal/Source/Everward"


class Phase2ManipulatorArmSurfaceTests(unittest.TestCase):
    def setUp(self) -> None:
        self.manipulator = (SIM / "manipulator.hpp").read_text(encoding="utf-8")
        self.adapter_h = (SOURCE / "ProbeSimulationAdapter.h").read_text(encoding="utf-8")
        self.adapter_cpp = (SOURCE / "ProbeSimulationAdapter.cpp").read_text(encoding="utf-8")
        self.controller_h = (SOURCE / "EverwardPlayerController.h").read_text(encoding="utf-8")
        self.controller_cpp = (SOURCE / "EverwardPlayerController.cpp").read_text(encoding="utf-8")
        self.hud_cpp = (SOURCE / "EverwardHUD.cpp").read_text(encoding="utf-8")
        self.cmake = (ROOT / "src/simulation/CMakeLists.txt").read_text(encoding="utf-8")

    def test_rig_is_engine_independent_with_exactly_two_arms(self) -> None:
        self.assertIn("enum class ManipulatorArmId", self.manipulator)
        self.assertIn("Port,", self.manipulator)
        self.assertIn("Starboard", self.manipulator)
        self.assertIn("class ManipulatorRig", self.manipulator)
        self.assertNotIn("#include \"CoreMinimal.h\"", self.manipulator)
        self.assertNotIn("UENUM", self.manipulator)
        self.assertNotIn("USTRUCT", self.manipulator)

    def test_joints_are_constrained_and_rate_limited(self) -> None:
        self.assertIn("enum class ManipulatorJoint", self.manipulator)
        self.assertIn("Shoulder", self.manipulator)
        self.assertIn("Elbow", self.manipulator)
        self.assertIn("Wrist", self.manipulator)
        self.assertIn("shoulder_range()", self.manipulator)
        self.assertIn("elbow_range()", self.manipulator)
        self.assertIn("wrist_range()", self.manipulator)
        self.assertIn("kJointSlewDegreesPerSecond", self.manipulator)
        self.assertIn("clamp_to_range", self.manipulator)
        self.assertIn("slew_toward", self.manipulator)

    def test_deploy_stow_is_gradual_and_gates_joint_and_tool_commands(self) -> None:
        self.assertIn("kDeployStowDurationS", self.manipulator)
        self.assertIn("void begin_deploy(ManipulatorArmId id)", self.manipulator)
        self.assertIn("void begin_stow(ManipulatorArmId id)", self.manipulator)
        self.assertIn("detach tool before stowing manipulator arm", self.manipulator)
        self.assertIn("must be fully deployed to command a joint", self.manipulator)
        self.assertIn("must be fully deployed to attach a tool", self.manipulator)
        self.assertIn("ArmDeployCompleted", self.manipulator)
        self.assertIn("ArmStowCompleted", self.manipulator)

    def test_cmake_registers_manipulator_tests(self) -> None:
        self.assertIn("manipulator_tests.cpp", self.cmake)
        self.assertIn("everward_manipulator_tests", self.cmake)

    def test_unreal_adapter_exposes_manipulator_surface_without_owning_it(self) -> None:
        self.assertIn("ManipulatorRig", self.adapter_h)
        self.assertIn("EEverwardManipulatorArmId", self.adapter_h)
        self.assertIn("EEverwardManipulatorJoint", self.adapter_h)
        self.assertIn("FEverwardManipulatorArmState", self.adapter_h)
        for method in (
            "GetManipulatorArmStates",
            "CommandDeployManipulatorArm",
            "CommandStowManipulatorArm",
            "CommandSetManipulatorJointTargetDegrees",
            "CommandAttachManipulatorTool",
            "CommandDetachManipulatorTool",
        ):
            self.assertIn(method, self.adapter_h)

        self.assertIn("everward/simulation/manipulator.hpp", self.adapter_cpp)
        self.assertIn("new everward::simulation::ManipulatorRig()", self.adapter_cpp)
        self.assertIn("delete Manipulators", self.adapter_cpp)
        self.assertIn("Manipulators->advance(FixedStepSeconds)", self.adapter_cpp)
        self.assertIn("Manipulators->begin_deploy(", self.adapter_cpp)
        self.assertIn("Manipulators->begin_stow(", self.adapter_cpp)
        self.assertIn("Manipulators->command_joint_target_degrees(", self.adapter_cpp)
        self.assertIn("Manipulators->attach_tool(", self.adapter_cpp)
        self.assertIn("Manipulators->detach_tool(", self.adapter_cpp)
        # Joint clamping/slew math stays in the engine-independent module.
        self.assertNotIn("clamp_to_range", self.adapter_cpp)
        self.assertNotIn("slew_toward", self.adapter_cpp)

    def test_player_controller_exposes_deploy_stow_and_tool_toggle_input(self) -> None:
        self.assertIn("EKeys::One", self.controller_cpp)
        self.assertIn("EKeys::Two", self.controller_cpp)
        self.assertIn("EKeys::Three", self.controller_cpp)
        self.assertIn("TogglePortManipulatorArm", self.controller_h)
        self.assertIn("ToggleStarboardManipulatorArm", self.controller_h)
        self.assertIn("ToggleManipulatorTool", self.controller_h)
        self.assertIn("GetManipulatorArmStates", self.controller_cpp)
        self.assertIn("CommandDeployManipulatorArm", self.controller_cpp)
        self.assertIn("CommandStowManipulatorArm", self.controller_cpp)
        self.assertIn("CommandAttachManipulatorTool", self.controller_cpp)
        self.assertIn("CommandDetachManipulatorTool", self.controller_cpp)

    def test_hud_renders_authoritative_arm_state_without_owning_it(self) -> None:
        self.assertIn("GetManipulatorArmStates", self.hud_cpp)
        self.assertIn("DEPLOYING", self.hud_cpp)
        self.assertIn("STOWING", self.hud_cpp)
        self.assertIn("DEPLOYED", self.hud_cpp)
        self.assertIn("STOWED", self.hud_cpp)
        self.assertNotIn("everward/simulation", self.hud_cpp)


if __name__ == "__main__":
    unittest.main()
