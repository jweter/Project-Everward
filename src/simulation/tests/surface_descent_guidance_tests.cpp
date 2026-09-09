#include "everward/simulation/surface_descent_guidance.hpp"

#undef NDEBUG
#include <cassert>
#include <cmath>
#include <cstdio>

namespace {

using everward::simulation::ControlledDescentEnvelope;
using everward::simulation::SphericalPlanetaryBody;
using everward::simulation::constrain_surface_approach_velocity;

bool nearly_equal(double a, double b, double epsilon = 1e-6) {
    return std::fabs(a - b) <= epsilon;
}

void test_limits_descent_and_tangential_rates_relative_to_moving_body() {
    const SphericalPlanetaryBody body{
        "moon",
        {10.0, 20.0, 30.0},
        100.0,
        {1.0, 2.0, 3.0},
    };
    const ControlledDescentEnvelope envelope{5.0, 2.0, 0.0};

    const auto command = constrain_surface_approach_velocity(
        {110.0, 20.0, 30.0},
        {-8.0, 8.0, 3.0},
        body,
        envelope
    );

    assert(command.descent_rate_limited);
    assert(command.tangential_rate_limited);
    assert(nearly_equal(command.velocity_mps.x, -4.0));
    assert(nearly_equal(command.velocity_mps.y, 4.0));
    assert(nearly_equal(command.velocity_mps.z, 3.0));
}

void test_preserves_request_already_inside_controlled_descent_envelope() {
    const SphericalPlanetaryBody body{"moon", {}, 100.0, {}};
    const ControlledDescentEnvelope envelope{5.0, 2.0, 0.0};

    const auto command = constrain_surface_approach_velocity(
        {110.0, 0.0, 0.0},
        {-4.0, 1.5, 0.0},
        body,
        envelope
    );

    assert(!command.descent_rate_limited);
    assert(!command.tangential_rate_limited);
    assert(nearly_equal(command.velocity_mps.x, -4.0));
    assert(nearly_equal(command.velocity_mps.y, 1.5));
    assert(nearly_equal(command.velocity_mps.z, 0.0));
}

void test_preserves_outward_motion_while_limiting_tangential_rate() {
    const SphericalPlanetaryBody body{"moon", {}, 100.0, {}};
    const ControlledDescentEnvelope envelope{5.0, 2.0, 0.0};

    const auto command = constrain_surface_approach_velocity(
        {110.0, 0.0, 0.0},
        {4.0, 3.0, 4.0},
        body,
        envelope
    );

    assert(!command.descent_rate_limited);
    assert(command.tangential_rate_limited);
    assert(nearly_equal(command.velocity_mps.x, 4.0));
    assert(nearly_equal(command.velocity_mps.y, 1.2));
    assert(nearly_equal(command.velocity_mps.z, 1.6));
}

} // namespace

int main() {
    test_limits_descent_and_tangential_rates_relative_to_moving_body();
    test_preserves_request_already_inside_controlled_descent_envelope();
    test_preserves_outward_motion_while_limiting_tangential_rate();

    std::puts("surface_descent_guidance_tests: all tests passed");
    return 0;
}
