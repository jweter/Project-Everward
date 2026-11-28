#include "everward/simulation/solar_power.hpp"

#undef NDEBUG
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>

using everward::simulation::EulerAttitudeDegrees;
using everward::simulation::SolarArray;
using everward::simulation::SolarEnvironment;
using everward::simulation::mark1_solar_array;
using everward::simulation::solar_capacity_w;
using everward::simulation::solar_generation_w;
using everward::simulation::solar_incidence_cosine;

// A fitted probe under a star: what a scenario configures, never a default.
static SolarEnvironment lit() {
    SolarEnvironment env{};
    env.irradiance_w_per_m2 = everward::simulation::kSolarConstantWPerM2;
    return env;
}

static bool nearly(double a, double b, double eps = 1e-9) {
    return std::fabs(a - b) <= eps;
}

int main() {
    const EulerAttitudeDegrees level{0.0, 0.0, 0.0};

    // Square to the star, undamaged, nothing in the way: the full product of the factors and nothing else.
    {
        SolarArray array = mark1_solar_array();          // 6 m^2 at 22%
        SolarEnvironment env = lit();      // 1361 W/m^2, sun straight up (+z), clear
        const double expected = 1361.0 * 6.0 * 0.22;
        assert(nearly(solar_generation_w(array, env, level), expected, 1e-6));
        assert(nearly(solar_capacity_w(array, env), expected, 1e-6));
        assert(nearly(solar_incidence_cosine(array, env, level), 1.0));
    }

    // Damage scales generation and nothing else. This is the rule the rest of the probe follows -- thrust by
    // propulsion, scan time by sensors -- so a half-wrecked array is worth half the watts.
    {
        SolarArray array = mark1_solar_array();
        SolarEnvironment env = lit();
        const double full = solar_generation_w(array, env, level);
        array.integrity = 0.5;
        assert(nearly(solar_generation_w(array, env, level), full * 0.5, 1e-6));
        array.integrity = 0.0;
        assert(nearly(solar_generation_w(array, env, level), 0.0));
        // The array's best possible output does not fall just because it is damaged: that is what tells a player
        // the difference between a broken panel and a badly aimed one.
        assert(solar_capacity_w(array, env) > 0.0);
    }

    // Area is the trait with no ceiling, and it scales linearly: doubling the panels doubles the power. This is
    // the owner's "always increasable" requirement expressed as arithmetic, so no upgrade can hit a wall here.
    {
        SolarEnvironment env = lit();
        SolarArray small = mark1_solar_array();
        SolarArray large = mark1_solar_array();
        large.area_m2 = small.area_m2 * 2.0;
        assert(nearly(solar_generation_w(large, env, level), solar_generation_w(small, env, level) * 2.0, 1e-6));
        SolarArray huge = mark1_solar_array();
        huge.area_m2 = 1.0e6;
        assert(solar_generation_w(huge, env, level) > solar_generation_w(large, env, level));
        assert(std::isfinite(solar_generation_w(huge, env, level)));
    }

    // Orientation matters: edge-on and facing away both generate nothing, and never a negative figure. A panel
    // turned away from the star must not quietly drain the reserve.
    {
        SolarArray array = mark1_solar_array();
        SolarEnvironment env = lit();
        const EulerAttitudeDegrees edge_on{0.0, 90.0, 0.0};   // pitch the +z face to the horizon
        assert(nearly(solar_generation_w(array, env, edge_on), 0.0, 1e-9));
        const EulerAttitudeDegrees upside_down{0.0, 180.0, 0.0};
        assert(nearly(solar_generation_w(array, env, upside_down), 0.0));
        assert(solar_generation_w(array, env, upside_down) >= 0.0);
        // Half-turned is the cosine, not a cliff.
        const EulerAttitudeDegrees tilted{0.0, 60.0, 0.0};
        const double cosine = solar_incidence_cosine(array, env, tilted);
        assert(cosine > 0.49 && cosine < 0.51);
    }

    // Stowed panels catch nothing however perfect the conditions, and shadow scales smoothly to nothing. Raising
    // occlusion is how night will arrive, so it has to be a factor rather than a special case.
    {
        SolarArray array = mark1_solar_array();
        SolarEnvironment env = lit();
        array.deployed = false;
        assert(nearly(solar_generation_w(array, env, level), 0.0));
        array.deployed = true;
        const double full = solar_generation_w(array, env, level);
        env.occlusion = 0.5;
        assert(nearly(solar_generation_w(array, env, level), full * 0.5, 1e-6));
        env.occlusion = 1.0;
        assert(nearly(solar_generation_w(array, env, level), 0.0));
        env.occlusion = 0.0;
        env.irradiance_w_per_m2 = 0.0;   // no star in reach
        assert(nearly(solar_generation_w(array, env, level), 0.0));
    }

    // Nonsense inputs produce no power rather than impossible power. Inventing energy from bad data is the one
    // failure here that would matter, because the energy balance is what keeps the probe alive.
    {
        SolarEnvironment env = lit();
        SolarArray bad = mark1_solar_array();
        bad.area_m2 = std::nan("");
        assert(nearly(solar_generation_w(bad, env, level), 0.0));
        bad = mark1_solar_array();
        bad.efficiency = std::numeric_limits<double>::infinity();
        assert(nearly(solar_generation_w(bad, env, level), 0.0));   // rejected, not clamped into free power
        bad = mark1_solar_array();
        bad.area_m2 = -5.0;
        assert(nearly(solar_generation_w(bad, env, level), 0.0));
        bad = mark1_solar_array();
        bad.integrity = 2.0;   // clamped, never a bonus
        SolarArray healthy = mark1_solar_array();
        assert(nearly(solar_generation_w(bad, env, level), solar_generation_w(healthy, env, level), 1e-6));
        SolarEnvironment bad_env = lit();
        bad_env.sun_direction = {0.0, 0.0, 0.0};
        assert(nearly(solar_generation_w(healthy, bad_env, level), 0.0));
        bad_env = lit();
        bad_env.occlusion = -3.0;   // clamped, never a bonus
        assert(nearly(solar_generation_w(healthy, bad_env, level), solar_generation_w(healthy, env, level), 1e-6));
        bad_env = lit();
        bad_env.irradiance_w_per_m2 = std::nan("");
        assert(nearly(solar_generation_w(healthy, bad_env, level), 0.0));
    }

    std::cout << "Solar power tests passed\n";
    return 0;
}
