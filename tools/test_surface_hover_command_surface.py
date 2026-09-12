from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "unreal/Source/Everward"
DOCS = ROOT / "docs"
SIMULATION_INCLUDE = ROOT / "src/simulation/include/everward/simulation"
SIMULATION_TESTS = ROOT / "src/simulation/tests"


class SurfaceHoverCommandSurfaceTests(unittest.TestCase):
    def setUp(self) -> None:
        self.adapter_h = (SOURCE / "ProbeSimulationAdapter.h").read_text(encoding="utf-8")
        self.descent_bridge_cpp = (SOURCE / "ProbeSurfaceDescentBridge.cpp").read_text(encoding="utf-8")
        self.descent_guidance_hpp = (SIMULATION_INCLUDE / "surface_descent_guidance.hpp").read_text(encoding="utf-8")
        self.descent_guidance_tests_cpp = (
            SIMULATION_TESTS / "surface_descent_guidance_tests.cpp"
        ).read_text(encoding="utf-8")
        self.cmake_lists = (ROOT / "src/simulation/CMakeLists.txt").read_text(encoding="utf-8")
        self.plan = (DOCS / "PHASE2_VERTICAL_SLICE_PLAN.md").read_text(encoding="utf-8")
        self.status = (DOCS / "PROJECT_STATUS.md").read_text(encoding="utf-8")

    def test_hover_command_shaping_math_stays_engine_independent_and_ctest_covered(self) -> None:
        # The Slice 10 gap this closes: controlled_hover_velocity_command()
        # existed only as a standalone module with no adapter caller -- the
        # same "math foundation only, not yet wired" gap the descent command
        # already closed for constrain_surface_approach_velocity().
        self.assertIn("controlled_hover_velocity_command", self.descent_guidance_hpp)
        self.assertIn("SurfaceHoverProfile", self.descent_guidance_hpp)
        self.assertIn("controlled_hover_velocity_command", self.descent_guidance_tests_cpp)
        self.assertIn("everward_surface_descent_guidance_tests", self.cmake_lists)

    def test_adapter_exposes_hover_query_and_command_over_the_registered_planetary_body(self) -> None:
        self.assertIn("GetControlledHoverVelocityCommand", self.adapter_h)
        self.assertIn("CommandSetControlledHoverVelocityMetersPerSecond", self.adapter_h)

    def test_hover_query_and_command_are_actually_wired_rather_than_merely_present(self) -> None:
        self.assertIn("controlled_hover_velocity_command(", self.descent_bridge_cpp)
        self.assertIn("GetControlledHoverVelocityCommand", self.descent_bridge_cpp)
        self.assertIn("CommandSetControlledHoverVelocityMetersPerSecond", self.descent_bridge_cpp)

    def test_hover_command_reuses_the_existing_velocity_mutation_boundary(self) -> None:
        # No second velocity-mutation path: both the descent and hover
        # commands must route through the same SimulationCore::set_velocity_mps()
        # boundary CommandSetVelocityMetersPerSecond() already uses.
        hover_command_start = self.descent_bridge_cpp.index(
            "FEverwardProbeCommandResult UProbeSimulationAdapter::"
            "CommandSetControlledHoverVelocityMetersPerSecond("
        )
        self.assertIn("Core->set_velocity_mps(", self.descent_bridge_cpp[hover_command_start:])

    def test_hover_command_fails_closed_with_no_registered_planetary_body(self) -> None:
        # Both the descent and hover BlueprintCallable commands reject with
        # this exact message when no planetary body is registered.
        hover_command_start = self.descent_bridge_cpp.index(
            "FEverwardProbeCommandResult UProbeSimulationAdapter::"
            "CommandSetControlledHoverVelocityMetersPerSecond("
        )
        self.assertIn('"no planetary body registered"', self.descent_bridge_cpp[hover_command_start:])

    def test_hover_reuses_the_existing_controlled_descent_command_struct(self) -> None:
        # No second Blueprint-visible result struct is introduced for hover;
        # GetControlledHoverVelocityCommand() returns the same
        # FEverwardControlledDescentCommand shape the descent query already
        # returns.
        self.assertIn(
            "FEverwardControlledDescentCommand UProbeSimulationAdapter::GetControlledHoverVelocityCommand(",
            self.descent_bridge_cpp,
        )

    def test_slice_10_status_reflects_the_wired_hover_command(self) -> None:
        self.assertIn("GetControlledHoverVelocityCommand", self.plan + self.status)
        self.assertIn("CommandSetControlledHoverVelocityMetersPerSecond", self.plan + self.status)


if __name__ == "__main__":
    unittest.main()
