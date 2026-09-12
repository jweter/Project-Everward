from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
SIM = ROOT / "src/simulation/include/everward/simulation"
SOURCE = ROOT / "unreal/Source/Everward"


class Phase2ScienceKnowledgeSurfaceTests(unittest.TestCase):
    """Slice 11 ("Science as gameplay") foundation wiring: scanning
    accumulates per-target TargetKnowledgeState (science_knowledge.hpp)
    through the existing scan lifecycle rather than a countdown that
    discards its own result. Read-only for the HUD/adapter surface --
    classification/composition estimates remain later work, matching the
    other partial-slice foundations in this codebase (see
    test_phase2_manipulator_reach_surface.py, test_phase2_target_selection_surface.py)."""

    def setUp(self) -> None:
        self.types_hpp = (SIM / "types.hpp").read_text(encoding="utf-8")
        self.core_hpp = (SIM / "core.hpp").read_text(encoding="utf-8")
        self.software_policy = (SIM / "software_policy.hpp").read_text(encoding="utf-8")
        self.impact_damage = (SIM / "impact_damage.hpp").read_text(encoding="utf-8")
        self.save_data = (SIM / "save_data.hpp").read_text(encoding="utf-8")
        self.adapter_h = (SOURCE / "ProbeSimulationAdapter.h").read_text(encoding="utf-8")
        self.bridge_cpp = (SOURCE / "ProbeTargetSelectionBridge.cpp").read_text(encoding="utf-8")
        self.hud_cpp = (SOURCE / "EverwardHUD.cpp").read_text(encoding="utf-8")

    def test_snapshot_owns_target_knowledge_by_id(self) -> None:
        self.assertIn("everward/simulation/science_knowledge.hpp", self.types_hpp)
        self.assertIn("std::map<std::string, TargetKnowledgeState> target_knowledge{};", self.types_hpp)

    def test_core_accumulates_knowledge_only_through_the_scan_lifecycle(self) -> None:
        self.assertIn("void observe_active_scan_progress(double seconds)", self.core_hpp)
        self.assertIn("observe_active_scan_progress(seconds);", self.core_hpp)
        self.assertIn("apply_observation(entry->second, evidence);", self.core_hpp)
        self.assertIn("kNominalActiveScanConfidenceTimeConstantS", self.core_hpp)
        # Read accessors fail closed to nullopt rather than fabricating a
        # reading for a target never observed.
        self.assertIn(
            "std::optional<TargetKnowledgeState> target_knowledge_state(",
            self.core_hpp,
        )

    def test_probe_runtime_and_damage_aware_runtime_forward_without_duplicating_state(self) -> None:
        for source in (self.software_policy, self.impact_damage):
            self.assertIn(
                "const std::map<std::string, TargetKnowledgeState>& target_knowledge() const noexcept",
                source,
            )
            self.assertIn("target_knowledge_state(", source)
        self.assertIn("core_.target_knowledge()", self.software_policy)
        self.assertIn("runtime_.target_knowledge()", self.impact_damage)

    def test_save_data_round_trips_knowledge_as_an_additive_field(self) -> None:
        self.assertIn("target_knowledge_state_to_json(", self.save_data)
        self.assertIn("target_knowledge_state_from_json(", self.save_data)
        self.assertIn('object.set("target_knowledge", std::move(knowledge));', self.save_data)
        # Additive v1 field: absent read back as an empty map, no schema
        # migration, matching material_inventory_kg's own precedent.
        self.assertIn('value.find("target_knowledge")', self.save_data)

    def test_unreal_adapter_exposes_a_read_only_knowledge_status(self) -> None:
        self.assertIn("FEverwardTargetKnowledgeStatus", self.adapter_h)
        self.assertIn("EEverwardKnowledgeLevel", self.adapter_h)
        self.assertIn("GetSelectedTargetKnowledgeStatus", self.adapter_h)
        self.assertIn("FEverwardTargetKnowledgeStatus UProbeSimulationAdapter::GetSelectedTargetKnowledgeStatus() const", self.bridge_cpp)
        # Reuses the existing target-selection result rather than inventing a
        # second "which target" concept.
        self.assertIn("Core->selected_target_status();", self.bridge_cpp)
        self.assertIn("Core->target_knowledge_state(Selection.body_id)", self.bridge_cpp)

    def test_hud_renders_a_knowledge_row_without_relayouting_existing_rows(self) -> None:
        self.assertIn("GetSelectedTargetKnowledgeStatus()", self.hud_cpp)
        self.assertIn("KNOWLEDGE", self.hud_cpp)
        # The compact telemetry panel's fixed pixel layout is hand-tuned by
        # row count (see TARGET/SIM/arm rows above the new KNOWLEDGE row);
        # adding a row must extend the panel height rather than overlapping
        # the existing rows below it.
        self.assertIn("LineHeight * 10.0f", self.hud_cpp)


if __name__ == "__main__":
    unittest.main()
