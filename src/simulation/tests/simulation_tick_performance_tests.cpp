#include "everward/simulation/impact_damage.hpp"

// PERFORMANCE_BUDGETS.md calls for headless simulation throughput to be
// treated as a first-class metric and for automated headless benchmarks to
// exist before engine-side frame budgets are layered on top. This is that
// first automated pass: a coarse, hardware-independent regression smoke
// gate over the exact authoritative per-frame call
// (DamageAwareProbeRuntime::advance_wall_ticks) Unreal's
// UProbeSimulationAdapter::TickComponent drives once per fixed step, not a
// precise timing budget. See ERROR_RESOLUTION_LEDGER.md-style NDEBUG note
// below for why every CTest source in this directory re-enables assert().
#undef NDEBUG
#include <cassert>
#include <chrono>
#include <cstdint>
#include <iostream>

using everward::simulation::DamageAwareProbeRuntime;
using everward::simulation::StaticSphereBody;

namespace {

constexpr std::int64_t kFixedStepTicks = 16667;

void register_representative_scene(DamageAwareProbeRuntime& runtime) {
    runtime.add_static_sphere_body({"bench-target-001", {40.0, 0.0, 0.0}, 2.0});
    runtime.add_static_sphere_body({"bench-target-002", {0.0, 103.0, 0.0}, 2.0});
    runtime.add_static_sphere_body({"bench-target-003", {0.0, 0.0, 174.0}, 2.0});
}

double run_frames(DamageAwareProbeRuntime& runtime, std::int64_t frame_count) {
    const auto start = std::chrono::steady_clock::now();
    for (std::int64_t frame = 0; frame < frame_count; ++frame) {
        runtime.advance_wall_ticks(kFixedStepTicks);
        (void)runtime.drain_events();
        (void)runtime.drain_damage_records();
    }
    const auto end = std::chrono::steady_clock::now();
    return std::chrono::duration<double>(end - start).count();
}

} // namespace

int main() {
    DamageAwareProbeRuntime runtime = DamageAwareProbeRuntime::make_canonical_ev0001();
    register_representative_scene(runtime);

    // Multiple equal windows make sustained growth visible without relying on
    // a single first-half/second-half ratio. For a linear per-tick cost growth
    // bug, adjacent late windows approach only ~1.4x rather than the misleading
    // 3x aggregate half ratio, so a 2x bound rejects that failure class while
    // retaining generous headroom for shared-runner jitter.
    constexpr std::int64_t kFramesPerWindow = 50'000;
    constexpr std::int64_t kWindowCount = 4;
    double window_seconds[kWindowCount]{};
    for (std::int64_t window = 0; window < kWindowCount; ++window) {
        window_seconds[window] = run_frames(runtime, kFramesPerWindow);
    }

    assert(runtime.tick() == kWindowCount * kFramesPerWindow * kFixedStepTicks);

    // Compare the two late windows, after warm-up effects have settled. The
    // small additive allowance prevents sub-millisecond clock noise from
    // dominating otherwise-fast runs, while the factor remains below the
    // ~3x signature that the previous two-half check accidentally allowed.
    if (window_seconds[2] > 0.005) {
        constexpr double kMaxLateWindowGrowthFactor = 2.0;
        assert(window_seconds[3] <=
               window_seconds[2] * kMaxLateWindowGrowthFactor + 0.025);
    }

    std::cout << "simulation_tick_performance_tests: "
              << (kWindowCount * kFramesPerWindow) << " frames in "
              << (window_seconds[0] + window_seconds[1] + window_seconds[2] + window_seconds[3])
              << "s (windows: " << window_seconds[0] << ", " << window_seconds[1]
              << ", " << window_seconds[2] << ", " << window_seconds[3] << "s)\n";

    return 0;
}
