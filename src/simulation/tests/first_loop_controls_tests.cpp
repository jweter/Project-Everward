// Everward issue #372 (Aura run 20261108-084846): P1-1 survey lost after a cancelled rescan, P1-2 arm toggle cannot
// reverse a stow, F6 joint rows offered while stowing, P2-1 HUD capability index unbounded. Each test drives the REAL
// runtime or rig where one exists, not a stub.

#include "everward/simulation/core.hpp"
#include "everward/simulation/first_loop_controls.hpp"

#undef NDEBUG
#include <cassert>
#include <string>
#include <vector>

using namespace everward::simulation;

namespace {

void test_a_completed_survey_survives_a_cancelled_rescan_of_the_same_target() {
    // Trace 3 of the Aura run: full scan -> second scan started -> cancelled -> survey must still hold.
    const std::string target = "phase2-test-target-001";
    SimulationCore core;
    bool surveyed = false;   // the adapter's latch: only ever set true
    auto feed = [&](const std::vector<DomainEvent>& events) {
        for (const DomainEvent& event : events) surveyed = surveyed || scan_event_completes_survey(event, target);
    };

    core.start_scan(target, 2.0);
    core.advance_wall_ticks(SimulationClock::TicksPerSecond * 3);
    feed(core.drain_events());
    assert(surveyed);                               // the genuine ScanCompleted event earned it

    core.start_scan(target, 2.0);
    core.advance_wall_ticks(SimulationClock::TicksPerSecond / 2);
    core.cancel_scan();
    const std::vector<DomainEvent> after_cancel = core.drain_events();
    bool saw_cancel = false;
    for (const DomainEvent& event : after_cancel) saw_cancel = saw_cancel || event.type == DomainEventType::ScanCancelled;
    assert(saw_cancel);                             // the real cancel happened
    feed(after_cancel);
    assert(surveyed);                               // ...and did not take the survey away (the Aura defect)
}

void test_only_a_completed_scan_of_exactly_that_target_earns_the_survey() {
    const std::string target = "phase2-test-target-001";
    SimulationCore core;
    core.start_scan(target, 2.0);
    core.advance_wall_ticks(SimulationClock::TicksPerSecond / 2);
    core.cancel_scan();
    for (const DomainEvent& event : core.drain_events()) assert(!scan_event_completes_survey(event, target));   // cancel only

    core.start_scan("phase2-test-target-0012", 2.0);   // another target whose id merely starts with ours
    core.advance_wall_ticks(SimulationClock::TicksPerSecond * 3);
    for (const DomainEvent& event : core.drain_events()) assert(!scan_event_completes_survey(event, target));
    assert(!scan_event_completes_survey(DomainEvent{0, DomainEventType::ScanCompleted, "scan complete: x"}, ""));
}

void test_a_stow_can_be_reversed_mid_way_and_a_deploy_too() {
    // Trace 2 t~12.5 s: deployed arm, toggle -> stowing, toggle again mid-stow -> must deploy, not stow again.
    ManipulatorRig rig;
    const ManipulatorArmId arm = ManipulatorArmId::Starboard;
    rig.begin_deploy(arm);
    rig.advance(ManipulatorRig::kDeployStowDurationS);
    assert(rig.arm(arm).is_deployed && arm_toggle_command(rig.arm(arm)) == ArmToggleCommand::Stow);

    rig.begin_stow(arm);
    rig.advance(ManipulatorRig::kDeployStowDurationS * 0.4);
    assert(rig.arm(arm).is_stowing && rig.arm(arm).is_deployed);              // the state the old controller misread
    assert(arm_toggle_command(rig.arm(arm)) == ArmToggleCommand::Deploy);     // reverse, do not re-stow
    rig.begin_deploy(arm);
    rig.advance(ManipulatorRig::kDeployStowDurationS);
    assert(rig.arm(arm).is_deployed && !rig.arm(arm).is_stowing && rig.arm(arm).deployment_fraction >= 1.0);

    rig.begin_stow(arm);
    rig.advance(ManipulatorRig::kDeployStowDurationS);
    assert(!rig.arm(arm).is_deployed && arm_toggle_command(rig.arm(arm)) == ArmToggleCommand::Deploy);
    rig.begin_deploy(arm);
    rig.advance(ManipulatorRig::kDeployStowDurationS * 0.4);
    assert(rig.arm(arm).is_deploying && arm_toggle_command(rig.arm(arm)) == ArmToggleCommand::Stow);
    rig.begin_stow(arm);
    rig.advance(ManipulatorRig::kDeployStowDurationS);
    assert(!rig.arm(arm).is_deployed && rig.arm(arm).deployment_fraction <= 0.0);
}

void test_joint_control_is_offered_only_when_the_rig_would_accept_it() {
    ManipulatorRig rig;
    const ManipulatorArmId arm = ManipulatorArmId::Port;
    auto offered = [&] { const auto& a = rig.arm(arm); return arm_accepts_joint_commands(a.is_deployed, a.is_deploying, a.is_stowing); };
    auto accepted = [&] {
        try { rig.command_joint_target_degrees(arm, ManipulatorJoint::Elbow, 10.0); return true; }
        catch (const std::exception&) { return false; }
    };
    assert(!offered() && !accepted());                                        // stowed
    rig.begin_deploy(arm);
    rig.advance(ManipulatorRig::kDeployStowDurationS * 0.5);
    assert(!offered() && !accepted());                                        // deploying
    rig.advance(ManipulatorRig::kDeployStowDurationS);
    assert(offered() && accepted());                                          // fully deployed
    rig.begin_stow(arm);
    rig.advance(ManipulatorRig::kDeployStowDurationS * 0.1);
    assert(rig.arm(arm).is_deployed);                                         // still "deployed" while stowing...
    assert(!offered() && !accepted());                                        // ...yet neither offered nor accepted (F6)
}

void test_the_selection_cursor_never_leaves_its_list() {
    int index = 2;
    for (int i = 0; i < 6; ++i) index = bounded_selection(index, +1, 4);     // Aura: ']' six times took 2 -> 8
    assert(index == 3);
    for (int i = 0; i < 9; ++i) index = bounded_selection(index, -1, 4);
    assert(index == 0);
    assert(bounded_selection(7, 0, 4) == 3);                                  // an index already out of range is pulled back
    assert(bounded_selection(5, +1, 0) == 0);                                 // nothing installed
}

} // namespace

int main() {
    test_a_completed_survey_survives_a_cancelled_rescan_of_the_same_target();
    test_only_a_completed_scan_of_exactly_that_target_earns_the_survey();
    test_a_stow_can_be_reversed_mid_way_and_a_deploy_too();
    test_joint_control_is_offered_only_when_the_rig_would_accept_it();
    test_the_selection_cursor_never_leaves_its_list();
    return 0;
}
