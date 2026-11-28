#include "everward/simulation/surface_landing.hpp"
#include "everward/simulation/surface_descent_guidance.hpp"

#undef NDEBUG
#include <cassert>
#include <cmath>
#include <iostream>

using everward::simulation::altitude_limited_descent_speed_mps;
using everward::simulation::AltitudeAwareDescentProfile;
using everward::simulation::ControlledDescentEnvelope;
using everward::simulation::descent_speed_limit_mps;
using everward::simulation::kDefaultTakeoffMinimumIntegrity;
using everward::simulation::LandedState;
using everward::simulation::LandedThresholds;
using everward::simulation::SphericalPlanetaryBody;
using everward::simulation::takeoff_climb_rate_mps;
using everward::simulation::takeoff_refusal;
using everward::simulation::update_landed_state;
using everward::simulation::Vector3d;

static bool nearly(double a, double b, double eps = 1e-9) {
    return std::fabs(a - b) <= eps;
}

int main() {
    const double dt = 1.0 / 60.0;

    // The taper: full speed high up, the touchdown crawl near the surface, nothing at the floor.
    {
        const ControlledDescentEnvelope envelope{};   // 5 m/s max descent
        const AltitudeAwareDescentProfile profile{};  // full speed above 25 m, 0.5 m/s at touchdown
        assert(nearly(descent_speed_limit_mps(100.0, envelope, profile), 5.0));
        assert(nearly(descent_speed_limit_mps(25.0, envelope, profile), 5.0));
        assert(nearly(descent_speed_limit_mps(0.0, envelope, profile), 0.0));
        const double half = descent_speed_limit_mps(12.5, envelope, profile);
        assert(half > 0.5 && half < 5.0);
        assert(nearly(half, 0.5 + 4.5 * 0.5, 1e-9));
        // Monotonic: coming down never licenses going faster.
        double previous = 0.0;
        for (double altitude = 0.0; altitude <= 30.0; altitude += 0.5) {
            const double limit = descent_speed_limit_mps(altitude, envelope, profile);
            assert(limit >= previous - 1e-12);
            previous = limit;
        }
    }

    // It must agree with the sphere-based function already in the core, or there would be two descent rules.
    // Checked against a large body where a measured altitude and a radial one mean the same thing.
    {
        const ControlledDescentEnvelope envelope{};
        const AltitudeAwareDescentProfile profile{};
        SphericalPlanetaryBody body{};
        body.radius_m = 600000.0;
        for (const double altitude : {0.5, 3.0, 10.0, 24.0, 40.0}) {
            const Vector3d position{body.radius_m + altitude, 0.0, 0.0};
            const double from_body = altitude_limited_descent_speed_mps(position, body, envelope, profile);
            const double from_measurement = descent_speed_limit_mps(altitude, envelope, profile);
            assert(nearly(from_body, from_measurement, 1e-6));
        }
    }

    // A minimum clearance raises the floor: the envelope's clearance is subtracted before the taper, so a probe
    // told to stay 2 m off the surface stops descending at 2 m rather than at 0.
    {
        ControlledDescentEnvelope envelope{};
        envelope.minimum_clearance_m = 2.0;
        assert(nearly(descent_speed_limit_mps(2.0, envelope), 0.0));
        assert(descent_speed_limit_mps(3.0, envelope) > 0.0);
    }

    // An unknown altitude is not permission to fall.
    {
        assert(nearly(descent_speed_limit_mps(std::nan("")), 0.0));
        ControlledDescentEnvelope bad{};
        bad.max_descent_speed_mps = std::nan("");
        assert(nearly(descent_speed_limit_mps(50.0, bad), 0.0));
    }

    // Landing is held over time: one frame of contact is not a landing, settle_s of it is.
    {
        const LandedThresholds t{};
        LandedState s{};
        s = update_landed_state(s, true, 0.05, 0.0, dt, t);
        assert(!s.landed && s.settled_s > 0.0);
        for (int i = 0; i < 40; ++i) {
            s = update_landed_state(s, true, 0.05, 0.0, dt, t);
        }
        assert(s.landed);
        assert(s.settled_s >= t.settle_s);
    }

    // Sliding along the surface is not resting on it, and it resets the timer rather than merely pausing it.
    {
        const LandedThresholds t{};
        LandedState s{};
        for (int i = 0; i < 40; ++i) {
            s = update_landed_state(s, true, 0.05, 0.0, dt, t);
        }
        assert(s.landed);
        s = update_landed_state(s, true, 3.0, 0.0, dt, t);   // skidding
        assert(!s.landed);
        assert(nearly(s.settled_s, 0.0));
    }

    // A bouncing probe never accumulates a landing out of brief touches.
    {
        const LandedThresholds t{};
        LandedState s{};
        for (int i = 0; i < 100; ++i) {
            const bool touching = (i % 2) == 0;
            s = update_landed_state(s, touching, 0.05, 0.2, dt, t);
            assert(!s.landed);
        }
    }

    // Clear air ends a landing outright, which is what makes take-off observable rather than inferred.
    {
        const LandedThresholds t{};
        LandedState s{};
        for (int i = 0; i < 40; ++i) {
            s = update_landed_state(s, true, 0.0, 0.0, dt, t);
        }
        assert(s.landed);
        s = update_landed_state(s, false, 0.4, 5.0, dt, t);
        assert(!s.landed);
        assert(nearly(s.settled_s, 0.0));
        // Still inside the clearance band is neither landed nor flying, and does not resurrect the landing.
        LandedState band = update_landed_state(LandedState{true, 1.0}, false, 0.05, 0.4, dt, t);
        assert(!band.landed);
    }

    // Bad data neither invents nor revokes a landing.
    {
        const LandedState landed{true, 1.0};
        assert(update_landed_state(landed, true, std::nan(""), 0.0, dt).landed);
        assert(update_landed_state(landed, true, 0.0, 0.0, std::nan("")).landed);
        assert(update_landed_state(landed, true, 0.0, 0.0, -1.0).landed);
        const LandedState flying{};
        assert(!update_landed_state(flying, true, std::nan(""), 0.0, dt).landed);
    }

    // Take-off is refused below the floor, and the refusal says what to do about it rather than going silent.
    {
        assert(takeoff_refusal(1.0).empty());
        assert(takeoff_refusal(kDefaultTakeoffMinimumIntegrity).empty());   // exactly at the floor is enough
        assert(!takeoff_refusal(0.1).empty());
        assert(!takeoff_refusal(0.0).empty());
        assert(takeoff_refusal(0.0).find("Fix_It") != std::string::npos);
        assert(takeoff_refusal(0.1).find("Fix_It") != std::string::npos);
        assert(!takeoff_refusal(std::nan("")).empty());
    }

    // Climb authority scales with propulsion, and can never disagree with the refusal: zero exactly when refused.
    {
        assert(nearly(takeoff_climb_rate_mps(4.0, 1.0), 4.0));
        assert(nearly(takeoff_climb_rate_mps(4.0, 0.5), 2.0));
        assert(nearly(takeoff_climb_rate_mps(4.0, 0.0), 0.0));
        assert(nearly(takeoff_climb_rate_mps(4.0, 0.1), 0.0));   // refused, so no climb at all
        for (double integrity = 0.0; integrity <= 1.0; integrity += 0.01) {
            const bool refused = !takeoff_refusal(4.0 * 0.0 + integrity).empty();
            const double rate = takeoff_climb_rate_mps(4.0, integrity);
            assert(refused == nearly(rate, 0.0));
        }
        assert(nearly(takeoff_climb_rate_mps(std::nan(""), 1.0), 0.0));
        assert(nearly(takeoff_climb_rate_mps(-4.0, 1.0), 0.0));
    }

    std::cout << "Surface landing tests passed\n";
    return 0;
}
