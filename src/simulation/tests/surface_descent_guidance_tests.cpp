#include "everward/simulation/surface_descent_guidance.hpp"

#undef NDEBUG
#include <cassert>
#include <cmath>
#include <cstdio>
#include <optional>

namespace {

using everward::simulation::AltitudeAwareDescentProfile;
using everward::simulation::ControlledDescentEnvelope;
using everward::simulation::SphericalPlanetaryBody;
using everward::simulation::altitude_limited_descent_speed_mps;
using everward::simulation::constrain_surface_approach_velocity;
using everward::simulation::controlled_descent_velocity_command;

bool nearly_equal(double a, double b, double epsilon = 1e-6) {
    return std::fabs(a - b) <= epsilon;
}

void test_limits_descent_and_tangential_rates_relative_to_moving_body() {
    const SphericalPlanetaryBody body{"moon", {10.0, 20.0, 30.0}, 100.0, {1.0, 2.0, 3.0}};
    const ControlledDescentEnvelope envelope{5.0, 2.0, 0.0};
    const auto command = constrain_surface_approach_velocity({210.0, 20.0, 30.0}, {-8.0, 8.0, 3.0}, body, envelope);
    assert(command.descent_rate_limited);
    assert(command.tangential_rate_limited);
    assert(nearly_equal(command.velocity_mps.x, -4.0));
    assert(nearly_equal(command.velocity_mps.y, 4.0));
    assert(nearly_equal(command.velocity_mps.z, 3.0));
}

void test_preserves_request_already_inside_controlled_descent_envelope() {
    const SphericalPlanetaryBody body{"moon", {}, 100.0, {}};
    const ControlledDescentEnvelope envelope{5.0, 2.0, 0.0};
    const auto command = constrain_surface_approach_velocity({200.0, 0.0, 0.0}, {-4.0, 1.5, 0.0}, body, envelope);
    assert(!command.descent_rate_limited);
    assert(!command.tangential_rate_limited);
    assert(nearly_equal(command.velocity_mps.x, -4.0));
    assert(nearly_equal(command.velocity_mps.y, 1.5));
    assert(nearly_equal(command.velocity_mps.z, 0.0));
}

void test_preserves_outward_motion_while_limiting_tangential_rate() {
    const SphericalPlanetaryBody body{"moon", {}, 100.0, {}};
    const ControlledDescentEnvelope envelope{5.0, 2.0, 0.0};
    const auto command = constrain_surface_approach_velocity({110.0, 0.0, 0.0}, {4.0, 3.0, 4.0}, body, envelope);
    assert(!command.descent_rate_limited);
    assert(command.tangential_rate_limited);
    assert(nearly_equal(command.velocity_mps.x, 4.0));
    assert(nearly_equal(command.velocity_mps.y, 1.2));
    assert(nearly_equal(command.velocity_mps.z, 1.6));
}

void test_tapers_descent_speed_as_clearance_is_approached() {
    const SphericalPlanetaryBody body{"moon", {}, 100.0, {}};
    const ControlledDescentEnvelope envelope{5.0, 2.0, 2.0};
    const AltitudeAwareDescentProfile profile{20.0, 0.5};
    assert(nearly_equal(altitude_limited_descent_speed_mps({122.0, 0.0, 0.0}, body, envelope, profile), 5.0));
    assert(nearly_equal(altitude_limited_descent_speed_mps({112.0, 0.0, 0.0}, body, envelope, profile), 2.75));
    assert(nearly_equal(altitude_limited_descent_speed_mps({102.0, 0.0, 0.0}, body, envelope, profile), 0.0));
    const auto command = constrain_surface_approach_velocity({112.0, 0.0, 0.0}, {-5.0, 0.0, 0.0}, body, envelope, profile);
    assert(command.descent_rate_limited);
    assert(nearly_equal(command.velocity_mps.x, -2.75));
}

void test_holds_inward_command_at_minimum_clearance() {
    const SphericalPlanetaryBody body{"moon", {}, 100.0, {1.0, 0.0, 0.0}};
    const ControlledDescentEnvelope envelope{5.0, 2.0, 2.0};
    const AltitudeAwareDescentProfile profile{20.0, 0.5};
    const auto command = constrain_surface_approach_velocity({102.0, 0.0, 0.0}, {-4.0, 1.0, 0.0}, body, envelope, profile);
    assert(command.descent_rate_limited);
    assert(!command.tangential_rate_limited);
    assert(nearly_equal(command.velocity_mps.x, 1.0));
    assert(nearly_equal(command.velocity_mps.y, 1.0));
    assert(nearly_equal(command.velocity_mps.z, 0.0));
}

void test_holds_at_planet_scale_clearance_despite_radial_roundoff() {
    constexpr double radius_m = 6'371'000.0;
    constexpr double clearance_m = 2.0;
    const SphericalPlanetaryBody body{"earth-scale", {}, radius_m, {}};
    const ControlledDescentEnvelope envelope{5.0, 2.0, clearance_m};
    const AltitudeAwareDescentProfile profile{20.0, 0.5};
    const double component = (radius_m + clearance_m) / std::sqrt(3.0);
    const auto command = constrain_surface_approach_velocity(
        {component, component, component}, {-1.0, -1.0, -1.0}, body, envelope, profile
    );
    assert(command.descent_rate_limited);
    const double radial_speed = (
        command.velocity_mps.x + command.velocity_mps.y + command.velocity_mps.z
    ) / std::sqrt(3.0);
    assert(nearly_equal(radial_speed, 0.0));
}

void test_profile_never_increases_envelope_descent_limit() {
    const SphericalPlanetaryBody body{"moon", {}, 100.0, {}};
    const ControlledDescentEnvelope envelope{3.0, 2.0, 0.0};
    const AltitudeAwareDescentProfile profile{10.0, 20.0};
    assert(nearly_equal(altitude_limited_descent_speed_mps({100.0, 0.0, 0.0}, body, envelope, profile), 0.0));
    assert(nearly_equal(altitude_limited_descent_speed_mps({110.0, 0.0, 0.0}, body, envelope, profile), 3.0));
}

void test_controlled_descent_command_fails_closed_with_no_registered_body() {
    const std::optional<SphericalPlanetaryBody> no_body;
    const auto command = controlled_descent_velocity_command({210.0, 20.0, 30.0}, {-8.0, 8.0, 3.0}, no_body);
    assert(!command.has_value());
}

void test_controlled_descent_command_matches_direct_call_with_registered_body() {
    const SphericalPlanetaryBody body{"moon", {10.0, 20.0, 30.0}, 100.0, {1.0, 2.0, 3.0}};
    const ControlledDescentEnvelope envelope{5.0, 2.0, 0.0};
    const std::optional<SphericalPlanetaryBody> registered = body;
    const auto wrapped = controlled_descent_velocity_command({210.0, 20.0, 30.0}, {-8.0, 8.0, 3.0}, registered, envelope);
    const auto direct = constrain_surface_approach_velocity({210.0, 20.0, 30.0}, {-8.0, 8.0, 3.0}, body, envelope);
    assert(wrapped.has_value());
    assert(wrapped->descent_rate_limited == direct.descent_rate_limited);
    assert(wrapped->tangential_rate_limited == direct.tangential_rate_limited);
    assert(nearly_equal(wrapped->velocity_mps.x, direct.velocity_mps.x));
    assert(nearly_equal(wrapped->velocity_mps.y, direct.velocity_mps.y));
    assert(nearly_equal(wrapped->velocity_mps.z, direct.velocity_mps.z));
}

} // namespace

int main() {
    test_limits_descent_and_tangential_rates_relative_to_moving_body();
    test_preserves_request_already_inside_controlled_descent_envelope();
    test_preserves_outward_motion_while_limiting_tangential_rate();
    test_tapers_descent_speed_as_clearance_is_approached();
    test_holds_inward_command_at_minimum_clearance();
    test_holds_at_planet_scale_clearance_despite_radial_roundoff();
    test_profile_never_increases_envelope_descent_limit();
    test_controlled_descent_command_fails_closed_with_no_registered_body();
    test_controlled_descent_command_matches_direct_call_with_registered_body();
    std::puts("surface_descent_guidance_tests: all tests passed");
    return 0;
}
