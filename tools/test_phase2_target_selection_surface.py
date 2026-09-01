from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
SIM = ROOT / "src/simulation/include/everward/simulation"
SOURCE = ROOT / "unreal/Source/Everward"


class Phase2TargetSelectionSurfaceTests(unittest.TestCase):
    def setUp(self) -> None:
        self.target_selection = (SIM / "target_selection.hpp").read_text(encoding="utf-8")
        self.target_cycle_runtime = (SIM / "target_cycle_runtime.hpp").read_text(encoding="utf-8")
        self.software_policy = (SIM / "software_policy.hpp").read_text(encoding="utf-8")
        self.impact_damage = (SIM / "impact_damage.hpp").read_text(encoding="utf-8")
        self.adapter_h = (SOURCE / "ProbeSimulationAdapter.h").read_text(encoding="utf-8")
        self.bridge_cpp = (SOURCE / "ProbeTargetSelectionBridge.cpp").read_text(encoding="utf-8")
        self.controller_h = (SOURCE / "EverwardPlayerController.h").read_text(encoding="utf-8")
        self.controller_cpp = (SOURCE / "EverwardPlayerController.cpp").read_text(encoding="utf-8")
        self.hud_cpp = (SOURCE / "EverwardHUD.cpp").read_text(encoding="utf-8")
        self.cmake = (ROOT / "src/simulation/CMakeLists.txt").read_text(encoding="utf-8")

    def test_target_selection_math_is_engine_independent(self) -> None:
        self.assertIn("find_nearest_selectable_target", self.target_selection)
        self.assertIn("find_next_selectable_target", self.target_selection)
        self.assertIn("select_target_telemetry", self.target_selection)
        self.assertNotIn("#include \"CoreMinimal.h\"", self.target_selection)
        self.assertNotIn("UENUM", self.target_selection)
        self.assertNotIn("USTRUCT", self.target_selection)

    def test_cmake_registers_target_selection_and_cycle_runtime_tests(self) -> None:
        self.assertIn("target_selection_tests.cpp", self.cmake)
        self.assertIn("everward_target_selection_tests", self.cmake)
        self.assertIn("target_cycle_runtime_tests.cpp", self.cmake)
        self.assertIn("everward_target_cycle_runtime_tests", self.cmake)

    def test_probe_runtime_wires_authoritative_selection_state(self) -> None:
        self.assertIn("struct TargetSelectionStatus", self.software_policy)
        self.assertIn("everward/simulation/target_selection.hpp", self.software_policy)
        for method in (
            "void select_nearest_target(double max_selection_range_m)",
            "void select_target(const std::string& body_id)",
            "void clear_target_selection() noexcept",
            "TargetSelectionStatus selected_target_status() const noexcept",
        ):
            self.assertIn(method, self.software_policy)
        self.assertIn("find_nearest_selectable_target(", self.software_policy)
        self.assertIn("select_target_telemetry(", self.software_policy)

    def test_cycle_operation_uses_authoritative_runtime_state(self) -> None:
        self.assertIn("cycle_next_target_selection", self.target_cycle_runtime)
        self.assertIn("find_next_selectable_target(", self.target_cycle_runtime)
        self.assertIn("runtime.static_bodies()", self.target_cycle_runtime)
        self.assertIn("runtime.selected_target_status()", self.target_cycle_runtime)
        self.assertIn("runtime.select_target(", self.target_cycle_runtime)
        self.assertIn("runtime.clear_target_selection()", self.target_cycle_runtime)
        self.assertNotIn("CoreMinimal.h", self.target_cycle_runtime)

    def test_damage_aware_runtime_forwards_selection_without_duplicating_it(self) -> None:
        self.assertIn("void select_nearest_target(double max_selection_range_m)", self.impact_damage)
        self.assertIn("void select_target(const std::string& body_id)", self.impact_damage)
        self.assertIn("void clear_target_selection() noexcept", self.impact_damage)
        self.assertIn("selected_target_status() const noexcept", self.impact_damage)
        self.assertIn("runtime_.select_nearest_target(", self.impact_damage)
        self.assertIn("runtime_.select_target(", self.impact_damage)
        self.assertIn("runtime_.selected_target_status(", self.impact_damage)

    def test_unreal_adapter_exposes_target_cycle_without_owning_ordering(self) -> None:
        self.assertIn("FEverwardTargetSelectionStatus", self.adapter_h)
        for method in (
            "GetSelectedTargetStatus",
            "CommandSelectNearestTarget",
            "CommandCycleTarget",
            "CommandSelectTarget",
            "CommandClearTargetSelection",
        ):
            self.assertIn(method, self.adapter_h)

        self.assertIn("everward/simulation/target_cycle_runtime.hpp", self.bridge_cpp)
        self.assertIn("Core->selected_target_status()", self.bridge_cpp)
        self.assertIn("cycle_next_target_selection(*Core", self.bridge_cpp)
        self.assertIn("Core->select_target(", self.bridge_cpp)
        self.assertIn("Core->clear_target_selection()", self.bridge_cpp)
        self.assertNotIn("find_next_selectable_target", self.bridge_cpp)
        self.assertNotIn("surface_range_to_body", self.bridge_cpp)
        self.assertNotIn("closing_speed_to_body", self.bridge_cpp)

    def test_player_controller_cycles_target_with_t(self) -> None:
        self.assertIn("EKeys::T", self.controller_cpp)
        self.assertIn("SelectNearestPhysicalTarget", self.controller_h)
        self.assertIn("SelectNearestPhysicalTarget", self.controller_cpp)
        self.assertIn("CommandCycleTarget", self.controller_cpp)
        self.assertIn("TargetSelectionRangeMeters", self.controller_h)

    def test_hud_renders_authoritative_target_status_without_owning_it(self) -> None:
        self.assertIn("GetSelectedTargetStatus", self.hud_cpp)
        self.assertIn("bHasSelection", self.hud_cpp)
        self.assertIn("SurfaceRangeMeters", self.hud_cpp)
        self.assertIn("ClosingSpeedMetersPerSecond", self.hud_cpp)
        self.assertNotIn("everward/simulation", self.hud_cpp)

    def test_controls_reference_documents_target_cycle_binding(self) -> None:
        self.assertIn("CYCLE PHYSICAL TARGET", self.hud_cpp)
        self.assertIn("[T] CYCLE", self.hud_cpp)


if __name__ == "__main__":
    unittest.main()
