#include "everward/simulation/target_selection.hpp"

#undef NDEBUG
#include <cassert>
#include <cstdio>
#include <vector>

namespace {

using everward::simulation::StaticSphereBody;
using everward::simulation::Vector3d;
using everward::simulation::find_next_selectable_target;

StaticSphereBody body(const char* id, double x, double y = 0.0) {
    return StaticSphereBody{id, {x, y, 0.0}, 1.0};
}

void test_cycle_starts_at_nearest_when_nothing_selected() {
    const std::vector<StaticSphereBody> bodies{body("far", 40.0), body("near", 10.0)};
    const auto next = find_next_selectable_target(
        {0.0, 0.0, 0.0}, {2.0, 0.0, 0.0}, bodies, 100.0, "");
    assert(next.has_value());
    assert(next->body_id == "near");
    assert(next->closing_speed_mps == 2.0);
}

void test_cycle_uses_nearest_to_farthest_order_and_wraps() {
    const std::vector<StaticSphereBody> bodies{
        body("middle", 20.0), body("far", 30.0), body("near", 10.0)};
    const Vector3d position{0.0, 0.0, 0.0};
    const Vector3d velocity{0.0, 0.0, 0.0};

    const auto after_near = find_next_selectable_target(position, velocity, bodies, 100.0, "near");
    assert(after_near.has_value() && after_near->body_id == "middle");
    const auto after_middle = find_next_selectable_target(position, velocity, bodies, 100.0, "middle");
    assert(after_middle.has_value() && after_middle->body_id == "far");
    const auto after_far = find_next_selectable_target(position, velocity, bodies, 100.0, "far");
    assert(after_far.has_value() && after_far->body_id == "near");
}

void test_cycle_preserves_registration_order_for_equal_ranges() {
    const std::vector<StaticSphereBody> bodies{body("first", 10.0), body("second", 0.0, 10.0)};
    const auto next = find_next_selectable_target(
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, bodies, 100.0, "first");
    assert(next.has_value());
    assert(next->body_id == "second");
}

void test_cycle_fails_closed_and_ignores_out_of_range_current_target() {
    const std::vector<StaticSphereBody> bodies{body("eligible", 10.0), body("too-far", 500.0)};
    const auto recovered = find_next_selectable_target(
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, bodies, 100.0, "too-far");
    assert(recovered.has_value());
    assert(recovered->body_id == "eligible");

    const auto none = find_next_selectable_target(
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, bodies, 1.0, "eligible");
    assert(!none.has_value());
}

} // namespace

int main() {
    test_cycle_starts_at_nearest_when_nothing_selected();
    test_cycle_uses_nearest_to_farthest_order_and_wraps();
    test_cycle_preserves_registration_order_for_equal_ranges();
    test_cycle_fails_closed_and_ignores_out_of_range_current_target();
    std::puts("target_cycle_tests: all tests passed");
    return 0;
}
