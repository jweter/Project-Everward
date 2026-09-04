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
        self.environment_h = (SOURCE / "EverwardPhase2TestEnvironment.h").read_text(encoding="utf-8")
        self.environment_cpp = (SOURCE / "EverwardPhase2TestEnvironment.cpp").read_text(encoding="utf-8")
        self.cmake = (ROOT / "src/simulation/CMakeLists.txt").read_text(encoding="utf-8")

    def test_target_selection_math_is_engine_independent(self) -> None:
        self.assertIn("find_nearest_selectable_target", self.target_selection)
        self.assertIn("find_next_selectable_target", self.target_selection)
        self.assertIn("select_target_telemetry", self.target_selection)
        self.assertIn("classify_approach_motion", self.target_selection)
        self.assertIn("enum class ApproachMotionState", self.target_selection)
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
        self.assertIn("ApproachMotionState approach_motion", self.software_policy)
        self.assertIn("classify_approach_motion(", self.software_policy)

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
        self.assertIn("EEverwardApproachMotion", self.adapter_h)
        self.assertIn("ApproachMotion", self.adapter_h)
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
        self.assertIn("ToApproachMotion", self.bridge_cpp)
        self.assertIn("Selection.approach_motion", self.bridge_cpp)
        self.assertNotIn("find_next_selectable_target", self.bridge_cpp)
        self.assertNotIn("surface_range_to_body", self.bridge_cpp)
        self.assertNotIn("closing_speed_to_body", self.bridge_cpp)
        self.assertNotIn("classify_approach_motion", self.bridge_cpp)

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

    def test_hud_labels_approach_motion_instead_of_always_closing(self) -> None:
        # The TARGET row previously always printed "CLOSING %.2f M/S" even
        # while the probe was receding (a negative closing speed). It must
        # now read the authoritative ApproachMotion classification instead
        # of hardcoding one label for every sign.
        self.assertIn("TargetSelection.ApproachMotion", self.hud_cpp)
        self.assertIn("EEverwardApproachMotion::Closing", self.hud_cpp)
        self.assertIn("EEverwardApproachMotion::Opening", self.hud_cpp)
        self.assertIn("EEverwardApproachMotion::HoldingRange", self.hud_cpp)
        self.assertIn("OPENING %.2f M/S", self.hud_cpp)
        self.assertIn("HOLDING RANGE", self.hud_cpp)

    def test_environment_highlights_selected_target_from_authoritative_status(self) -> None:
        # Parallel-safe visual selection indicator: the registered physical
        # body's own mesh retints itself by reading the same
        # FEverwardTargetSelectionStatus the HUD row already renders, rather
        # than Unreal inventing a second, independently-tracked notion of
        # "selected". No new simulation state or engine-independent math is
        # introduced by this presentation-only change.
        self.assertIn("RefreshTargetSelectionHighlight", self.environment_h)
        self.assertIn("RefreshTargetSelectionHighlight", self.environment_cpp)
        self.assertIn("GetSelectedTargetStatus", self.environment_cpp)
        self.assertIn("TargetSelection.bHasSelection", self.environment_cpp)
        self.assertIn("TargetSelection.TargetId == BootstrapScanTargetId", self.environment_cpp)
        self.assertIn("ScanTargetDynamicMaterial", self.environment_cpp)
        self.assertNotIn("everward/simulation", self.environment_cpp)

    def test_environment_registers_multiple_targets_at_different_ranges(self) -> None:
        # Slice 8 (partial): PHASE2_TARGET_CYCLING_TEST.md's local acceptance
        # section previously called out that a single registered physical
        # body makes multi-target nearest-to-farthest wraparound impossible
        # to visually verify. Two additional registered reference targets at
        # different ranges close that specific gap without inventing a new
        # selection/cycling/highlight mechanic -- the existing ones are
        # simply given more registered bodies to operate on.
        for target_id, center_x in (
            ("phase2-test-target-002", "ReferenceTarget1CenterXMeters = 95.0"),
            ("phase2-test-target-003", "ReferenceTarget2CenterXMeters = 160.0"),
        ):
            self.assertIn(target_id, self.environment_h)
            self.assertIn(center_x, self.environment_h)

        self.assertIn("ReferenceTargetMeshes", self.environment_cpp)
        self.assertIn("ReferenceTargetIds", self.environment_cpp)

        # Both additional bodies must actually be registered with the
        # authoritative runtime, not merely rendered.
        adapter_cpp = (SOURCE / "ProbeSimulationAdapter.cpp").read_text(encoding="utf-8")
        self.assertIn("AEverwardPhase2TestEnvironment::ReferenceTarget1Id", adapter_cpp)
        self.assertIn("AEverwardPhase2TestEnvironment::ReferenceTarget2Id", adapter_cpp)
        self.assertEqual(adapter_cpp.count("Core->add_static_sphere_body("), 3)

        # Highlight and position mirroring must be generalized to these
        # targets too, reusing GetSelectedTargetStatus/GetStaticBodyPositionMeters
        # exactly as the bootstrap target already does, rather than only
        # working for one hardcoded body id.
        self.assertIn("RefreshReferenceTargets", self.environment_h)
        self.assertIn("void AEverwardPhase2TestEnvironment::RefreshReferenceTargets()", self.environment_cpp)
        self.assertIn("GetStaticBodyPositionMeters(ReferenceTargetIds[Index]", self.environment_cpp)
        self.assertIn("TargetSelection.TargetId == ReferenceTargetIds[Index]", self.environment_cpp)


if __name__ == "__main__":
    unittest.main()
