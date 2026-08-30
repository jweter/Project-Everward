#include "everward/simulation/mining.hpp"

#include <cassert>
#include <cmath>
#include <string>

using namespace everward::simulation;

namespace {

ManipulatorRig ready_rig_with_port_tool() {
    ManipulatorRig rig;
    rig.begin_deploy(ManipulatorArmId::Port);
    rig.advance(ManipulatorRig::kDeployStowDurationS);
    assert(rig.arm(ManipulatorArmId::Port).is_deployed);
    rig.attach_tool(ManipulatorArmId::Port);
    return rig;
}

ResourceDeposit deposit_near_port_wrist(const ManipulatorRig& rig, double gap_m = 0.20) {
    const ManipulatorArmState& arm = rig.arm(ManipulatorArmId::Port);
    const ManipulatorArmContactSamples samples =
        manipulator_arm_contact_samples(ManipulatorArmId::Port, arm.deployment_fraction, arm.angles);

    ResourceDeposit deposit;
    deposit.body.body_id = "test-rock";
    deposit.body.radius_m = 0.50;
    deposit.body.center_m = {
        samples.wrist.center_m.x + deposit.body.radius_m + samples.wrist.radius_m + gap_m,
        samples.wrist.center_m.y,
        samples.wrist.center_m.z,
    };
    deposit.material_id = "iron_regolith";
    deposit.display_name = "Iron-bearing regolith";
    deposit.remaining_kg = 20.0;
    deposit.extraction_kg_per_cycle = 5.0;
    return deposit;
}

} // namespace

int main() {
    {
        ManipulatorRig rig = ready_rig_with_port_tool();
        MiningSystem mining;
        mining.add_deposit(deposit_near_port_wrist(rig));

        const MiningAttemptResult before_scan = mining.mine_once(
            "test-rock", rig, ProbeWorldPose{}, 0.0, 500.0);
        assert(!before_scan.accepted);
        assert(before_scan.detail.find("scan target first") != std::string::npos);

        mining.mark_surveyed("test-rock");
        assert(mining.is_surveyed("test-rock"));
        assert(mining.survey_summary("test-rock").find("MINING UNLOCKED") != std::string::npos);

        const MiningAttemptResult first = mining.mine_once(
            "test-rock", rig, ProbeWorldPose{}, 0.0, 500.0);
        assert(first.accepted);
        assert(std::fabs(first.extracted_kg - 5.0) < 1e-9);
        assert(std::fabs(first.stored_material_kg - 5.0) < 1e-9);
        assert(std::fabs(first.remaining_deposit_kg - 15.0) < 1e-9);
        assert(first.tool_surface_gap_m <= MiningSystem::kGeneration1ToolWorkingReachM);
    }

    {
        // A surveyed body still cannot be mined by remote control: physical
        // manipulator deployment/tool state is part of the requirement.
        ManipulatorRig rig;
        MiningSystem mining;
        ResourceDeposit deposit;
        deposit.body.body_id = "no-arm-rock";
        deposit.body.center_m = {2.0, 0.0, 0.0};
        deposit.body.radius_m = 0.5;
        deposit.display_name = "Test material";
        deposit.remaining_kg = 10.0;
        mining.add_deposit(deposit);
        mining.mark_surveyed("no-arm-rock");

        const MiningAttemptResult result = mining.mine_once(
            "no-arm-rock", rig, ProbeWorldPose{}, 0.0, 500.0);
        assert(!result.accepted);
        assert(result.detail.find("deploy a manipulator arm") != std::string::npos);
    }

    {
        ManipulatorRig rig = ready_rig_with_port_tool();
        MiningSystem mining;
        mining.add_deposit(deposit_near_port_wrist(rig, 3.0));
        mining.mark_surveyed("test-rock");

        const MiningAttemptResult result = mining.mine_once(
            "test-rock", rig, ProbeWorldPose{}, 0.0, 500.0);
        assert(!result.accepted);
        assert(result.detail.find("not in reach") != std::string::npos);
    }

    {
        ManipulatorRig rig = ready_rig_with_port_tool();
        MiningSystem mining;
        mining.add_deposit(deposit_near_port_wrist(rig));
        mining.mark_surveyed("test-rock");

        const MiningAttemptResult result = mining.mine_once(
            "test-rock", rig, ProbeWorldPose{}, 498.0, 500.0);
        assert(result.accepted);
        assert(std::fabs(result.extracted_kg - 2.0) < 1e-9);
        assert(std::fabs(result.stored_material_kg - 2.0) < 1e-9);
    }

    return 0;
}
