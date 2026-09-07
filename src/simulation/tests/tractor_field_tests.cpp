#include "everward/simulation/tractor_field.hpp"

#undef NDEBUG
#include <cassert>
#include <cmath>
#include <iostream>
#include <stdexcept>

using everward::simulation::TractorBodyState;
using everward::simulation::TractorFieldConfig;
using everward::simulation::TractorFieldSystem;
using everward::simulation::Vector3d;

static bool nearly_equal(double a, double b, double eps = 1e-9) {
    return std::fabs(a - b) <= eps;
}

static Vector3d momentum(const TractorBodyState& body) {
    return {
        body.mass_kg * body.velocity_mps.x,
        body.mass_kg * body.velocity_mps.y,
        body.mass_kg * body.velocity_mps.z,
    };
}

int main() {
    const TractorFieldSystem field(TractorFieldConfig{40.0, 1000.0});

    // A light debris body moves much more than the 2,500 kg Generation-1 probe.
    // Equal-and-opposite beam impulse is preserved: 1,000 N for one second
    // gives +0.4 m/s to the probe and -2.0 m/s to a 500 kg target.
    {
        TractorBodyState probe{"EV-0001", {0.0, 0.0, 0.0}, {}, 2500.0, 1.0};
        TractorBodyState debris{"debris-light", {10.0, 0.0, 0.0}, {}, 500.0, 0.5};

        const auto result = field.step(probe, debris, 1.0, 1000.0);
        assert(result.accepted);
        assert(nearly_equal(result.probe_after.velocity_mps.x, 0.4));
        assert(nearly_equal(result.target_after.velocity_mps.x, -2.0));
        assert(std::fabs(result.target_after.velocity_mps.x) >
               std::fabs(result.probe_after.velocity_mps.x));

        const Vector3d before_total = {
            momentum(probe).x + momentum(debris).x,
            momentum(probe).y + momentum(debris).y,
            momentum(probe).z + momentum(debris).z,
        };
        const Vector3d after_total = {
            momentum(result.probe_after).x + momentum(result.target_after).x,
            momentum(result.probe_after).y + momentum(result.target_after).y,
            momentum(result.probe_after).z + momentum(result.target_after).z,
        };
        assert(nearly_equal(before_total.x, after_total.x));
        assert(nearly_equal(before_total.y, after_total.y));
        assert(nearly_equal(before_total.z, after_total.z));
    }

    // A target four times the probe's mass behaves like an anchor: the same
    // field pulls the probe toward the target four times faster than it moves
    // the target toward the probe.
    {
        TractorBodyState probe{"EV-0001", {0.0, 0.0, 0.0}, {}, 2500.0, 1.0};
        TractorBodyState boulder{"boulder-heavy", {10.0, 0.0, 0.0}, {}, 10000.0, 2.0};

        const auto result = field.step(probe, boulder, 1.0, 1000.0);
        assert(result.accepted);
        assert(nearly_equal(result.probe_after.velocity_mps.x, 0.4));
        assert(nearly_equal(result.target_after.velocity_mps.x, -0.1));
        assert(std::fabs(result.probe_after.velocity_mps.x) ==
               4.0 * std::fabs(result.target_after.velocity_mps.x));
    }

    // Pre-existing probe momentum can be transferred through the coupling.
    // The beam does not create that momentum: the pair's total momentum is
    // unchanged while the heavy target gains some of the probe's motion.
    {
        TractorBodyState probe{"EV-0001", {0.0, 0.0, 0.0}, {-5.0, 0.0, 0.0}, 2500.0, 1.0};
        TractorBodyState cargo{"cargo", {10.0, 0.0, 0.0}, {}, 500.0, 0.5};
        const double momentum_before_x = momentum(probe).x + momentum(cargo).x;

        const auto result = field.step(probe, cargo, 1.0, 1000.0);
        assert(result.accepted);
        assert(nearly_equal(result.probe_after.velocity_mps.x, -4.6));
        assert(nearly_equal(result.target_after.velocity_mps.x, -2.0));
        const double momentum_after_x =
            momentum(result.probe_after).x + momentum(result.target_after).x;
        assert(nearly_equal(momentum_before_x, momentum_after_x));
    }

    // Continuous engine thrust is an external force. When the probe thrusts
    // away from the target strongly enough while the field remains engaged,
    // both probe and target can move in the towing direction. The pair's total
    // momentum changes by exactly the engine impulse, not by extra beam magic.
    {
        TractorBodyState probe{"EV-0001", {0.0, 0.0, 0.0}, {}, 2500.0, 1.0};
        TractorBodyState cargo{"cargo-heavy", {10.0, 0.0, 0.0}, {}, 10000.0, 2.0};
        const Vector3d engine_force{-2000.0, 0.0, 0.0};

        const auto result = field.step(probe, cargo, 1.0, 1000.0, engine_force);
        assert(result.accepted);
        assert(nearly_equal(result.probe_after.velocity_mps.x, -0.4));
        assert(nearly_equal(result.target_after.velocity_mps.x, -0.1));
        assert(nearly_equal(result.external_engine_impulse_ns.x, -2000.0));

        const double total_momentum_after_x =
            momentum(result.probe_after).x + momentum(result.target_after).x;
        assert(nearly_equal(total_momentum_after_x, -2000.0));
    }

    // Installed field strength is a hard hardware rating and requested force
    // above that rating clamps rather than producing impossible extra impulse.
    {
        TractorBodyState probe{"EV-0001", {0.0, 0.0, 0.0}, {}, 2500.0, 1.0};
        TractorBodyState debris{"debris", {10.0, 0.0, 0.0}, {}, 500.0, 0.5};
        const auto result = field.step(probe, debris, 1.0, 1.0e9);
        assert(result.accepted);
        assert(nearly_equal(result.applied_beam_force_n, 1000.0));
        assert(nearly_equal(result.beam_impulse_ns, 1000.0));
    }

    // Range is surface-to-surface and an out-of-range target fails without
    // changing either body's state.
    {
        TractorBodyState probe{"EV-0001", {0.0, 0.0, 0.0}, {}, 2500.0, 1.0};
        TractorBodyState far_target{"far", {100.0, 0.0, 0.0}, {}, 500.0, 0.5};
        const auto result = field.step(probe, far_target, 1.0, 1000.0);
        assert(!result.accepted);
        assert(nearly_equal(result.probe_after.velocity_mps.x, 0.0));
        assert(nearly_equal(result.target_after.velocity_mps.x, 0.0));
    }

    // Invalid physical inputs fail closed.
    {
        bool bad_mass_threw = false;
        try {
            TractorBodyState probe{"EV-0001", {}, {}, 0.0, 1.0};
            TractorBodyState target{"target", {10.0, 0.0, 0.0}, {}, 1.0, 1.0};
            (void)field.step(probe, target, 1.0, 100.0);
        } catch (const std::invalid_argument&) {
            bad_mass_threw = true;
        }
        assert(bad_mass_threw);
    }

    std::cout << "Tractor-field momentum tests passed\n";
    return 0;
}
