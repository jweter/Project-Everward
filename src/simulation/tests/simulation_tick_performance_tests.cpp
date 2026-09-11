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

// Mirrors UProbeSimulationAdapter.h's FixedStepTicks: the exact tick count
// the real game advances the authoritative simulation by on every fixed
// 60 Hz step, so this benchmark measures the same per-frame call the
// player's build actually pays for.
constexpr std::int64_t kFixedStepTicks = 16667;

// A representative scene (matching the Phase 2 test environment's
// SCAN-001/002/003 layout) so the benchmark exercises registered-body
// gravity/contact resolution rather than an empty-world best case.
void register_representative_scene(DamageAwareProbeRuntime& runtime) {
    runtime.add_static_sphere_body({"bench-target-001", {40.0, 0.0, 0.0}, 2.0});
    runtime.add_static_sphere_body({"bench-target-002", {0.0, 103.0, 0.0}, 2.0});
    runtime.add_static_sphere_body({"bench-target-003", {0.0, 0.0, 174.0}, 2.0});
}

double run_frames(DamageAwareProbeRuntime& runtime, std::int64_t frame_count) {
    const auto start = std::chrono::steady_clock::now();
    for (std::int64_t frame = 0; frame < frame_count; ++frame) {
        runtime.advance_wall_ticks(kFixedStepTicks);
        // Real per-frame usage always drains what it produces (see
        // ProbeSimulationAdapter.cpp's TickComponent); an un-drained event/
        // damage-record vector would otherwise grow without bound and this
        // benchmark would be measuring that leak instead of steady-state
        // per-frame cost.
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

    constexpr std::int64_t kFramesPerHalf = 100'000;

    const double first_half_seconds = run_frames(runtime, kFramesPerHalf);
    const double second_half_seconds = run_frames(runtime, kFramesPerHalf);

    // Correctness: every requested frame actually advanced the authoritative
    // clock by exactly kFixedStepTicks, matching how the real fixed-step
    // loop accumulates tick(). A benchmark that silently measured a no-op
    // would be worthless as a regression gate.
    assert(runtime.tick() == 2 * kFramesPerHalf * kFixedStepTicks);

    // Absolute floor: closed-form per-tick integration over a handful of
    // registered bodies should comfortably clear tens of thousands of
    // frames/second on any CI runner. This ceiling leaves roughly two
    // orders of magnitude of headroom below that so it will not flake on
    // slow/shared hardware, while still catching a catastrophic regression
    // (for example an accidentally quadratic contact/gravity pass).
    constexpr double kMaxSecondsPerHalf = 10.0;
    assert(first_half_seconds < kMaxSecondsPerHalf);
    assert(second_half_seconds < kMaxSecondsPerHalf);

    // Growth-over-time check, independent of absolute machine speed:
    // steady-state per-frame cost should not climb as more frames are
    // simulated. This is what would actually catch the class of bug
    // PERFORMANCE_BUDGETS.md warns about ("memory does not grow
    // continuously when no new persistent entities/events are being
    // created") -- e.g. an accidentally undrained accumulator scanned
    // every tick -- without depending on this runner's absolute speed.
    // A small absolute-time floor avoids dividing by noise when both
    // halves are already fast enough to be dominated by clock jitter.
    if (first_half_seconds > 0.01) {
        constexpr double kMaxGrowthFactor = 3.0;
        assert(second_half_seconds <= first_half_seconds * kMaxGrowthFactor + 0.05);
    }

    std::cout << "simulation_tick_performance_tests: "
              << (2 * kFramesPerHalf) << " frames in "
              << (first_half_seconds + second_half_seconds) << "s ("
              << first_half_seconds << "s then " << second_half_seconds << "s)\n";

    return 0;
}
