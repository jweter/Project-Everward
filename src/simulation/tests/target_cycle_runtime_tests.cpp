#include "everward/simulation/target_cycle_runtime.hpp"

#undef NDEBUG
#include <cassert>
#include <cstdio>

using everward::simulation::DamageAwareProbeRuntime;
using everward::simulation::cycle_next_target_selection;

int main() {
    DamageAwareProbeRuntime runtime = DamageAwareProbeRuntime::make_canonical_ev0001();
    runtime.add_static_sphere_body({"far", {30.0, 0.0, 0.0}, 1.0});
    runtime.add_static_sphere_body({"near", {10.0, 0.0, 0.0}, 1.0});
    runtime.add_static_sphere_body({"middle", {20.0, 0.0, 0.0}, 1.0});

    auto status = cycle_next_target_selection(runtime, 100.0);
    assert(status.has_selection && status.body_id == "near");

    status = cycle_next_target_selection(runtime, 100.0);
    assert(status.has_selection && status.body_id == "middle");

    status = cycle_next_target_selection(runtime, 100.0);
    assert(status.has_selection && status.body_id == "far");

    status = cycle_next_target_selection(runtime, 100.0);
    assert(status.has_selection && status.body_id == "near");

    runtime.select_target("far");
    status = cycle_next_target_selection(runtime, 15.0);
    assert(status.has_selection && status.body_id == "near");

    status = cycle_next_target_selection(runtime, 1.0);
    assert(!status.has_selection);

    std::puts("target_cycle_runtime_tests: all tests passed");
    return 0;
}
