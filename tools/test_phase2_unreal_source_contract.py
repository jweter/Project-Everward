"""Static regression checks for the Phase 2 Unreal/simulation ownership boundary.

GitHub's portable CI does not build the Unreal Engine module, so these checks do
not pretend to replace UBT compilation. They protect the architectural contract
that can be validated from source alone: authoritative position comes from the
simulation adapter, metre-to-centimetre conversion happens once at that
boundary, and the presentation pawn does not independently author position.
"""

from __future__ import annotations

from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]
ADAPTER_HEADER = ROOT / "unreal/Source/Everward/ProbeSimulationAdapter.h"
ADAPTER_SOURCE = ROOT / "unreal/Source/Everward/ProbeSimulationAdapter.cpp"
PAWN_SOURCE = ROOT / "unreal/Source/Everward/EverwardProbePawn.cpp"


class Phase2UnrealSourceContractTests(unittest.TestCase):
    def setUp(self) -> None:
        self.header = ADAPTER_HEADER.read_text(encoding="utf-8")
        self.adapter = ADAPTER_SOURCE.read_text(encoding="utf-8")
        self.pawn = PAWN_SOURCE.read_text(encoding="utf-8")

    def test_adapter_owns_single_unit_conversion_constant(self) -> None:
        self.assertIn("MetersToCentimeters = 100.0", self.header)
        self.assertEqual(self.header.count("MetersToCentimeters"), 1)
        self.assertEqual(self.adapter.count("MetersToCentimeters"), 3)

    def test_adapter_syncs_owner_from_authoritative_snapshot(self) -> None:
        self.assertIn("void UProbeSimulationAdapter::SyncOwnerTransformFromSimulation()", self.adapter)
        self.assertIn("Core->snapshot().position_m", self.adapter)
        self.assertIn("GetOwner()", self.adapter)
        self.assertIn("Owner->SetActorLocation(PresentationPositionCentimeters", self.adapter)

    def test_sync_runs_at_initialization_and_after_fixed_step_drain(self) -> None:
        begin_play = self.adapter.split("void UProbeSimulationAdapter::BeginPlay()", 1)[1].split(
            "void UProbeSimulationAdapter::EndPlay", 1
        )[0]
        tick = self.adapter.split("void UProbeSimulationAdapter::TickComponent", 1)[1].split(
            "int64 UProbeSimulationAdapter::GetSimulationTick", 1
        )[0]

        self.assertIn("make_canonical_ev0001()", begin_play)
        self.assertIn("SyncOwnerTransformFromSimulation();", begin_play)
        self.assertIn("Core->advance_wall_ticks(FixedStepTicks);", tick)
        self.assertIn("SyncOwnerTransformFromSimulation();", tick)
        self.assertGreater(
            tick.index("SyncOwnerTransformFromSimulation();"),
            tick.index("Core->advance_wall_ticks(FixedStepTicks);"),
        )

    def test_pawn_does_not_independently_author_mechanical_position(self) -> None:
        self.assertNotIn("SetActorLocation", self.pawn)
        self.assertNotIn("AddActorWorldOffset", self.pawn)
        self.assertNotIn("SetWorldLocation", self.pawn)


if __name__ == "__main__":
    unittest.main()
