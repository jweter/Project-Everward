#pragma once
// The parity scenario, written once in C++ for the native reference run. The Godot side runs the same
// steps from GDScript through the binding (tests/parity.gd); both print the same lines and must match.
#include "everward_sim_core.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

namespace everward::binding {

inline std::vector<std::string> run_native_parity() {
    std::vector<std::string> out;
    char buf[256];
    auto line = [&](const char* fmt, auto... args) {
        std::snprintf(buf, sizeof(buf), fmt, args...);
        out.emplace_back(buf);
    };
    SimCore sim;
    sim.set_velocity(1.5, 0.0, -0.5);
    sim.advance_ticks(600000);  // 0.6 s at 1 us per wall tick
    const Vector3d p = sim.position();
    line("tick %lld pos %.9f %.9f %.9f", static_cast<long long>(sim.tick()), p.x, p.y, p.z);
    sim.setup_bootstrap_deposit(250.0);
    for (int i = 0; i < 6; ++i) {
        const MineResult r = sim.mine_once();
        line("mine %d accepted %d extracted %.9f remaining %.9f", i, r.accepted ? 1 : 0, r.extracted_kg, r.remaining_kg);
    }
    line("after6 inventory %.9f extracted %.9f remaining %.9f", sim.inventory_kg("iron_regolith"), sim.extracted_kg(),
         sim.deposit_remaining_kg());
    const std::string save = sim.save_json();
    for (int i = 0; i < 4; ++i) (void)sim.mine_once();
    line("after10 inventory %.9f remaining %.9f", sim.inventory_kg("iron_regolith"), sim.deposit_remaining_kg());
    line("load %d", sim.load_json(save) ? 1 : 0);
    line("loaded tick %lld inventory %.9f extracted %.9f remaining %.9f", static_cast<long long>(sim.tick()),
         sim.inventory_kg("iron_regolith"), sim.extracted_kg(), sim.deposit_remaining_kg());
    for (int i = 0; i < 2; ++i) (void)sim.mine_once();
    line("final inventory %.9f extracted %.9f remaining %.9f", sim.inventory_kg("iron_regolith"), sim.extracted_kg(),
         sim.deposit_remaining_kg());
    line("bad load %d", sim.load_json("{not json") ? 1 : 0);
    line("after bad load inventory %.9f", sim.inventory_kg("iron_regolith"));

    // Review regressions (#437). 1: storage is counted once, so a 600 kg deposit fills the 500 kg hold.
    SimCore cap;
    cap.setup_bootstrap_deposit(600.0);
    int accepted = 0;
    for (int i = 0; i < 200 && cap.mine_once().accepted; ++i) ++accepted;
    line("capacity accepted %d storage %.9f of %.9f remaining %.9f", accepted, cap.storage_used_kg(),
         cap.storage_capacity_kg(), cap.deposit_remaining_kg());
    // 2: loading a save taken before the deposit existed restores the no-deposit state.
    SimCore pre;
    const std::string before_deposit = pre.save_json();
    pre.setup_bootstrap_deposit(250.0);
    (void)pre.mine_once();
    // evaluate in order (C++ leaves function-argument order unspecified)
    const int pl = pre.load_json(before_deposit) ? 1 : 0;
    const int hd = pre.has_deposit() ? 1 : 0;
    const double inv = pre.inventory_kg("iron_regolith");
    const double rem = pre.deposit_remaining_kg();
    const double ext = pre.extracted_kg();
    const int after = pre.mine_once().accepted ? 1 : 0;
    line("predeposit load %d has_deposit %d inventory %.9f remaining %.9f extracted %.9f mine_after %d", pl, hd, inv,
         rem, ext, after);
    // 3: a well-formed save with an invalid runtime value is rejected, never thrown, and changes nothing.
    std::string bad = sim.save_json();
    const std::string key = "\"mass_kg\": ";
    const std::size_t at = bad.find(key);
    if (at != std::string::npos) {
        const std::size_t end = bad.find_first_of(",\n}", at + key.size());
        bad.replace(at + key.size(), end - (at + key.size()), "-1");
    }
    const int bad_loaded = sim.load_json(bad) ? 1 : 0;
    line("invalid mass load %d inventory %.9f", bad_loaded, sim.inventory_kg("iron_regolith"));
    // 4: invalid commands are rejected at the boundary instead of throwing into the engine.
    SimCore cmd;
    const int adv = cmd.advance_ticks(-1) ? 1 : 0;
    const int vel = cmd.set_velocity(std::nan(""), 0.0, 0.0) ? 1 : 0;
    const int dep = cmd.setup_bootstrap_deposit(-1.0) ? 1 : 0;
    const int mined = cmd.mine_once().accepted ? 1 : 0;
    line("invalid commands advance %d velocity %d deposit %d mine %d tick %lld", adv, vel, dep, mined,
         static_cast<long long>(cmd.tick()));

    // 5 (G2): turn, thrust in the probe frame, and fly into a rock; the simulation reports the contact.
    SimCore fly;
    const int att = fly.adjust_attitude(30.0, 10.0, 0.0) ? 1 : 0;
    const EulerAttitudeDegrees a = fly.attitude();
    line("attitude ok %d yaw %.9f pitch %.9f roll %.9f", att, a.yaw, a.pitch, a.roll);
    fly.adjust_attitude(-30.0, -10.0, 0.0);
    const int thr = fly.adjust_local_velocity(10.0, 0.0, 0.0) ? 1 : 0;
    const Vector3d v = fly.velocity();
    line("thrust ok %d velocity %.9f %.9f %.9f", thr, v.x, v.y, v.z);
    const int body = fly.add_sphere_body("rock-1", 30.0, 0.0, 0.0, 3.0) ? 1 : 0;
    const int bad_body = fly.add_sphere_body("", 0.0, 0.0, 0.0, 1.0) ? 1 : 0;
    int contacts = 0;
    for (int step = 0; step < 50; ++step) {  // 5 s in 0.1 s steps
        fly.advance_ticks(100000);
        contacts += fly.drain_contact_events();
    }
    const auto c = fly.last_contact();
    const Vector3d fp = fly.position();
    line("flight body %d bad_body %d contacts %d has %d id %s normal_speed %.9f pos %.9f %.9f %.9f", body, bad_body,
         contacts, c.has_contact ? 1 : 0, c.body_id.c_str(), c.normal_speed, fp.x, fp.y, fp.z);

    // 6 (G2): a body moved into the flight path is hit where it now is; moving an unknown body is rejected.
    SimCore mv;
    mv.add_sphere_body("rock-far", 100.0, 0.0, 0.0, 3.0);
    const int moved = mv.move_sphere_body("rock-far", 30.0, 0.0, 0.0) ? 1 : 0;
    const int moved_unknown = mv.move_sphere_body("nope", 1.0, 0.0, 0.0) ? 1 : 0;
    mv.adjust_local_velocity(10.0, 0.0, 0.0);
    int mv_contacts = 0;
    for (int step = 0; step < 50; ++step) {
        mv.advance_ticks(100000);
        mv_contacts += mv.drain_contact_events();
    }
    const Vector3d mp = mv.position();
    line("moved body %d unknown %d contacts %d pos %.9f", moved, moved_unknown, mv_contacts, mp.x);

    // 7 (G3): the first loop on a world deposit -- scan, survey, staging, approach, arm path, mine -- plus the
    // rejections that must hold on the way (unsurveyed, out of reach) and the G05 survey latch.
    const char* ore = "ore-1";
    const char* mat = "iron_bearing_silicate_regolith";
    SimCore fl;
    const int fl_dep = fl.add_deposit(ore, 40.0, 0.0, 0.0, 2.0, mat, "Iron-bearing regolith", 120.0, 5.0) ? 1 : 0;
    const int dup = fl.add_deposit(ore, 0.0, 0.0, 0.0, 1.0, mat, "", 1.0, 1.0) ? 1 : 0;
    const MineResult early = fl.mine_deposit(ore);
    line("deposit %d dup %d early %d reason %s", fl_dep, dup, early.accepted ? 1 : 0, early.reason.c_str());
    const int scan = fl.start_scan(ore, 10.0) ? 1 : 0;
    for (int step = 0; step < 105; ++step) fl.advance_ticks(100000);
    line("scan %d surveyed %d scanning %d", scan, fl.is_surveyed(ore) ? 1 : 0, fl.is_scanning() ? 1 : 0);
    const int rescan = fl.start_scan(ore, 10.0) ? 1 : 0;
    fl.advance_ticks(1000000);
    const int cancelled = fl.cancel_scan() ? 1 : 0;
    line("rescan %d cancelled %d surveyed %d", rescan, cancelled, fl.is_surveyed(ore) ? 1 : 0);
    const int joint_early = fl.set_joint_targets(0, 10.0, 10.0, 0.0) ? 1 : 0;
    const int toggle = fl.toggle_arm(0) ? 1 : 0;
    for (int step = 0; step < 22; ++step) fl.advance_ticks(100000);
    const int tool = fl.attach_tool(0) ? 1 : 0;
    const ArmInfo arm = fl.arm_info(0);
    line("joint_early %d toggle %d tool %d deployed %d accepts %d fraction %.9f", joint_early, toggle, tool,
         arm.deployed ? 1 : 0, arm.accepts_joint_commands ? 1 : 0, arm.deployment_fraction);
    const MineResult far = fl.mine_deposit(ore);
    line("far %d gap %.9f reason %s", far.accepted ? 1 : 0, far.tool_gap_m, far.reason.c_str());
    const StagingPlan plan = fl.plan_staging(0, ore);
    line("staging %d pos %.9f %.9f %.9f angles %.3f %.3f %.3f gap %.9f", plan.has_solution ? 1 : 0, plan.probe_position.x,
         plan.probe_position.y, plan.probe_position.z, plan.angles.shoulder_degrees, plan.angles.elbow_degrees,
         plan.angles.wrist_degrees, plan.tool_gap_m);
    int approach_steps = 0;
    double dist = 0.0;
    for (; approach_steps < 600; ++approach_steps) {  // the Unreal auto-approach: <= 2 m/s, stop within 0.25 m
        const Vector3d cur = fl.position();
        const Vector3d d{plan.probe_position.x - cur.x, plan.probe_position.y - cur.y, plan.probe_position.z - cur.z};
        dist = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
        if (dist <= 0.25) {
            fl.set_velocity(0.0, 0.0, 0.0);
            break;
        }
        const double speed = std::min(2.0, std::max(0.25, dist));
        fl.set_velocity(d.x / dist * speed, d.y / dist * speed, d.z / dist * speed);
        fl.advance_ticks(100000);
    }
    line("approach steps %d dist %.9f contacts %d", approach_steps, dist, fl.drain_contact_events());
    const auto path = fl.plan_arm_path(0, plan.angles.shoulder_degrees, plan.angles.elbow_degrees,
                                       plan.angles.wrist_degrees);
    int reached = 0;
    if (path) {
        for (const ManipulatorArmAngles& w : *path) {
            fl.set_joint_targets(0, w.shoulder_degrees, w.elbow_degrees, w.wrist_degrees);
            for (int step = 0; step < 100; ++step) {
                const ArmInfo ja = fl.arm_info(0);
                if (std::fabs(ja.angles.shoulder_degrees - w.shoulder_degrees) < 1e-6 &&
                    std::fabs(ja.angles.elbow_degrees - w.elbow_degrees) < 1e-6 &&
                    std::fabs(ja.angles.wrist_degrees - w.wrist_degrees) < 1e-6) {
                    ++reached;
                    break;
                }
                fl.advance_ticks(100000);
            }
        }
    }
    const ArmInfo posed = fl.arm_info(0);
    line("path %d waypoints %d reached %d angles %.6f %.6f %.6f", path ? 1 : 0, path ? static_cast<int>(path->size()) : 0,
         reached, posed.angles.shoulder_degrees, posed.angles.elbow_degrees, posed.angles.wrist_degrees);
    const MineResult fl_mined = fl.mine_deposit(ore);
    line("mine %d extracted %.9f remaining %.9f gap %.9f arm %d material %s", fl_mined.accepted ? 1 : 0, fl_mined.extracted_kg,
         fl_mined.remaining_kg, fl_mined.tool_gap_m, fl_mined.arm, fl_mined.material_id.c_str());
    line("inventory %.9f storage %.9f deposit %.9f", fl.inventory_kg(mat), fl.storage_used_kg(),
         fl.world_deposit_remaining_kg(ore));

    // 7b (#378 G08): world deposits and surveys survive save/load; mining continues from the saved mass; a save
    // without world deposits restores none.
    const std::string fl_save = fl.save_json();
    const MineResult after_save = fl.mine_deposit(ore);
    const int extra = fl.add_deposit("ore-2", -30.0, 12.0, 0.0, 2.0, "nickel_regolith", "Nickel regolith", 80.0, 4.0) ? 1 : 0;
    const int fl_load = fl.load_json(fl_save) ? 1 : 0;
    line("g08 after_save %d extra %d load %d surveyed %d deposit %.9f inventory %.9f ore2 %.3f", after_save.accepted ? 1 : 0,
         extra, fl_load, fl.is_surveyed(ore) ? 1 : 0, fl.world_deposit_remaining_kg(ore), fl.inventory_kg(mat),
         fl.world_deposit_remaining_kg("ore-2"));
    const MineResult resumed = fl.mine_deposit(ore);
    line("g08 resumed %d deposit %.9f inventory %.9f", resumed.accepted ? 1 : 0, fl.world_deposit_remaining_kg(ore),
         fl.inventory_kg(mat));
    SimCore blank;
    const int blank_load = fl.load_json(blank.save_json()) ? 1 : 0;
    line("g08 blank load %d ore1 %.3f surveyed %d", blank_load, fl.world_deposit_remaining_kg(ore), fl.is_surveyed(ore) ? 1 : 0);

    // 8 (G3, #378 G02): Fix_It on the damaged awakening waits without material, then repairs with mined material.
    SimCore fx;
    fx.enable_fix_it(true);
    const ComponentIntegritySnapshot awake = fx.integrity();
    line("fixit awake computation %.3f thermal %.3f sensors %.3f propulsion %.3f", awake.computation, awake.thermal,
         awake.sensors, awake.propulsion);
    for (int step = 0; step < 50; ++step) fx.advance_ticks(100000);
    const FixItInfo idle = fx.fix_it_info();
    line("fixit idle state %s subsystem %s need %.6f kg computation %.9f", idle.state.c_str(), idle.subsystem.c_str(),
         idle.material_required_kg, fx.integrity().computation);
    fx.setup_bootstrap_deposit(250.0);
    const MineResult fuel = fx.mine_once();
    const double energy0 = fx.stored_energy_j();
    line("fixit mined %d storage %.9f", fuel.accepted ? 1 : 0, fx.storage_used_kg());
    for (int step = 0; step < 200; ++step) fx.advance_ticks(100000);
    const FixItInfo fixed = fx.fix_it_info();
    const ComponentIntegritySnapshot healed = fx.integrity();
    line("fixit after state %s subsystem %s computation %.9f thermal %.9f storage %.9f inventory %.9f energy_used %.3f",
         fixed.state.c_str(), fixed.subsystem.c_str(), healed.computation, healed.thermal, fx.storage_used_kg(),
         fx.inventory_kg("iron_regolith"), energy0 - fx.stored_energy_j());
    const std::vector<std::string> fix_events = fx.drain_fix_it_events();
    line("fixit events %d first %s", static_cast<int>(fix_events.size()), fix_events.size() > 1 ? fix_events[1].c_str() : "");
    // Fix_It's cumulative consumption equals what actually left storage (completed plus active repair).
    line("fixit total consumed %.9f storage debit %.9f", fx.fix_it_info().total_material_consumed_kg,
         5.0 - fx.storage_used_kg());

    // Review regression (#444): loading a save while Fix_It is running must discard the
    // old executor operation before the restored world advances.
    SimCore load_fx;
    load_fx.enable_fix_it(true);
    load_fx.setup_bootstrap_deposit(250.0);
    (void)load_fx.mine_once();
    load_fx.advance_ticks(100000);
    const bool repair_was_running = load_fx.fix_it_info().state == "repairing";
    SimCore pristine_fx;
    pristine_fx.setup_bootstrap_deposit(250.0);
    (void)pristine_fx.mine_once();
    const int fix_load = load_fx.load_json(pristine_fx.save_json()) ? 1 : 0;
    load_fx.advance_ticks(100000);
    const FixItInfo after_fix_load = load_fx.fix_it_info();
    const double loaded_computation = load_fx.integrity().computation;
    const bool load_reset_ok = repair_was_running && fix_load == 1 &&
                               std::abs(loaded_computation - 1.0) < 1e-12 &&
                               after_fix_load.state == "monitor";
    if (!load_reset_ok) throw std::runtime_error("Fix_It executor survived load_json");
    line("fixit load reset %d computation %.9f state %s", load_reset_ok ? 1 : 0, loaded_computation,
         after_fix_load.state.c_str());
    return out;
}

}  // namespace everward::binding
