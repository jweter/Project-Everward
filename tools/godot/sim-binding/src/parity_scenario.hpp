#pragma once
// The parity scenario, written once in C++ for the native reference run. The Godot side runs the same
// steps from GDScript through the binding (tests/parity.gd); both print the same lines and must match.
#include "everward_sim_core.hpp"

#include <cmath>
#include <cstdio>
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
    return out;
}

}  // namespace everward::binding
