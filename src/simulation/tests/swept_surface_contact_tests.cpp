#include "everward/simulation/planetary_body.hpp"

#undef NDEBUG
#include <cassert>
#include <cmath>

int main() {
    using namespace everward::simulation;
    SphericalPlanetaryBody body{"moon", {}, 100.0, {}};

    const auto tunneled = resolve_swept_surface_contact(
        {101.0, 0.0, 0.0}, {-101.0, 0.0, 0.0}, {-202.0, 0.0, 0.0}, body);
    assert(tunneled.corrected);
    assert(std::fabs(tunneled.position_m.x - 100.0) < 1e-6);
    assert(std::fabs(tunneled.position_m.y) < 1e-6);
    assert(std::fabs(tunneled.velocity_mps.x) < 1e-6);

    const auto miss = resolve_swept_surface_contact(
        {101.0, 101.0, 0.0}, {-101.0, 101.0, 0.0}, {-202.0, 0.0, 0.0}, body);
    assert(!miss.corrected);
    assert(std::fabs(miss.position_m.x + 101.0) < 1e-6);

    return 0;
}
