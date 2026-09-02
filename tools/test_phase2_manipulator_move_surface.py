from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
SIM = ROOT / "src/simulation/include/everward/simulation"
SOURCE = ROOT / "unreal/Source/Everward"


class Phase2ManipulatorMoveSurfaceTests(unittest.TestCase):
    """Slice 7's "move" minimum interaction (PHASE2_VERTICAL_SLICE_PLAN.md):
    the sub-slice that wires manipulator_move.hpp's read-only
    grasped_target_position() math into authoritative registered-body
    position, the Unreal adapter's per-tick loop, and the Unreal-side scan
    target actor so a carried body visually follows the holding arm's wrist.
    Exercised as a source contract the same way manipulator_grasp's own
    integration layer is (see test_phase2_manipulator_grasp_surface.py)."""

    def setUp(self) -> None:
        self.software_policy = (SIM / "software_policy.hpp").read_text(encoding="utf-8")
        self.impact_damage = (SIM / "impact_damage.hpp").read_text(encoding="utf-8")
        self.manipulator_move = (SIM / "manipulator_move.hpp").read_text(encoding="utf-8")
        self.adapter_h = (SOURCE / "ProbeSimulationAdapter.h").read_text(encoding="utf-8")
        self.adapter_cpp = (SOURCE / "ProbeSimulationAdapter.cpp").read_text(encoding="utf-8")
        self.target_selection_bridge = (SOURCE / "ProbeTargetSelectionBridge.cpp").read_text(encoding="utf-8")
        self.environment_h = (SOURCE / "EverwardPhase2TestEnvironment.h").read_text(encoding="utf-8")
        self.environment_cpp = (SOURCE / "EverwardPhase2TestEnvironment.cpp").read_text(encoding="utf-8")
        self.cmake = (ROOT / "src/simulation/CMakeLists.txt").read_text(encoding="utf-8")

    def test_runtime_owns_the_single_mutation_point_for_a_carried_body(self) -> None:
        self.assertIn(
            "void update_static_sphere_body_position(const std::string& body_id, Vector3d new_center_m)",
            self.software_policy)
        # Fails closed rather than fabricating/re-registering a deregistered
        # body, matching add_static_sphere_body's own registered-body
        # invariants.
        self.assertIn("if (found == static_bodies_.end())", self.software_policy)
        self.assertIn("void update_static_sphere_body_position(const std::string& body_id, Vector3d new_center_m)",
            self.impact_damage)
        self.assertIn("runtime_.update_static_sphere_body_position(body_id, new_center_m)", self.impact_damage)

    def test_adapter_tick_wires_grasped_position_into_authoritative_state(self) -> None:
        self.assertIn("everward/simulation/manipulator_move.hpp", self.adapter_cpp)
        self.assertIn("everward::simulation::grasped_target_position(", self.adapter_cpp)
        self.assertIn("Core->update_static_sphere_body_position(Moved->body_id, Moved->world_position_m)",
            self.adapter_cpp)
        # Wired inside the fixed-step loop, after Manipulators->advance so it
        # reads that tick's just-slewed joint angles, not the prior tick's.
        advance_index = self.adapter_cpp.index("Manipulators->advance(FixedStepSeconds)")
        wiring_index = self.adapter_cpp.index("Core->update_static_sphere_body_position(")
        self.assertLess(advance_index, wiring_index)

    def test_adapter_exposes_a_registered_body_position_reader(self) -> None:
        self.assertIn(
            "bool GetStaticBodyPositionMeters(const FString& BodyId, FVector& OutPositionMeters) const",
            self.adapter_h)
        self.assertIn(
            "bool UProbeSimulationAdapter::GetStaticBodyPositionMeters(const FString& BodyId, FVector& OutPositionMeters) const",
            self.target_selection_bridge)
        # Reads Core->static_bodies() directly rather than inventing a
        # second position source alongside the authoritative registry.
        self.assertIn("Core->static_bodies()", self.target_selection_bridge)

    def test_environment_mirrors_authoritative_position_onto_the_existing_mesh(self) -> None:
        self.assertIn("void RefreshScanTargetPosition();", self.environment_h)
        self.assertIn("RefreshScanTargetPosition();", self.environment_cpp)
        self.assertIn("GetStaticBodyPositionMeters(BootstrapScanTargetId, PositionMeters)", self.environment_cpp)
        self.assertIn("ScanTargetMesh->SetRelativeLocation(PositionCentimeters)", self.environment_cpp)
        # No second, Unreal-owned position/physics model is introduced -- the
        # mesh only mirrors whatever Core already reports.
        self.assertNotIn("AddActorWorldOffset", self.environment_cpp)

    def test_cmake_registers_manipulator_move_tests(self) -> None:
        self.assertIn("manipulator_move_tests.cpp", self.cmake)
        self.assertIn("everward_manipulator_move_tests", self.cmake)


if __name__ == "__main__":
    unittest.main()
