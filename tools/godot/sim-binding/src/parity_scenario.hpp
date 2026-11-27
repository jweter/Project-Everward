#pragma once
// The parity scenario, written once in C++ for the native reference run. The Godot side runs the same
// steps from GDScript through the binding (tests/parity.gd); both print the same lines and must match.
#include "everward_sim_core.hpp"

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
    return out;
}

}  // namespace everward::binding
