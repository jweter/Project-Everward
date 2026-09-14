from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
SIM = ROOT / "src/simulation/include/everward/simulation"
SOURCE = ROOT / "unreal/Source/Everward"


class Phase2ManipulatorCollectionSurfaceTests(unittest.TestCase):
    """Slice 12 ("resource/sample loop", PHASE2_VERTICAL_SLICE_PLAN.md /
    PHASE2_MANIPULATOR_COLLECTION_TEST.md) initial scope: "sampleable
    object/material" and "manipulator/tool acquisition of a sampled object"
    as opposed to mining.hpp's repeated-cycle tool-beam extraction. Exercised
    as a source contract the same way the other manipulator sub-slices are
    (see test_phase2_manipulator_release_surface.py)."""

    def setUp(self) -> None:
        self.types = (SIM / "types.hpp").read_text(encoding="utf-8")
        self.software_policy = (SIM / "software_policy.hpp").read_text(encoding="utf-8")
        self.impact_damage = (SIM / "impact_damage.hpp").read_text(encoding="utf-8")
        self.manipulator_collection = (SIM / "manipulator_collection.hpp").read_text(encoding="utf-8")
        self.save_data = (SIM / "save_data.hpp").read_text(encoding="utf-8")
        self.cmake = (ROOT / "src/simulation/CMakeLists.txt").read_text(encoding="utf-8")
        self.adapter_h = (SOURCE / "ProbeSimulationAdapter.h").read_text(encoding="utf-8")
        self.adapter_cpp = (SOURCE / "ProbeSimulationAdapter.cpp").read_text(encoding="utf-8")
        self.controller_h = (SOURCE / "EverwardPlayerController.h").read_text(encoding="utf-8")
        self.controller_cpp = (SOURCE / "EverwardPlayerController.cpp").read_text(encoding="utf-8")
        self.hud_cpp = (SOURCE / "EverwardHUD.cpp").read_text(encoding="utf-8")
        self.environment_h = (SOURCE / "EverwardPhase2TestEnvironment.h").read_text(encoding="utf-8")
        self.environment_cpp = (SOURCE / "EverwardPhase2TestEnvironment.cpp").read_text(encoding="utf-8")

    def test_static_sphere_body_gains_sample_mass_kg(self) -> None:
        self.assertIn("double sample_mass_kg{0.0};", self.types)

    def test_runtime_gains_the_sole_deregistration_mutation_point(self) -> None:
        self.assertIn("bool remove_static_sphere_body(const std::string& body_id) noexcept", self.software_policy)
        self.assertIn("bool remove_static_sphere_body(const std::string& body_id) noexcept", self.impact_damage)
        self.assertIn("runtime_.remove_static_sphere_body(body_id)", self.impact_damage)

    def test_collection_module_is_engine_independent_and_fails_closed(self) -> None:
        self.assertNotIn("#include \"CoreMinimal.h\"", self.manipulator_collection)
        self.assertNotIn("USTRUCT", self.manipulator_collection)
        self.assertIn("if (held_id.empty()) return std::nullopt;", self.manipulator_collection)
        self.assertIn("if (found == bodies.end()) return std::nullopt;", self.manipulator_collection)
        self.assertIn("if (!(found->sample_mass_kg > 0.0)) return std::nullopt;", self.manipulator_collection)
        self.assertIn("rig.release_grasp(id);", self.manipulator_collection)

    def test_collection_module_does_not_itself_mutate_the_registry_or_storage(self) -> None:
        # The header's own contract: only the grasp is released here. Removing
        # the body and crediting storage stay the caller's (adapter's)
        # composition, mirroring how the "move" sub-slice already split
        # grasped_target_position() (read) from update_static_sphere_body_position()
        # (the runtime's own mutation).
        self.assertNotIn(".remove_static_sphere_body(", self.manipulator_collection)
        self.assertNotIn(".add_stored_material_kg(", self.manipulator_collection)
        # The convenience overload takes a const runtime reference, so it
        # could not call either non-const mutator even if it tried.
        self.assertIn("const DamageAwareProbeRuntime& runtime", self.manipulator_collection)

    def test_cmake_registers_manipulator_collection_tests(self) -> None:
        self.assertIn("manipulator_collection_tests.cpp", self.cmake)
        self.assertIn("everward_manipulator_collection_tests", self.cmake)

    def test_save_data_round_trips_sample_mass_kg_as_additive_field(self) -> None:
        self.assertIn("object.set(\"sample_mass_kg\", JsonValue(body.sample_mass_kg));", self.save_data)
        self.assertIn("value.find(\"sample_mass_kg\")", self.save_data)

    def test_adapter_composes_the_gate_with_the_registry_and_storage_mutations(self) -> None:
        self.assertIn("CommandCollectGraspedTarget", self.adapter_h)
        self.assertIn(
            "everward::simulation::attempt_collect_grasped_target(*Manipulators, *Core", self.adapter_cpp)
        self.assertIn("Core->remove_static_sphere_body(Result->body_id);", self.adapter_cpp)
        self.assertIn("Core->add_stored_material_kg(Result->mass_kg, Result->material_id);", self.adapter_cpp)

    def test_player_controller_binds_a_dedicated_collect_key(self) -> None:
        self.assertIn("CollectGraspedSample", self.controller_h)
        self.assertIn(
            "InputComponent->BindKey(EKeys::X, IE_Pressed, this, &AEverwardPlayerController::CollectGraspedSample);",
            self.controller_cpp)
        self.assertIn("CommandCollectGraspedTarget(ArmId)", self.controller_cpp)

    def test_hud_documents_the_new_binding(self) -> None:
        self.assertIn("COLLECT GRASPED SAMPLE", self.hud_cpp)
        self.assertIn("COLLECT SAMPLE", self.hud_cpp)

    def test_environment_registers_a_manipulator_collectible_sample_body(self) -> None:
        self.assertIn("SampleTargetId = TEXT(\"phase2-test-target-004\")", self.environment_h)
        self.assertIn("SampleTargetSampleMassKilograms", self.environment_h)
        self.assertIn("AEverwardPhase2TestEnvironment::SampleTargetId", self.adapter_cpp)
        self.assertIn("AEverwardPhase2TestEnvironment::SampleTargetSampleMassKilograms", self.adapter_cpp)
        # Distinct from the plain reference bodies (sample_mass_kg defaults to
        # 0.0, i.e. not collectible) and from the bootstrap mining deposit.
        self.assertIn("carbonaceous_chondrite_fragment", self.adapter_cpp)

    def test_environment_hides_the_sample_once_actually_collected(self) -> None:
        self.assertIn("RefreshSampleTarget", self.environment_h)
        self.assertIn("RefreshSampleTarget();", self.environment_cpp)
        self.assertIn("bSampleTargetCollected = true;", self.environment_cpp)
        self.assertIn("SampleTargetMesh->SetVisibility(false);", self.environment_cpp)


if __name__ == "__main__":
    unittest.main()
