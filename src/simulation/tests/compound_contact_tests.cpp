#include "everward/simulation/compound_contact.hpp"

#undef NDEBUG
#include <cassert>
#include <cmath>
#include <iostream>

using everward::simulation::EulerAttitudeDegrees;
using everward::simulation::ProbeCompoundCollisionEnvelope;
using everward::simulation::StaticSphereBody;
using everward::simulation::Vector3d;
using everward::simulation::resolve_compound_contact;
using everward::simulation::rotate_local_contact_offset;
using everward::simulation::sweep_compound_probe_against_body;

static bool nearly_equal(double a, double b, double eps = 1e-6) {
    return std::fabs(a - b) <= eps;
}

int main() {
    const ProbeCompoundCollisionEnvelope envelope{};

    // Nose-first travel should hit with the forward hull sample rather than an
    // oversized center sphere. A body centered at x=20 with radius 2 should be
    // touched when the probe root is near x=11.65: forward offset 5 + radii 3.35.
    {
        const StaticSphereBody body{"nose-target", {20.0, 0.0, 0.0}, 2.0};
        const auto hit = sweep_compound_probe_against_body(
            {0.0, 0.0, 0.0},
            {30.0, 0.0, 0.0},
            {},
            envelope,
            body);
        assert(hit.hit);
        assert(hit.sample_index == 0);
        assert(nearly_equal(hit.probe_root_at_contact.x, 11.65, 1e-5));
        assert(hit.normal.x < -0.999);

        const auto resolution = resolve_compound_contact(
            hit,
            {},
            envelope.samples[hit.sample_index],
            body,
            {30.0, 4.0, 0.0});
        assert(nearly_equal(resolution.resolved_probe_root.x, 11.649999, 2e-5));
        assert(nearly_equal(resolution.surface_point.x, 18.0, 1e-5));
        assert(resolution.normal_speed_mps > 29.9);
        assert(std::fabs(resolution.resolved_velocity.x) < 1e-6);
        assert(nearly_equal(resolution.resolved_velocity.y, 4.0));
    }

    // A lateral approach should contact the corresponding wing sample. This is
    // the critical behavior the old 8 m sphere could not represent honestly.
    {
        const StaticSphereBody body{"port-target", {0.0, -10.0, 0.0}, 1.0};
        const auto hit = sweep_compound_probe_against_body(
            {0.0, 0.0, 0.0},
            {0.0, -12.0, 0.0},
            {},
            envelope,
            body);
        assert(hit.hit);
        assert(hit.sample_index == 3);
        assert(hit.probe_root_at_contact.y < 0.0);
        assert(hit.probe_root_at_contact.y > -7.1);

        const auto resolution = resolve_compound_contact(
            hit,
            {},
            envelope.samples[hit.sample_index],
            body,
            {3.0, -12.0, 0.0});
        // The port wing sample sits at local x=-0.5 (see
        // ProbeCompoundCollisionEnvelope), so its contact normal is not purely
        // +Y: it tilts toward -X. Removing the full normal-velocity component
        // therefore bleeds a little of the incoming +X speed off too, rather
        // than leaving it untouched at 3.0 -- that would only hold for a wing
        // sample with zero local x-offset.
        assert(nearly_equal(resolution.resolved_velocity.x, -0.092237509655561, 1e-9));
        assert(nearly_equal(resolution.resolved_velocity.y, -0.023815622586108, 1e-9));
    }

    // Attitude must rotate local-space collision samples with the probe. A 90
    // degree yaw moves the forward sample from +X to +Y.
    {
        const Vector3d rotated = rotate_local_contact_offset(
            {5.0, 0.0, 0.0},
            EulerAttitudeDegrees{90.0, 0.0, 0.0});
        assert(std::fabs(rotated.x) < 1e-6);
        assert(nearly_equal(rotated.y, 5.0));
        assert(std::fabs(rotated.z) < 1e-6);

        const StaticSphereBody body{"turned-nose", {0.0, 20.0, 0.0}, 2.0};
        const auto hit = sweep_compound_probe_against_body(
            {0.0, 0.0, 0.0},
            {0.0, 30.0, 0.0},
            EulerAttitudeDegrees{90.0, 0.0, 0.0},
            envelope,
            body);
        assert(hit.hit);
        assert(hit.sample_index == 0);
        assert(nearly_equal(hit.probe_root_at_contact.y, 11.65, 1e-5));
    }

    std::cout << "Compound contact geometry tests passed\n";
    return 0;
}
