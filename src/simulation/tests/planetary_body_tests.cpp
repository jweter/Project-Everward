#include "everward/simulation/planetary_body.hpp"

#undef NDEBUG
#include <cassert>
#include <cmath>
#include <cstdio>

namespace {

using everward::simulation::ControlledDescentEnvelope;
using everward::simulation::PlanetaryLocalFrame;
using everward::simulation::SphericalPlanetaryBody;
using everward::simulation::SurfaceApproachState;
using everward::simulation::SurfaceRelativeMotion;
using everward::simulation::Vector3d;
using everward::simulation::altitude_above_reference_surface;
using everward::simulation::body_relative_velocity;
using everward::simulation::classify_surface_approach;
using everward::simulation::gravitational_acceleration;
using everward::simulation::is_below_reference_surface;
using everward::simulation::local_horizon_frame;
using everward::simulation::local_surface_normal;
using everward::simulation::surface_relative_motion;

bool nearly_equal(double a, double b, double epsilon = 1e-6) {
    return std::fabs(a - b) <= epsilon;
}

double dot(Vector3d a, Vector3d b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

double magnitude(Vector3d value) {
    return std::sqrt(dot(value, value));
}

void test_altitude_above_surface() {
    SphericalPlanetaryBody body{"moon", {0.0, 0.0, 0.0}, 1000.0, {}};
    assert(nearly_equal(altitude_above_reference_surface({1250.0, 0.0, 0.0}, body), 250.0));
}

void test_altitude_is_negative_below_surface() {
    SphericalPlanetaryBody body{"moon", {10.0, 0.0, 0.0}, 100.0, {}};
    assert(nearly_equal(altitude_above_reference_surface({60.0, 0.0, 0.0}, body), -50.0));
    assert(is_below_reference_surface({60.0, 0.0, 0.0}, body));
}

void test_surface_normal_points_outward_from_offset_body_center() {
    SphericalPlanetaryBody body{"moon", {100.0, 50.0, -20.0}, 25.0, {}};
    const Vector3d normal = local_surface_normal({100.0, 80.0, -20.0}, body);
    assert(nearly_equal(normal.x, 0.0));
    assert(nearly_equal(normal.y, 1.0));
    assert(nearly_equal(normal.z, 0.0));
}

void test_local_horizon_frame_is_orthonormal() {
    SphericalPlanetaryBody body{"moon", {0.0, 0.0, 0.0}, 100.0, {}};
    const PlanetaryLocalFrame frame = local_horizon_frame({100.0, 100.0, 50.0}, body);

    assert(nearly_equal(magnitude(frame.up), 1.0));
    assert(nearly_equal(magnitude(frame.east), 1.0));
    assert(nearly_equal(magnitude(frame.north), 1.0));
    assert(nearly_equal(dot(frame.up, frame.east), 0.0));
    assert(nearly_equal(dot(frame.up, frame.north), 0.0));
    assert(nearly_equal(dot(frame.east, frame.north), 0.0));
}

void test_local_horizon_frame_is_stable_at_pole() {
    SphericalPlanetaryBody body{"moon", {0.0, 0.0, 0.0}, 100.0, {}};
    const PlanetaryLocalFrame frame = local_horizon_frame({0.0, 0.0, 100.0}, body);

    assert(nearly_equal(frame.up.z, 1.0));
    assert(nearly_equal(magnitude(frame.east), 1.0));
    assert(nearly_equal(magnitude(frame.north), 1.0));
}

void test_local_horizon_frame_does_not_snap_near_old_pole_threshold() {
    SphericalPlanetaryBody body{"moon", {0.0, 0.0, 0.0}, 100.0, {}};
    const PlanetaryLocalFrame below = local_horizon_frame({4.48, 0.0, 99.8996}, body);
    const PlanetaryLocalFrame above = local_horizon_frame({4.46, 0.0, 99.9005}, body);

    // These nearby positions straddle the former abs(up.z) > 0.999 switch.
    // A continuous tangent construction keeps corresponding axes aligned.
    assert(dot(below.east, above.east) > 0.999);
    assert(dot(below.north, above.north) > 0.999);
}

void test_body_relative_velocity_subtracts_body_motion() {
    SphericalPlanetaryBody body{"moon", {}, 100.0, {2.0, -3.0, 4.0}};
    const Vector3d relative = body_relative_velocity({12.0, 7.0, -1.0}, body);
    assert(nearly_equal(relative.x, 10.0));
    assert(nearly_equal(relative.y, 10.0));
    assert(nearly_equal(relative.z, -5.0));
}

void test_gravity_points_toward_body_center_with_inverse_square_magnitude() {
    SphericalPlanetaryBody body{"moon", {}, 100.0, {}, 400.0};
    const Vector3d near_acceleration = gravitational_acceleration({10.0, 0.0, 0.0}, body);
    const Vector3d far_acceleration = gravitational_acceleration({20.0, 0.0, 0.0}, body);

    assert(nearly_equal(near_acceleration.x, -4.0));
    assert(nearly_equal(near_acceleration.y, 0.0));
    assert(nearly_equal(near_acceleration.z, 0.0));
    assert(nearly_equal(magnitude(far_acceleration), magnitude(near_acceleration) / 4.0));
}

void test_gravity_defaults_to_zero_and_is_safe_at_body_center() {
    SphericalPlanetaryBody disabled{"moon", {}, 100.0, {}};
    const Vector3d disabled_acceleration = gravitational_acceleration({10.0, 0.0, 0.0}, disabled);
    assert(nearly_equal(magnitude(disabled_acceleration), 0.0));

    SphericalPlanetaryBody enabled{"moon", {}, 100.0, {}, 400.0};
    const Vector3d center_acceleration = gravitational_acceleration({}, enabled);
    assert(nearly_equal(magnitude(center_acceleration), 0.0));
}

void test_surface_relative_motion_splits_vertical_and_tangential_velocity() {
    SphericalPlanetaryBody body{"moon", {}, 100.0, {1.0, 2.0, 3.0}};
    const SurfaceRelativeMotion motion = surface_relative_motion(
        {100.0, 0.0, 0.0},
        {6.0, 14.0, 3.0},
        body);

    assert(nearly_equal(motion.body_relative_velocity_mps.x, 5.0));
    assert(nearly_equal(motion.body_relative_velocity_mps.y, 12.0));
    assert(nearly_equal(motion.body_relative_velocity_mps.z, 0.0));
    assert(nearly_equal(motion.vertical_speed_mps, 5.0));
    assert(nearly_equal(motion.vertical_velocity_mps.x, 5.0));
    assert(nearly_equal(motion.vertical_velocity_mps.y, 0.0));
    assert(nearly_equal(motion.tangential_velocity_mps.x, 0.0));
    assert(nearly_equal(motion.tangential_velocity_mps.y, 12.0));
    assert(nearly_equal(motion.tangential_speed_mps, 12.0));
    assert(nearly_equal(
        dot(motion.vertical_velocity_mps, motion.tangential_velocity_mps),
        0.0));
}

void test_surface_relative_motion_preserves_descent_sign() {
    SphericalPlanetaryBody body{"moon", {}, 100.0, {}};
    const SurfaceRelativeMotion motion = surface_relative_motion(
        {0.0, 100.0, 0.0},
        {3.0, -7.0, 4.0},
        body);

    assert(nearly_equal(motion.vertical_speed_mps, -7.0));
    assert(nearly_equal(motion.vertical_velocity_mps.y, -7.0));
    assert(nearly_equal(motion.tangential_velocity_mps.x, 3.0));
    assert(nearly_equal(motion.tangential_velocity_mps.y, 0.0));
    assert(nearly_equal(motion.tangential_velocity_mps.z, 4.0));
    assert(nearly_equal(motion.tangential_speed_mps, 5.0));
}

void test_surface_approach_classifies_controlled_descent() {
    SphericalPlanetaryBody body{"moon", {}, 100.0, {}};
    const ControlledDescentEnvelope envelope{5.0, 2.0, 0.0};
    assert(classify_surface_approach({110.0, 0.0, 0.0}, {-4.0, 1.0, 0.0}, body, envelope)
        == SurfaceApproachState::ControlledDescent);
}

void test_surface_approach_rejects_excessive_descent_rate() {
    SphericalPlanetaryBody body{"moon", {}, 100.0, {}};
    const ControlledDescentEnvelope envelope{5.0, 2.0, 0.0};
    assert(classify_surface_approach({110.0, 0.0, 0.0}, {-6.0, 0.0, 0.0}, body, envelope)
        == SurfaceApproachState::ExcessiveDescentRate);
}

void test_surface_approach_rejects_excessive_tangential_rate() {
    SphericalPlanetaryBody body{"moon", {}, 100.0, {}};
    const ControlledDescentEnvelope envelope{5.0, 2.0, 0.0};
    assert(classify_surface_approach({110.0, 0.0, 0.0}, {-2.0, 3.0, 0.0}, body, envelope)
        == SurfaceApproachState::ExcessiveTangentialRate);
}

void test_surface_approach_detects_clearance_violation() {
    SphericalPlanetaryBody body{"moon", {}, 100.0, {}};
    const ControlledDescentEnvelope envelope{5.0, 2.0, 1.0};
    assert(classify_surface_approach({100.5, 0.0, 0.0}, {0.0, 0.0, 0.0}, body, envelope)
        == SurfaceApproachState::SurfacePenetration);
}

} // namespace

int main() {
    test_altitude_above_surface();
    test_altitude_is_negative_below_surface();
    test_surface_normal_points_outward_from_offset_body_center();
    test_local_horizon_frame_is_orthonormal();
    test_local_horizon_frame_is_stable_at_pole();
    test_local_horizon_frame_does_not_snap_near_old_pole_threshold();
    test_body_relative_velocity_subtracts_body_motion();
    test_gravity_points_toward_body_center_with_inverse_square_magnitude();
    test_gravity_defaults_to_zero_and_is_safe_at_body_center();
    test_surface_relative_motion_splits_vertical_and_tangential_velocity();
    test_surface_relative_motion_preserves_descent_sign();
    test_surface_approach_classifies_controlled_descent();
    test_surface_approach_rejects_excessive_descent_rate();
    test_surface_approach_rejects_excessive_tangential_rate();
    test_surface_approach_detects_clearance_violation();

    std::puts("planetary_body_tests: all tests passed");
    return 0;
}
