#include "everward/simulation/jose_autopilot.hpp"

#undef NDEBUG
#include <cassert>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace {

using everward::simulation::JoseAutopilotConfig;
using everward::simulation::JoseGuidanceCommand;
using everward::simulation::JoseGuidanceOutcome;
using everward::simulation::StaticSphereBody;
using everward::simulation::Vector3d;
using everward::simulation::jose_guidance_command;
using everward::simulation::jose_guidance_command_for_body;

bool nearly_equal(double a, double b, double epsilon = 1e-6) {
    return std::fabs(a - b) <= epsilon;
}

double magnitude(Vector3d v) {
    return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

void test_far_from_arrival_commands_cruise_speed() {
    // remaining = 1000 - 20 = 980; 980 * 0.35 far exceeds cruise, so the
    // command should clamp at cruise speed, not the raw proportional value.
    const JoseAutopilotConfig config;
    const auto result = jose_guidance_command(Vector3d{0.0, 0.0, 0.0}, Vector3d{1000.0, 0.0, 0.0}, 1000.0, config);
    assert(result.outcome == JoseGuidanceOutcome::Continue);
    assert(nearly_equal(magnitude(result.command_velocity_mps), config.cruise_speed_mps));
    assert(nearly_equal(result.command_velocity_mps.x, config.cruise_speed_mps));
    assert(nearly_equal(result.remaining_surface_range_m, 980.0));
}

void test_near_arrival_commands_proportionally_reduced_speed() {
    // destination_surface_range_m 40 -> remaining = 20; 20 * 0.35 = 7.0,
    // still above cruise (6.0), so this should also clamp at cruise. Pick a
    // range that lands strictly inside the proportional band instead.
    const JoseAutopilotConfig config;
    const double destination_surface_range_m = config.arrival_surface_standoff_m + 10.0; // remaining = 10
    const auto result = jose_guidance_command(Vector3d{0.0, 0.0, 0.0}, Vector3d{10.0, 0.0, 0.0}, destination_surface_range_m, config);
    assert(result.outcome == JoseGuidanceOutcome::Continue);
    const double expected_speed = 10.0 * config.approach_gain_per_second; // 3.5, inside [0.25, 6.0]
    assert(nearly_equal(magnitude(result.command_velocity_mps), expected_speed));
}

void test_approach_speed_floors_at_minimum_rather_than_stalling() {
    // remaining just above tolerance: proportional speed would be tiny, but
    // must never fall below minimum_approach_speed_mps (never a stalled crawl).
    const JoseAutopilotConfig config;
    const double destination_surface_range_m = config.arrival_surface_standoff_m + config.arrival_tolerance_m + 0.01;
    const auto result = jose_guidance_command(Vector3d{0.0, 0.0, 0.0}, Vector3d{0.51, 0.0, 0.0}, destination_surface_range_m, config);
    assert(result.outcome == JoseGuidanceOutcome::Continue);
    assert(nearly_equal(magnitude(result.command_velocity_mps), config.minimum_approach_speed_mps));
}

void test_command_direction_points_toward_destination() {
    const JoseAutopilotConfig config;
    const auto result = jose_guidance_command(Vector3d{5.0, 5.0, 5.0}, Vector3d{5.0, 5.0, 105.0}, 1000.0, config);
    assert(result.outcome == JoseGuidanceOutcome::Continue);
    assert(nearly_equal(result.command_velocity_mps.x, 0.0));
    assert(nearly_equal(result.command_velocity_mps.y, 0.0));
    assert(result.command_velocity_mps.z > 0.0);
}

void test_remaining_range_at_or_below_tolerance_reports_arrived() {
    const JoseAutopilotConfig config;
    const double exactly_at_tolerance = config.arrival_surface_standoff_m + config.arrival_tolerance_m;
    const auto at_tolerance = jose_guidance_command(Vector3d{0.0, 0.0, 0.0}, Vector3d{50.0, 0.0, 0.0}, exactly_at_tolerance, config);
    assert(at_tolerance.outcome == JoseGuidanceOutcome::Arrived);
    assert(nearly_equal(at_tolerance.command_velocity_mps.x, 0.0));
    assert(nearly_equal(at_tolerance.command_velocity_mps.y, 0.0));
    assert(nearly_equal(at_tolerance.command_velocity_mps.z, 0.0));

    const auto inside_standoff = jose_guidance_command(Vector3d{0.0, 0.0, 0.0}, Vector3d{50.0, 0.0, 0.0}, config.arrival_surface_standoff_m - 5.0, config);
    assert(inside_standoff.outcome == JoseGuidanceOutcome::Arrived);
}

void test_coincident_probe_and_destination_is_unresolved_not_a_fabricated_heading() {
    const JoseAutopilotConfig config;
    const auto result = jose_guidance_command(Vector3d{10.0, 10.0, 10.0}, Vector3d{10.0, 10.0, 10.0}, 1000.0, config);
    assert(result.outcome == JoseGuidanceOutcome::DestinationUnresolved);
    assert(nearly_equal(result.command_velocity_mps.x, 0.0));
    assert(nearly_equal(result.command_velocity_mps.y, 0.0));
    assert(nearly_equal(result.command_velocity_mps.z, 0.0));
}

void test_arrival_check_precedes_coincidence_check() {
    // A destination already within the arrival standoff that also happens to
    // be coincident with the probe must still report Arrived, not
    // DestinationUnresolved -- arrival is the higher-priority outcome.
    const JoseAutopilotConfig config;
    const auto result = jose_guidance_command(Vector3d{1.0, 2.0, 3.0}, Vector3d{1.0, 2.0, 3.0}, 0.0, config);
    assert(result.outcome == JoseGuidanceOutcome::Arrived);
}

void test_for_body_fails_closed_when_destination_not_registered() {
    const std::vector<StaticSphereBody> bodies{StaticSphereBody{"other", {5.0, 0.0, 0.0}, 1.0}};
    const auto result = jose_guidance_command_for_body(Vector3d{0.0, 0.0, 0.0}, bodies, "missing");
    assert(!result.has_value());
}

void test_for_body_reuses_surface_range_to_body() {
    // Body center at 1000m with radius 30m -> surface range 970m, matching
    // target_selection.hpp's surface_range_to_body exactly (no second
    // range formula).
    const std::vector<StaticSphereBody> bodies{StaticSphereBody{"rock", {1000.0, 0.0, 0.0}, 30.0}};
    const auto result = jose_guidance_command_for_body(Vector3d{0.0, 0.0, 0.0}, bodies, "rock");
    assert(result.has_value());
    assert(nearly_equal(result->remaining_surface_range_m, 970.0 - JoseAutopilotConfig{}.arrival_surface_standoff_m));
}

} // namespace

int main() {
    test_far_from_arrival_commands_cruise_speed();
    test_near_arrival_commands_proportionally_reduced_speed();
    test_approach_speed_floors_at_minimum_rather_than_stalling();
    test_command_direction_points_toward_destination();
    test_remaining_range_at_or_below_tolerance_reports_arrived();
    test_coincident_probe_and_destination_is_unresolved_not_a_fabricated_heading();
    test_arrival_check_precedes_coincidence_check();
    test_for_body_fails_closed_when_destination_not_registered();
    test_for_body_reuses_surface_range_to_body();

    std::puts("jose_autopilot_tests: all tests passed");
    return 0;
}
