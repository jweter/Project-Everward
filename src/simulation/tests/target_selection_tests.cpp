#include "everward/simulation/target_selection.hpp"

#undef NDEBUG
#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>

namespace {

using everward::simulation::StaticSphereBody;
using everward::simulation::TargetRangeTelemetry;
using everward::simulation::Vector3d;
using everward::simulation::closing_speed_to_body;
using everward::simulation::find_nearest_selectable_target;
using everward::simulation::select_target_telemetry;
using everward::simulation::surface_range_to_body;

bool nearly_equal(double a, double b, double epsilon = 1e-6) {
    return std::fabs(a - b) <= epsilon;
}

void test_surface_range_outside_body() {
    StaticSphereBody body;
    body.body_id = "asteroid-1";
    body.center_m = {10.0, 0.0, 0.0};
    body.radius_m = 2.0;

    const double range = surface_range_to_body({0.0, 0.0, 0.0}, body);
    assert(nearly_equal(range, 8.0));
}

void test_surface_range_clamped_when_inside_body() {
    StaticSphereBody body;
    body.body_id = "asteroid-1";
    body.center_m = {0.0, 0.0, 0.0};
    body.radius_m = 5.0;

    const double range = surface_range_to_body({1.0, 0.0, 0.0}, body);
    assert(nearly_equal(range, 0.0));
}

void test_closing_speed_positive_when_approaching() {
    StaticSphereBody body;
    body.body_id = "asteroid-1";
    body.center_m = {100.0, 0.0, 0.0};
    body.radius_m = 1.0;

    const double closing = closing_speed_to_body(
        {0.0, 0.0, 0.0}, {10.0, 0.0, 0.0}, body);
    assert(nearly_equal(closing, 10.0));
}

void test_closing_speed_negative_when_receding() {
    StaticSphereBody body;
    body.body_id = "asteroid-1";
    body.center_m = {100.0, 0.0, 0.0};
    body.radius_m = 1.0;

    const double closing = closing_speed_to_body(
        {0.0, 0.0, 0.0}, {-5.0, 0.0, 0.0}, body);
    assert(nearly_equal(closing, -5.0));
}

void test_closing_speed_zero_when_motion_perpendicular() {
    StaticSphereBody body;
    body.body_id = "asteroid-1";
    body.center_m = {100.0, 0.0, 0.0};
    body.radius_m = 1.0;

    const double closing = closing_speed_to_body(
        {0.0, 0.0, 0.0}, {0.0, 25.0, 0.0}, body);
    assert(nearly_equal(closing, 0.0));
}

void test_closing_speed_zero_when_point_at_body_center() {
    StaticSphereBody body;
    body.body_id = "asteroid-1";
    body.center_m = {5.0, 5.0, 5.0};
    body.radius_m = 1.0;

    const double closing = closing_speed_to_body(
        {5.0, 5.0, 5.0}, {10.0, 10.0, 10.0}, body);
    assert(nearly_equal(closing, 0.0));
}

void test_find_nearest_selectable_target_with_no_bodies() {
    const std::vector<StaticSphereBody> bodies;
    const auto result = find_nearest_selectable_target({0.0, 0.0, 0.0}, bodies, 100.0);
    assert(!result.has_value());
}

void test_find_nearest_selectable_target_excludes_out_of_range_bodies() {
    StaticSphereBody far_body;
    far_body.body_id = "far";
    far_body.center_m = {500.0, 0.0, 0.0};
    far_body.radius_m = 1.0;
    const std::vector<StaticSphereBody> bodies{far_body};

    const auto result = find_nearest_selectable_target({0.0, 0.0, 0.0}, bodies, 50.0);
    assert(!result.has_value());
}

void test_find_nearest_selectable_target_picks_closest() {
    StaticSphereBody near_body;
    near_body.body_id = "near";
    near_body.center_m = {10.0, 0.0, 0.0};
    near_body.radius_m = 1.0;

    StaticSphereBody far_body;
    far_body.body_id = "far";
    far_body.center_m = {40.0, 0.0, 0.0};
    far_body.radius_m = 1.0;

    const std::vector<StaticSphereBody> bodies{far_body, near_body};

    const auto result = find_nearest_selectable_target({0.0, 0.0, 0.0}, bodies, 100.0);
    assert(result.has_value());
    assert(result->body_id == "near");
    assert(nearly_equal(result->surface_range_m, 9.0));
}

void test_find_nearest_selectable_target_breaks_ties_toward_first_registered() {
    StaticSphereBody first;
    first.body_id = "first";
    first.center_m = {10.0, 0.0, 0.0};
    first.radius_m = 1.0;

    StaticSphereBody second;
    second.body_id = "second";
    second.center_m = {0.0, 10.0, 0.0};
    second.radius_m = 1.0;

    const std::vector<StaticSphereBody> bodies{first, second};

    const auto result = find_nearest_selectable_target({0.0, 0.0, 0.0}, bodies, 100.0);
    assert(result.has_value());
    assert(result->body_id == "first");
}

void test_select_target_telemetry_rejects_empty_id() {
    const std::vector<StaticSphereBody> bodies;
    const auto result = select_target_telemetry(
        "", bodies, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0});
    assert(!result.has_value());
}

void test_select_target_telemetry_rejects_unknown_id() {
    StaticSphereBody body;
    body.body_id = "known";
    body.center_m = {10.0, 0.0, 0.0};
    body.radius_m = 1.0;
    const std::vector<StaticSphereBody> bodies{body};

    const auto result = select_target_telemetry(
        "unknown", bodies, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0});
    assert(!result.has_value());
}

void test_select_target_telemetry_reports_range_and_closing_speed() {
    StaticSphereBody body;
    body.body_id = "known";
    body.center_m = {20.0, 0.0, 0.0};
    body.radius_m = 2.0;
    const std::vector<StaticSphereBody> bodies{body};

    const auto result = select_target_telemetry(
        "known", bodies, {0.0, 0.0, 0.0}, {4.0, 0.0, 0.0});
    assert(result.has_value());
    assert(result->body_id == "known");
    assert(nearly_equal(result->surface_range_m, 18.0));
    assert(nearly_equal(result->closing_speed_mps, 4.0));
}

} // namespace

int main() {
    test_surface_range_outside_body();
    test_surface_range_clamped_when_inside_body();
    test_closing_speed_positive_when_approaching();
    test_closing_speed_negative_when_receding();
    test_closing_speed_zero_when_motion_perpendicular();
    test_closing_speed_zero_when_point_at_body_center();
    test_find_nearest_selectable_target_with_no_bodies();
    test_find_nearest_selectable_target_excludes_out_of_range_bodies();
    test_find_nearest_selectable_target_picks_closest();
    test_find_nearest_selectable_target_breaks_ties_toward_first_registered();
    test_select_target_telemetry_rejects_empty_id();
    test_select_target_telemetry_rejects_unknown_id();
    test_select_target_telemetry_reports_range_and_closing_speed();

    std::puts("target_selection_tests: all tests passed");
    return 0;
}
