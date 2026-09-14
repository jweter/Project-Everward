#include "everward/simulation/manipulator_collection.hpp"

#include "everward/simulation/manipulator_grasp.hpp"

#undef NDEBUG
#include <cassert>
#include <cstdio>
#include <string>
#include <vector>

namespace {

using everward::simulation::DamageAwareProbeRuntime;
using everward::simulation::ManipulatorArmContactSamples;
using everward::simulation::ManipulatorArmId;
using everward::simulation::ManipulatorRig;
using everward::simulation::ProbeWorldPose;
using everward::simulation::SampleCollectionResult;
using everward::simulation::StaticSphereBody;
using everward::simulation::Vector3d;
using everward::simulation::attempt_collect_grasped_target;
using everward::simulation::attempt_grasp_selected_target;
using everward::simulation::manipulator_arm_contact_samples;

ManipulatorRig deployed_rig(ManipulatorArmId id) {
    ManipulatorRig rig;
    rig.begin_deploy(id);
    rig.advance(ManipulatorRig::kDeployStowDurationS);
    return rig;
}

// Grasps a body at the arm's own current wrist position, mirroring
// manipulator_release_tests.cpp's grasped_rig helper.
ManipulatorRig grasped_rig(
    ManipulatorArmId id,
    const std::string& body_id,
    double sample_mass_kg,
    const std::string& material_id,
    std::vector<StaticSphereBody>& bodies) {
    ManipulatorRig rig = deployed_rig(id);
    const ManipulatorArmContactSamples samples = manipulator_arm_contact_samples(id, 1.0, {});
    bodies.push_back(StaticSphereBody{body_id, samples.wrist.center_m, 0.05, material_id, sample_mass_kg});
    const bool grasped = attempt_grasp_selected_target(rig, id, ProbeWorldPose{}, bodies, body_id);
    assert(grasped);
    return rig;
}

void test_collect_nothing_held_fails_closed() {
    ManipulatorRig rig = deployed_rig(ManipulatorArmId::Port);
    const std::vector<StaticSphereBody> bodies{};

    const auto result = attempt_collect_grasped_target(rig, ManipulatorArmId::Port, bodies);
    assert(!result.has_value());
}

void test_collect_deregistered_body_fails_closed() {
    std::vector<StaticSphereBody> bodies;
    ManipulatorRig rig = grasped_rig(ManipulatorArmId::Port, "sample", 12.0, "regolith", bodies);
    bodies.clear(); // simulate the held body vanishing from the registry

    const auto result = attempt_collect_grasped_target(rig, ManipulatorArmId::Port, bodies);
    assert(!result.has_value());
    assert(rig.arm(ManipulatorArmId::Port).grasped_target_body_id == "sample");
}

void test_collect_non_sample_body_fails_closed() {
    // sample_mass_kg defaults to 0.0 -- a plain reference/deposit body is not
    // manipulator-collectible.
    std::vector<StaticSphereBody> bodies;
    ManipulatorRig rig = grasped_rig(ManipulatorArmId::Port, "reference-body", 0.0, "", bodies);

    const auto result = attempt_collect_grasped_target(rig, ManipulatorArmId::Port, bodies);
    assert(!result.has_value());
    assert(rig.arm(ManipulatorArmId::Port).grasped_target_body_id == "reference-body");
}

void test_collect_sample_succeeds_and_releases_grasp() {
    std::vector<StaticSphereBody> bodies;
    ManipulatorRig rig = grasped_rig(ManipulatorArmId::Port, "sample", 12.5, "carbonaceous_chondrite_fragment", bodies);

    const auto result = attempt_collect_grasped_target(rig, ManipulatorArmId::Port, bodies);
    assert(result.has_value());
    assert(result->body_id == "sample");
    assert(result->material_id == "carbonaceous_chondrite_fragment");
    assert(result->mass_kg == 12.5);
    assert(rig.arm(ManipulatorArmId::Port).grasped_target_body_id.empty());

    // This module's own contract: it does not remove the body from the
    // registry or credit any storage itself -- that composition belongs to
    // the caller (see the header comment).
    assert(bodies.size() == 1);
    assert(bodies.front().body_id == "sample");
}

void test_collect_gate_stays_scoped_to_the_queried_arm() {
    std::vector<StaticSphereBody> bodies;
    ManipulatorRig rig = grasped_rig(ManipulatorArmId::Port, "sample", 5.0, "regolith", bodies);
    rig.begin_deploy(ManipulatorArmId::Starboard);
    rig.advance(ManipulatorRig::kDeployStowDurationS);

    const auto result = attempt_collect_grasped_target(rig, ManipulatorArmId::Starboard, bodies);
    assert(!result.has_value()); // Starboard holds nothing, regardless of Port's grasp
    assert(rig.arm(ManipulatorArmId::Port).grasped_target_body_id == "sample");
}

void test_runtime_overload_matches_free_function() {
    DamageAwareProbeRuntime runtime = DamageAwareProbeRuntime::make_canonical_ev0001();
    ManipulatorRig rig = deployed_rig(ManipulatorArmId::Port);

    const ManipulatorArmContactSamples samples = manipulator_arm_contact_samples(ManipulatorArmId::Port, 1.0, {});
    const Vector3d world_wrist = everward::simulation::contact_add(runtime.snapshot().position_m, samples.wrist.center_m);
    runtime.add_static_sphere_body({"sample", world_wrist, 0.05, "regolith", 8.0});
    runtime.select_target("sample");
    assert(attempt_grasp_selected_target(rig, runtime, ManipulatorArmId::Port));

    const auto result = attempt_collect_grasped_target(rig, runtime, ManipulatorArmId::Port);
    assert(result.has_value());
    assert(result->body_id == "sample");
    assert(result->mass_kg == 8.0);
    assert(rig.arm(ManipulatorArmId::Port).grasped_target_body_id.empty());

    // Confirms the runtime overload also does not itself deregister the body
    // or touch storage -- runtime.static_bodies() still reports it.
    assert(runtime.static_bodies().size() == 1);
    assert(runtime.material_inventory_kg().empty());
}

} // namespace

int main() {
    test_collect_nothing_held_fails_closed();
    test_collect_deregistered_body_fails_closed();
    test_collect_non_sample_body_fails_closed();
    test_collect_sample_succeeds_and_releases_grasp();
    test_collect_gate_stays_scoped_to_the_queried_arm();
    test_runtime_overload_matches_free_function();

    std::puts("manipulator_collection_tests: all tests passed");
    return 0;
}
