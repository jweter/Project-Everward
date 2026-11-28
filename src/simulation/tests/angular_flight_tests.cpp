#include "everward/simulation/angular_flight.hpp"

#undef NDEBUG
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>

using everward::simulation::advance_rotation;
using everward::simulation::AngularRateDegPerS;
using everward::simulation::angular_speed_deg_per_s;
using everward::simulation::arrest_rotation;
using everward::simulation::RotationAuthority;
using everward::simulation::RotationCommand;
using everward::simulation::RotationModel;

static bool nearly(double a, double b, double eps = 1e-9) {
    return std::fabs(a - b) <= eps;
}

int main() {
    const double dt = 1.0 / 60.0;

    // Rate: a held input accelerates, and the spin PERSISTS after the input stops. This is the whole point --
    // today's probe stops rotating the instant input stops, which is why it feels like a number and not a machine.
    {
        RotationAuthority a{};
        a.model = RotationModel::Rate;
        a.authority = 60.0;      // deg/s^2
        a.damping_per_s = 0.0;   // a vacuum does not slow you down
        AngularRateDegPerS rate{};
        for (int i = 0; i < 60; ++i) {
            rate = advance_rotation(rate, RotationCommand{.yaw = 1.0}, a, dt).rate;
        }
        assert(rate.yaw > 59.0 && rate.yaw < 61.0);   // ~1 s at 60 deg/s^2
        // Let go: the spin is still there.
        const auto coast = advance_rotation(rate, RotationCommand{}, a, dt);
        assert(nearly(coast.rate.yaw, rate.yaw, 1e-9));
        assert(coast.attitude_delta.yaw > 0.0);
        assert(nearly(coast.attitude_delta.roll, 0.0));
        assert(nearly(coast.attitude_delta.pitch, 0.0));
    }

    // Damping bleeds a spin off when the scheme asks for it: this is the difference between Descent/KSP holding
    // their spin and an assisted scheme snapping to a stop.
    {
        RotationAuthority a{};
        a.authority = 60.0;
        a.damping_per_s = 4.0;
        AngularRateDegPerS rate{.yaw = 90.0};
        const auto step = advance_rotation(rate, RotationCommand{}, a, dt);
        assert(step.rate.yaw < 90.0 && step.rate.yaw > 0.0);
        AngularRateDegPerS settling = rate;
        for (int i = 0; i < 600; ++i) {
            settling = advance_rotation(settling, RotationCommand{}, a, dt).rate;
        }
        assert(std::fabs(settling.yaw) < 0.5);   // comes to rest, rather than hanging at some floor
    }

    // Holding an input must beat damping rather than fight to a standstill: damping applies to the spin that was
    // already there, not to the command being given.
    {
        RotationAuthority a{};
        a.authority = 60.0;
        a.damping_per_s = 4.0;
        AngularRateDegPerS rate{};
        for (int i = 0; i < 120; ++i) {
            rate = advance_rotation(rate, RotationCommand{.yaw = 1.0}, a, dt).rate;
        }
        assert(rate.yaw > 5.0);   // reaches a real sustained rate, not nearly zero
    }

    // The rate cap is a cap, flagged when it bites, and it scales with propulsion like everything else.
    {
        RotationAuthority a{};
        a.authority = 500.0;
        a.rate_max_deg_per_s = 45.0;
        AngularRateDegPerS rate{};
        bool ever_limited = false;
        for (int i = 0; i < 60; ++i) {
            const auto step = advance_rotation(rate, RotationCommand{.pitch = 1.0}, a, dt);
            rate = step.rate;
            ever_limited = ever_limited || step.rate_limited;
        }
        assert(ever_limited);
        assert(nearly(rate.pitch, 45.0, 1e-9));

        a.propulsion_effectiveness = 0.4;   // a damaged probe turns slower AND cannot turn as fast
        AngularRateDegPerS damaged{};
        for (int i = 0; i < 120; ++i) {
            damaged = advance_rotation(damaged, RotationCommand{.pitch = 1.0}, a, dt).rate;
        }
        assert(nearly(damaged.pitch, 18.0, 1e-9));   // 45 x 0.4
    }

    // Propulsion damage scales authority in Direct mode too: no scheme is a way around the damage model, which is
    // exactly the hole the autopilot had (#461).
    {
        RotationAuthority a{};
        a.model = RotationModel::Direct;
        a.authority = 12.0;
        const auto healthy = advance_rotation(AngularRateDegPerS{}, RotationCommand{.yaw = 1.0}, a, dt);
        assert(nearly(healthy.attitude_delta.yaw, 12.0));
        assert(nearly(healthy.rate.yaw, 0.0));   // Direct keeps no rate, by definition
        a.propulsion_effectiveness = 0.4;
        const auto damaged = advance_rotation(AngularRateDegPerS{}, RotationCommand{.yaw = 1.0}, a, dt);
        assert(nearly(damaged.attitude_delta.yaw, 4.8));   // the 4.8 degrees the mouse already gives at 40%
    }

    // A scheme without roll neither accepts roll input nor leaves an inherited spin running: switching scheme
    // mid-flight must not leave the probe rolling with no way to stop it.
    {
        RotationAuthority a{};
        a.roll_enabled = false;
        a.authority = 60.0;
        const auto step = advance_rotation(
            AngularRateDegPerS{.roll = 120.0}, RotationCommand{.roll = 1.0}, a, dt);
        assert(nearly(step.rate.roll, 0.0));
        assert(nearly(step.attitude_delta.roll, 0.0));
    }

    // Input beyond the -1..1 contract is clamped, not amplified: a scheme cannot buy extra authority by sending 5.
    {
        RotationAuthority a{};
        a.model = RotationModel::Direct;
        a.authority = 10.0;
        const auto over = advance_rotation(AngularRateDegPerS{}, RotationCommand{.yaw = 5.0}, a, dt);
        assert(nearly(over.attitude_delta.yaw, 10.0));
        const auto under = advance_rotation(AngularRateDegPerS{}, RotationCommand{.yaw = -5.0}, a, dt);
        assert(nearly(under.attitude_delta.yaw, -10.0));
    }

    // Nonsense in, nothing applied. A runaway or NaN spin would corrupt attitude, the hull sweep and the contact
    // model all at once, so this fails closed and says it did.
    {
        RotationAuthority a{};
        const auto nan_cmd = advance_rotation(
            AngularRateDegPerS{}, RotationCommand{.yaw = std::nan("")}, a, dt);
        assert(nan_cmd.rejected);
        assert(nearly(angular_speed_deg_per_s(nan_cmd.rate), 0.0));
        const auto nan_rate = advance_rotation(
            AngularRateDegPerS{.yaw = std::nan("")}, RotationCommand{}, a, dt);
        assert(nan_rate.rejected);
        assert(nearly(angular_speed_deg_per_s(nan_rate.rate), 0.0));
        RotationAuthority bad{};
        bad.authority = std::numeric_limits<double>::infinity();
        assert(advance_rotation(AngularRateDegPerS{}, RotationCommand{.yaw = 1.0}, bad, dt).rejected);
        assert(advance_rotation(AngularRateDegPerS{}, RotationCommand{}, a, std::nan("")).rejected);
        assert(advance_rotation(AngularRateDegPerS{}, RotationCommand{}, a, -1.0).rejected);
    }

    // A zero-length step changes nothing but is not an error: fixed-step engines pass 0 at the edges.
    {
        RotationAuthority a{};
        a.authority = 60.0;
        const AngularRateDegPerS spinning{.yaw = 30.0};
        const auto step = advance_rotation(spinning, RotationCommand{.yaw = 1.0}, a, 0.0);
        assert(!step.rejected);
        assert(nearly(step.rate.yaw, 30.0));
        assert(nearly(step.attitude_delta.yaw, 0.0));
    }

    // Arresting a spin is deliberate and reaches exactly zero rather than approaching it forever, so a stability
    // assist can actually finish.
    {
        AngularRateDegPerS rate{.yaw = 5.0, .pitch = -20.0, .roll = 10.0};
        for (int i = 0; i < 600; ++i) {
            rate = arrest_rotation(rate, 90.0, dt);
        }
        assert(nearly(angular_speed_deg_per_s(rate), 0.0));
        // It never overshoots into a spin the other way.
        AngularRateDegPerS small{.roll = 0.1};
        small = arrest_rotation(small, 90.0, dt);
        assert(nearly(small.roll, 0.0));
        // Nonsense leaves the rate alone rather than zeroing it silently.
        const AngularRateDegPerS kept{.roll = 7.0};
        assert(nearly(arrest_rotation(kept, -5.0, dt).roll, 7.0));
        assert(nearly(arrest_rotation(kept, 90.0, 0.0).roll, 7.0));
    }

    std::cout << "Angular flight tests passed\n";
    return 0;
}
