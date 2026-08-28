from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
SIM = ROOT / "src/simulation/include/everward/simulation"
SOURCE = ROOT / "unreal/Source/Everward"


class Phase2ManipulatorJointArticulationSurfaceTests(unittest.TestCase):
    """Joint-articulation input and the dedicated manipulator HUD page are the
    Slice 6 follow-up named in PHASE2_VERTICAL_SLICE_PLAN.md and
    PHASE2_MANIPULATOR_ARM_FOUNDATION_TEST.md's "explicitly not complete"
    list. This adds input/HUD surface over the already-authoritative
    ManipulatorRig::command_joint_target_degrees mechanics; it does not
    change engine-independent simulation behavior, so it is exercised here
    as a source contract the same way the rest of the Unreal integration
    layer is (see test_phase2_manipulator_arm_surface.py)."""

    def setUp(self) -> None:
        self.manipulator = (SIM / "manipulator.hpp").read_text(encoding="utf-8")
        self.adapter_h = (SOURCE / "ProbeSimulationAdapter.h").read_text(encoding="utf-8")
        self.adapter_cpp = (SOURCE / "ProbeSimulationAdapter.cpp").read_text(encoding="utf-8")
        self.controller_h = (SOURCE / "EverwardPlayerController.h").read_text(encoding="utf-8")
        self.controller_cpp = (SOURCE / "EverwardPlayerController.cpp").read_text(encoding="utf-8")
        self.hud_h = (SOURCE / "EverwardHUD.h").read_text(encoding="utf-8")
        self.hud_cpp = (SOURCE / "EverwardHUD.cpp").read_text(encoding="utf-8")

    def test_simulation_rig_is_unmodified_by_this_surface(self) -> None:
        # Joint-articulation input reads/writes the arm rig only through the
        # existing command_joint_target_degrees mechanics; it must not
        # require new engine-independent behavior.
        self.assertIn("void command_joint_target_degrees(", self.manipulator)
        self.assertIn("ManipulatorArmAngles commanded_angles{};", self.manipulator)

    def test_adapter_exposes_commanded_joint_targets(self) -> None:
        # Nudging a joint target must accumulate against the last commanded
        # target, not the transient current (slewing) angle, so the
        # Unreal-facing struct exposes both.
        for field in (
            "CommandedShoulderDegrees",
            "CommandedElbowDegrees",
            "CommandedWristDegrees",
        ):
            self.assertIn(field, self.adapter_h)

        self.assertIn("Out.CommandedShoulderDegrees = State.commanded_angles.shoulder_degrees;", self.adapter_cpp)
        self.assertIn("Out.CommandedElbowDegrees = State.commanded_angles.elbow_degrees;", self.adapter_cpp)
        self.assertIn("Out.CommandedWristDegrees = State.commanded_angles.wrist_degrees;", self.adapter_cpp)

        # Joint clamping/slew math must remain in the engine-independent
        # module, not be reimplemented at the adapter boundary.
        self.assertNotIn("clamp_to_range", self.adapter_cpp)
        self.assertNotIn("slew_toward", self.adapter_cpp)

    def test_hud_exposes_manipulator_arm_and_joint_selection_read_model(self) -> None:
        for symbol in (
            "ToggleManipulatorPanel",
            "CycleSelectedManipulatorArm",
            "SelectManipulatorJointShoulder",
            "SelectManipulatorJointElbow",
            "SelectManipulatorJointWrist",
            "IsManipulatorPanelExpanded",
            "GetSelectedManipulatorArmIndex",
            "GetSelectedManipulatorJointIndex",
        ):
            self.assertIn(symbol, self.hud_h)
            self.assertIn(symbol, self.hud_cpp)

    def test_hud_renders_a_dedicated_manipulator_page_with_joint_targets(self) -> None:
        self.assertIn("bManipulatorPanelExpanded", self.hud_cpp)
        self.assertIn("MANIPULATOR CONTROL", self.hud_cpp)
        self.assertIn("DEPLOY ARM TO COMMAND JOINTS", self.hud_cpp)
        self.assertIn("CommandedShoulderDegrees", self.hud_cpp)
        self.assertIn("CommandedElbowDegrees", self.hud_cpp)
        self.assertIn("CommandedWristDegrees", self.hud_cpp)
        self.assertNotIn("everward/simulation", self.hud_cpp)

    def test_player_controller_binds_joint_articulation_input(self) -> None:
        for key in (
            "EKeys::M",
            "EKeys::N",
            "EKeys::Four",
            "EKeys::Five",
            "EKeys::Six",
            "EKeys::Comma",
            "EKeys::Period",
        ):
            self.assertIn(key, self.controller_cpp)

        for symbol in (
            "ToggleManipulatorPanel",
            "CycleManipulatorArmSelection",
            "SelectManipulatorJointShoulder",
            "SelectManipulatorJointElbow",
            "SelectManipulatorJointWrist",
            "IncreaseManipulatorJointTarget",
            "DecreaseManipulatorJointTarget",
        ):
            self.assertIn(symbol, self.controller_h)
            self.assertIn(symbol, self.controller_cpp)

    def test_joint_target_command_gates_on_manipulator_panel_and_reads_last_commanded_value(self) -> None:
        self.assertIn("IsManipulatorPanelExpanded()", self.controller_cpp)
        self.assertIn("CommandSetManipulatorJointTargetDegrees(ArmId, Joint, CommandedDegrees + DeltaDegrees)", self.controller_cpp)


if __name__ == "__main__":
    unittest.main()
