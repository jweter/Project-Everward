#pragma once

#include "everward/simulation/types.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>

namespace everward::simulation {

// Engine-independent foundation for Everward's tractor-field / inertial-coupling
// tool. The field is speculative technology, but once coupled its mechanical
// consequences obey ordinary momentum accounting:
//
//   * the field applies equal and opposite forces to probe and target;
//   * the lighter body changes velocity more for the same beam force;
//   * a target more massive than the probe therefore tends to pull the probe
//     toward itself rather than behaving like an immovable arcade pickup;
//   * probe engine thrust is an external force and can add net momentum to the
//     coupled pair, enabling powered towing of masses that would otherwise drag
//     the probe around.
//
// The familiar m*v quantity is momentum, not force. Pre-accelerating the probe
// can still be useful: when the beam couples, some of that existing momentum can
// transfer to the target while the probe slows. Continuing to thrust while
// coupled adds fresh external momentum. This is the physically meaningful basis
// for the intended "get moving, couple, and tow something heavy" gameplay.
struct TractorBodyState {
    std::string body_id;
    Vector3d position_m{};
    Vector3d velocity_mps{};
    double mass_kg{1.0};
    double radius_m{0.5};
};

struct TractorFieldConfig {
    // Range is measured surface-to-surface so large targets do not receive an
    // arbitrary range advantage merely because their center is far away.
    double max_surface_range_m{40.0};
    double max_force_n{5000.0};
};

struct TractorFieldStepResult {
    bool accepted{false};
    TractorBodyState probe_after{};
    TractorBodyState target_after{};
    double center_distance_m{0.0};
    double surface_gap_m{0.0};
    double applied_beam_force_n{0.0};
    double beam_impulse_ns{0.0};
    Vector3d external_engine_impulse_ns{};
    std::string detail;
};

class TractorFieldSystem {
public:
    explicit TractorFieldSystem(TractorFieldConfig config = {}) : config_(config) {
        validate_config(config_);
    }

    [[nodiscard]] const TractorFieldConfig& config() const noexcept {
        return config_;
    }

    // Advances one deterministic coupling step. requested_force_n is attraction
    // only: negative values are invalid, and values above the installed field
    // rating are clamped to max_force_n. engine_force_world_n is the probe's
    // externally supplied thrust force during this step; it is intentionally
    // separate from the internal beam force so momentum conservation remains
    // legible and testable.
    [[nodiscard]] TractorFieldStepResult step(
        TractorBodyState probe,
        TractorBodyState target,
        double seconds,
        double requested_force_n,
        Vector3d engine_force_world_n = {}) const {
        validate_body(probe, "probe");
        validate_body(target, "target");
        require_finite(seconds, "seconds");
        require_finite(requested_force_n, "requested_force_n");
        require_finite_vector(engine_force_world_n, "engine_force_world_n");
        if (seconds < 0.0) {
            throw std::invalid_argument("seconds must be non-negative");
        }
        if (requested_force_n < 0.0) {
            throw std::invalid_argument("requested tractor force must be non-negative");
        }
        if (probe.body_id == target.body_id) {
            throw std::invalid_argument("tractor target must not be the probe itself");
        }

        TractorFieldStepResult result;
        result.probe_after = probe;
        result.target_after = target;

        const Vector3d separation = subtract(target.position_m, probe.position_m);
        const double center_distance = magnitude(separation);
        result.center_distance_m = center_distance;
        result.surface_gap_m = center_distance - (probe.radius_m + target.radius_m);

        if (center_distance <= 1.0e-9) {
            result.detail = "tractor field cannot establish a direction between coincident body centers";
            return result;
        }
        if (result.surface_gap_m > config_.max_surface_range_m) {
            result.detail = "tractor target is outside field range";
            return result;
        }

        const double beam_force_n = std::min(requested_force_n, config_.max_force_n);
        const Vector3d toward_target = scale(separation, 1.0 / center_distance);

        // Pulling the target toward the probe means the target receives
        // -toward_target force; Newton's third-law reaction pulls the probe
        // +toward_target. The pair's beam impulses cancel exactly.
        const Vector3d beam_force_on_probe = scale(toward_target, beam_force_n);
        const Vector3d beam_force_on_target = scale(toward_target, -beam_force_n);
        const Vector3d net_probe_force = add(beam_force_on_probe, engine_force_world_n);

        result.probe_after.velocity_mps = add(
            probe.velocity_mps,
            scale(net_probe_force, seconds / probe.mass_kg));
        result.target_after.velocity_mps = add(
            target.velocity_mps,
            scale(beam_force_on_target, seconds / target.mass_kg));

        // Semi-implicit Euler keeps the deterministic foundation simple and
        // makes the freshly changed velocities visible in position immediately.
        // Collision/capture resolution remains the responsibility of the normal
        // contact/manipulator systems when this foundation is composed into the
        // playable runtime.
        result.probe_after.position_m = add(
            probe.position_m,
            scale(result.probe_after.velocity_mps, seconds));
        result.target_after.position_m = add(
            target.position_m,
            scale(result.target_after.velocity_mps, seconds));

        result.accepted = true;
        result.applied_beam_force_n = beam_force_n;
        result.beam_impulse_ns = beam_force_n * seconds;
        result.external_engine_impulse_ns = scale(engine_force_world_n, seconds);

        if (target.mass_kg < probe.mass_kg) {
            result.detail = "tractor coupled: target is lighter, so it accelerates toward the probe more strongly";
        } else if (target.mass_kg > probe.mass_kg) {
            result.detail = "tractor coupled: target is heavier, so reaction accelerates the probe toward it more strongly";
        } else {
            result.detail = "tractor coupled: equal masses receive equal-and-opposite acceleration magnitudes";
        }
        return result;
    }

private:
    static void validate_config(const TractorFieldConfig& config) {
        require_finite(config.max_surface_range_m, "max_surface_range_m");
        require_finite(config.max_force_n, "max_force_n");
        if (config.max_surface_range_m < 0.0) {
            throw std::invalid_argument("tractor max surface range must be non-negative");
        }
        if (config.max_force_n < 0.0) {
            throw std::invalid_argument("tractor max force must be non-negative");
        }
    }

    static void validate_body(const TractorBodyState& body, const char* label) {
        if (body.body_id.empty()) {
            throw std::invalid_argument(std::string(label) + " body id must not be empty");
        }
        require_finite_vector(body.position_m, "body position");
        require_finite_vector(body.velocity_mps, "body velocity");
        require_finite(body.mass_kg, "body mass");
        require_finite(body.radius_m, "body radius");
        if (body.mass_kg <= 0.0) {
            throw std::invalid_argument(std::string(label) + " mass must be positive");
        }
        if (body.radius_m < 0.0) {
            throw std::invalid_argument(std::string(label) + " radius must be non-negative");
        }
    }

    static void require_finite(double value, const char* label) {
        if (!std::isfinite(value)) {
            throw std::invalid_argument(std::string(label) + " must be finite");
        }
    }

    static void require_finite_vector(Vector3d value, const char* label) {
        if (!std::isfinite(value.x) || !std::isfinite(value.y) || !std::isfinite(value.z)) {
            throw std::invalid_argument(std::string(label) + " must be finite");
        }
    }

    [[nodiscard]] static Vector3d add(Vector3d a, Vector3d b) noexcept {
        return {a.x + b.x, a.y + b.y, a.z + b.z};
    }

    [[nodiscard]] static Vector3d subtract(Vector3d a, Vector3d b) noexcept {
        return {a.x - b.x, a.y - b.y, a.z - b.z};
    }

    [[nodiscard]] static Vector3d scale(Vector3d value, double factor) noexcept {
        return {value.x * factor, value.y * factor, value.z * factor};
    }

    [[nodiscard]] static double magnitude(Vector3d value) noexcept {
        return std::sqrt(value.x * value.x + value.y * value.y + value.z * value.z);
    }

    TractorFieldConfig config_{};
};

} // namespace everward::simulation
