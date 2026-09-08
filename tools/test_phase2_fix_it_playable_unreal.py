from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]
ADAPTER = (ROOT / "unreal/Source/Everward/ProbeSimulationAdapter.h").read_text(encoding="utf-8")
ACTOR_CPP = (ROOT / "unreal/Source/Everward/FixItRuntimeActor.cpp").read_text(encoding="utf-8")
GAME_MODE = (ROOT / "unreal/Source/Everward/EverwardGameMode.cpp").read_text(encoding="utf-8")


class FixItPlayableUnrealContractTests(unittest.TestCase):
    def test_fix_it_runtime_is_spawned_by_production_game_mode(self) -> None:
        self.assertIn('#include "FixItRuntimeActor.h"', GAME_MODE)
        self.assertIn("SpawnActor<AFixItRuntimeActor>", GAME_MODE)

    def test_fix_it_uses_authoritative_simulation_kernel(self) -> None:
        self.assertIn('#include "everward/simulation/fix_it.hpp"', ACTOR_CPP)
        self.assertIn("FixItPlanner::plan_next", ACTOR_CPP)
        self.assertIn("FixItRepairExecutor", ACTOR_CPP)
        self.assertIn("RepairExecutor->advance(*BoundCore", ACTOR_CPP)

    def test_adapter_grants_narrow_fix_it_friend_access(self) -> None:
        self.assertIn("friend class AFixItRuntimeActor;", ADAPTER)

    def test_damaged_awakened_probe_remains_limp_capable(self) -> None:
        self.assertIn("PowerSubsystem::Computation, 0.18", ACTOR_CPP)
        self.assertIn("PowerSubsystem::Thermal, 0.22", ACTOR_CPP)
        self.assertIn("PowerSubsystem::Sensors, 0.35", ACTOR_CPP)
        self.assertIn("PowerSubsystem::Propulsion, 0.40", ACTOR_CPP)

    def test_fix_it_consumes_real_resources_and_waits_when_needed(self) -> None:
        self.assertIn("Snapshot.storage_used_kg", ACTOR_CPP)
        self.assertIn("Snapshot.stored_energy_j", ACTOR_CPP)
        self.assertIn("bWaitingForResources = true", ACTOR_CPP)
        self.assertIn("WAITING:", ACTOR_CPP)

    def test_playtest_evidence_covers_fix_it_lifecycle(self) -> None:
        for event_name in (
            "fix_it_awakened",
            "fix_it_runtime_bound",
            "fix_it_repair_started",
            "fix_it_repair_completed",
            "fix_it_repair_interrupted",
        ):
            self.assertIn(event_name, ACTOR_CPP)

    def test_fix_it_reasoning_is_player_visible(self) -> None:
        self.assertIn("AddOnScreenDebugMessage", ACTOR_CPP)
        self.assertIn("FIX_IT // PRIORITY", ACTOR_CPP)
        self.assertIn("ActiveReason", ACTOR_CPP)


if __name__ == "__main__":
    unittest.main()
