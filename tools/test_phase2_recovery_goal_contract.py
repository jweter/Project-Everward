"""Fix_It Recovery Goal Director source contract.

Portable CI proves source wiring, not a native Unreal UBT compile or playable
mining/repair loop. See docs/FIX_IT_RECOVERY_GOAL_DIRECTOR_V1.md.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
DOMAIN = ROOT / "src/simulation/include/everward/simulation/recovery_goal.hpp"
TEST = ROOT / "src/simulation/tests/recovery_goal_tests.cpp"
CMAKE = ROOT / "src/simulation/CMakeLists.txt"
ACTOR_H = ROOT / "unreal/Source/Everward/FixItRuntimeActor.h"
ACTOR_CPP = ROOT / "unreal/Source/Everward/FixItRuntimeActor.cpp"


class RecoveryGoalContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.domain = DOMAIN.read_text(encoding="utf-8")
        cls.test = TEST.read_text(encoding="utf-8")
        cls.cmake = CMAKE.read_text(encoding="utf-8")
        cls.actor_h = ACTOR_H.read_text(encoding="utf-8")
        cls.actor_cpp = ACTOR_CPP.read_text(encoding="utf-8")

    def test_single_authority_and_no_simulation_side_effects(self):
        self.assertIn("resolve_recovery_goal(", self.domain)
        self.assertIn("FixItDecision", self.domain)
        self.assertIn("FixItExecutionStatus", self.domain)
        self.assertIn("missing_material_kg", self.domain)
        self.assertIn("missing_energy_j", self.domain)
        for prohibited in (
            "consume_stored_material_kg(", "consume_stored_energy_j(",
            "set_subsystem_integrity(", "CommandMineBootstrapTarget(",
            "CommandStartScan(", "std::hash<", "Unsolvable",
        ):
            self.assertNotIn(prohibited, self.domain)

    def test_goal_requires_actual_completion_receipt(self):
        self.assertIn("FixItExecutionState::Completed", self.domain)
        self.assertIn("RecoveryGoalState::VerificationFailed", self.domain)
        self.assertIn("current_integrity + 1e-9 >= decision.target_integrity", self.domain)
        self.assertIn("test_completion_must_have_receipt_and_verified_integrity", self.test)

    def test_no_fake_repair_unlock_or_target_source(self):
        self.assertNotIn("CapabilityUnlock", self.domain)
        self.assertNotIn("find_material_sources(", self.domain)
        self.assertNotIn("SCAN TARGET", self.domain)
        self.assertNotIn("std::vector<ResolvableAction>", self.domain)
        self.assertIn("RecoveryGoalState::AdvisoryOnly", self.domain)

    def test_actor_getter_is_read_only_and_receipt_aware(self):
        self.assertIn("UFUNCTION(BlueprintPure, Category=\"Everward|Fix_It|Goal\")", self.actor_h)
        self.assertIn("FEverwardRecoveryGoalStatus GetRecoveryGoal() const;", self.actor_h)
        self.assertIn("resolve_recovery_goal(", self.actor_cpp)
        self.assertIn("if (RepairExecutor != nullptr", self.actor_cpp)
        self.assertIn("if (View.bHasGoal)", self.actor_cpp)
        getter_body = self.actor_cpp.split(
            "FEverwardRecoveryGoalStatus AFixItRuntimeActor::GetRecoveryGoal() const", 1
        )[1].split("void AFixItRuntimeActor::BindToCurrentProbe()", 1)[0]
        for prohibited in ("ReplanOrStart();", "->start(", "->advance(", "SeedDamagedAwakening();"):
            self.assertNotIn(prohibited, getter_body)

    def test_portable_coverage_and_registration(self):
        self.assertIn("everward_recovery_goal_tests", self.cmake)
        self.assertIn("tests/recovery_goal_tests.cpp", self.cmake)
        for scenario in (
            "test_awakening_survival_first_goal_and_true_cost",
            "test_stable_id_across_inventory_and_partial_recovery",
            "test_material_only_and_energy_only_blockers",
            "test_running_receipt_outweighs_new_planner_priority",
            "test_completion_must_have_receipt_and_verified_integrity",
            "test_interruption_does_not_fake_success",
            "test_no_goal_full_vs_damaged",
            "test_recommendations_are_not_fake_executable_repairs",
            "test_fabrication_unavailable_fails_closed",
            "test_time_budget_blocks_if_explicitly_constrained",
            "test_actual_executor_resources_and_transition_to_next_goal",
            "test_bounded_planner_only_repair_sequence_not_level_solvability",
        ):
            self.assertIn(scenario, self.test)


if __name__ == "__main__":
    unittest.main()
