from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "unreal/Source/Everward"
DOCS = ROOT / "docs"
SIMULATION_INCLUDE = ROOT / "src/simulation/include/everward/simulation"
SIMULATION_TESTS = ROOT / "src/simulation/tests"


class SurfaceDescentCommandSurfaceTests(unittest.TestCase):
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

    def test_command_shaping_math_stays_engine_independent_and_ctest_covered(self) -> None:
        # The Slice 10 gap this closes: constrain_surface_approach_velocity()
        # existed only as a standalone module with no fail-closed entry point
        # reasoning about SimulationCore's *optional* registered planetary
        # body -- every caller had to already unwrap it themselves.
        self.assertIn("controlled_descent_velocity_command", self.descent_guidance_hpp)
        self.assertIn("std::optional<SphericalPlanetaryBody>", self.descent_guidance_hpp)
        self.assertIn("controlled_descent_velocity_command", self.descent_guidance_tests_cpp)
        self.assertTrue((SIMULATION_TESTS / "surface_descent_guidance_tests.cpp").exists())
        self.assertIn("everward_surface_descent_guidance_tests", self.cmake_lists)

    def test_adapter_exposes_query_and_command_over_the_registered_planetary_body(self) -> None:
        self.assertIn("FEverwardControlledDescentCommand", self.adapter_h)
        self.assertIn("GetControlledDescentVelocityCommand", self.adapter_h)
        self.assertIn("CommandSetControlledDescentVelocityMetersPerSecond", self.adapter_h)

    def test_query_and_command_are_actually_wired_rather_than_merely_present(self) -> None:
        self.assertIn("controlled_descent_velocity_command(", self.descent_bridge_cpp)
        self.assertIn("Core->planetary_body()", self.descent_bridge_cpp)
        self.assertIn("Core->snapshot().position_m", self.descent_bridge_cpp)

    def test_command_reuses_the_existing_velocity_mutation_boundary(self) -> None:
        # No second velocity-mutation path: the command must route through the
        # exact same SimulationCore::set_velocity_mps() boundary
        # CommandSetVelocityMetersPerSecond() already uses.
        self.assertIn("Core->set_velocity_mps(", self.descent_bridge_cpp)
        self.assertNotIn("SetActorLocation", self.descent_bridge_cpp)
        self.assertNotIn("Teleport", self.descent_bridge_cpp)

    def test_command_fails_closed_with_no_registered_planetary_body(self) -> None:
        self.assertIn("no planetary body registered", self.descent_bridge_cpp)

    def test_slice_10_status_reflects_the_wired_command(self) -> None:
        self.assertIn("GetControlledDescentVelocityCommand", self.plan + self.status)
        self.assertIn("CommandSetControlledDescentVelocityMetersPerSecond", self.plan + self.status)


if __name__ == "__main__":
    unittest.main()
