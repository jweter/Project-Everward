#pragma once

#include "everward/simulation/types.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>

namespace everward::simulation {

struct CompoundContactCandidate {
    bool hit{false};
    std::size_t sample_index{0};
    double fraction{1.0};
    Vector3d sample_center_at_contact{};
    Vector3d probe_root_at_contact{};
    Vector3d normal{};
};

[[nodiscard]] inline Vector3d contact_add(Vector3d a, Vector3d b) noexcept {
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}

[[nodiscard]] inline Vector3d contact_subtract(Vector3d a, Vector3d b) noexcept {
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}

[[nodiscard]] inline Vector3d contact_scale(Vector3d value, double scalar) noexcept {
    return {value.x * scalar, value.y * scalar, value.z * scalar};
}

[[nodiscard]] inline double contact_dot(Vector3d a, Vector3d b) noexcept {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

[[nodiscard]] inline Vector3d contact_normalized_or_x(Vector3d value) noexcept {
    const double magnitude = std::sqrt(contact_dot(value, value));
    if (magnitude <= 1e-12) {
        return {1.0, 0.0, 0.0};
    }
    return contact_scale(value, 1.0 / magnitude);
}

[[nodiscard]] inline Vector3d rotate_local_contact_offset(
    Vector3d local,
    EulerAttitudeDegrees attitude) noexcept {
    constexpr double Pi = 3.14159265358979323846;
    const double yaw = attitude.yaw * Pi / 180.0;
    const double pitch = attitude.pitch * Pi / 180.0;
    const double roll = attitude.roll * Pi / 180.0;

    const double cy = std::cos(yaw);
    const double sy = std::sin(yaw);
    const double cp = std::cos(pitch);
    const double sp = std::sin(pitch);
    const double cr = std::cos(roll);
    const double sr = std::sin(roll);

    // Intrinsic roll(X), pitch(Y), yaw(Z), matching the Generation-1
    // yaw/pitch/roll convention used by the probe attitude read model.
    const Vector3d rolled{
        local.x,
        local.y * cr - local.z * sr,
        local.y * sr + local.z * cr,
    };
    const Vector3d pitched{
        rolled.x * cp + rolled.z * sp,
        rolled.y,
        -rolled.x * sp + rolled.z * cp,
    };
    return {
        pitched.x * cy - pitched.y * sy,
        pitched.x * sy + pitched.y * cy,
        pitched.z,
    };
}

[[nodiscard]] inline bool sweep_compound_probe_sample_against_body(
    Vector3d probe_start,
    Vector3d probe_end,
    EulerAttitudeDegrees attitude,
    const ProbeCollisionSphereSample& sample,
    std::size_t sample_index,
    const StaticSphereBody& body,
    CompoundContactCandidate& candidate) noexcept {
    const Vector3d offset = rotate_local_contact_offset(sample.local_center_m, attitude);
    const Vector3d sample_start = contact_add(probe_start, offset);
    const Vector3d sample_end = contact_add(probe_end, offset);
    const Vector3d delta = contact_subtract(sample_end, sample_start);
    const Vector3d from_center = contact_subtract(sample_start, body.center_m);
    const double combined_radius = body.radius_m + sample.radius_m;
    const double a = contact_dot(delta, delta);
    const double c = contact_dot(from_center, from_center) - combined_radius * combined_radius;

    double fraction = std::numeric_limits<double>::infinity();
    if (c <= 0.0) {
        fraction = 0.0;
    } else if (a > 1e-18) {
        const double b = 2.0 * contact_dot(from_center, delta);
        const double discriminant = b * b - 4.0 * a * c;
        if (discriminant >= 0.0) {
            const double root = (-b - std::sqrt(discriminant)) / (2.0 * a);
            if (root >= 0.0 && root <= 1.0) {
                fraction = root;
            }
        }
    }

    if (!std::isfinite(fraction)) {
        return false;
    }

    const Vector3d sample_center = contact_add(sample_start, contact_scale(delta, fraction));
    candidate.hit = true;
    candidate.sample_index = sample_index;
    candidate.fraction = fraction;
    candidate.sample_center_at_contact = sample_center;
    candidate.probe_root_at_contact = contact_subtract(sample_center, offset);
    candidate.normal = contact_normalized_or_x(contact_subtract(sample_center, body.center_m));
    return true;
}

[[nodiscard]] inline CompoundContactCandidate sweep_compound_probe_against_body(
    Vector3d probe_start,
    Vector3d probe_end,
    EulerAttitudeDegrees attitude,
    const ProbeCompoundCollisionEnvelope& envelope,
    const StaticSphereBody& body) noexcept {
    CompoundContactCandidate earliest;
    earliest.fraction = std::numeric_limits<double>::infinity();

    for (std::size_t index = 0; index < envelope.samples.size(); ++index) {
        CompoundContactCandidate candidate;
        if (sweep_compound_probe_sample_against_body(
                probe_start,
                probe_end,
                attitude,
                envelope.samples[index],
                index,
                body,
                candidate) &&
            candidate.fraction < earliest.fraction) {
            earliest = candidate;
        }
    }

    return earliest;
}

} // namespace everward::simulation
