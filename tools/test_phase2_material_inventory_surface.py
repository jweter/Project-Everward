from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
SIM = ROOT / "src/simulation/include/everward/simulation"
SOURCE = ROOT / "unreal/Source/Everward"


class Phase2MaterialInventorySurfaceTests(unittest.TestCase):
    """Slice 12 ("resource/sample loop") foundation follow-up
    (PHASE2_VERTICAL_SLICE_PLAN.md): a read-only HUD readout over the
    per-material storage breakdown (types.hpp's material_inventory_kg,
    credited by SimulationCore::add_stored_material_kg()) that the
    always-visible STORAGE row's single aggregate kilogram figure cannot
    show. No new authoritative state, mutation, or gameplay rule is
    introduced -- this only exposes data that already exists and is
    ctest-covered in simulation_core_tests.cpp/save_data_tests.cpp."""

    def setUp(self) -> None:
        self.core_hpp = (SIM / "core.hpp").read_text(encoding="utf-8")
        self.adapter_h = (SOURCE / "ProbeSimulationAdapter.h").read_text(encoding="utf-8")
        self.bridge = (SOURCE / "ProbeMiningBridge.cpp").read_text(encoding="utf-8")
        self.hud_cpp = (SOURCE / "EverwardHUD.cpp").read_text(encoding="utf-8")

    def test_core_already_owns_the_authoritative_breakdown(self) -> None:
        # This pass adds no new simulation-core state; it only exposes what
        # SimulationCore::material_inventory_kg() already accumulates.
        self.assertIn("material_inventory_kg()", self.core_hpp)
        self.assertIn("void add_stored_material_kg(", self.core_hpp)

    def test_adapter_declares_a_read_only_inventory_entry_and_accessor(self) -> None:
        self.assertIn("struct EVERWARD_API FEverwardMaterialInventoryEntry", self.adapter_h)
        self.assertIn("FString MaterialId;", self.adapter_h)
        self.assertIn("double Kilograms = 0.0;", self.adapter_h)
        self.assertIn(
            "TArray<FEverwardMaterialInventoryEntry> GetStoredMaterialInventory() const;",
            self.adapter_h,
        )

    def test_bridge_reads_cores_breakdown_without_inventing_a_second_one(self) -> None:
        self.assertIn(
            "TArray<FEverwardMaterialInventoryEntry> UProbeSimulationAdapter::GetStoredMaterialInventory() const",
            self.bridge,
        )
        self.assertIn("Core->material_inventory_kg()", self.bridge)
        # Fails closed to an empty array rather than fabricating an entry
        # when the adapter has no live simulation core, matching every other
        # read-only accessor's Core == nullptr contract.
        function_body = self.bridge.split(
            "TArray<FEverwardMaterialInventoryEntry> UProbeSimulationAdapter::GetStoredMaterialInventory() const",
            1,
        )[1]
        self.assertIn("if (Core == nullptr)", function_body.split("\n\n", 1)[0])

    def test_hud_draws_an_inventory_row_below_knowledge(self) -> None:
        self.assertIn("GetStoredMaterialInventory()", self.hud_cpp)
        self.assertIn("MaterialInventoryLine(", self.hud_cpp)
        self.assertIn("INVENTORY  EMPTY", self.hud_cpp)
        # The panel's fixed row budget (TelemetryHeight) must grow by exactly
        # one line to fit the new row rather than silently overlapping the
        # manipulator panel drawn above it.
        self.assertIn("LineHeight * 11.0f", self.hud_cpp)


if __name__ == "__main__":
    unittest.main()
