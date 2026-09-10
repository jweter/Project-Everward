#pragma once

#include "everward/simulation/types.hpp"

#include <algorithm>
#include <cmath>
#include <string>

namespace everward::simulation {

struct SphericalPlanetaryBody {
    std::string body_id;
    Vector3d center_m{};
    double radius_m{1.0};
    Vector3d velocity_mps{};
    double gravitational_parameter_m3_s2{0.0};
};

struct PlanetaryLocalFrame { Vector3d up{}; Vector3d east{}; Vector3d north{}; };
struct OrbitalContext { double radius_from_center_m{0.0}; double radial_speed_mps{0.0}; double tangential_speed_mps{0.0}; double circular_orbit_speed_mps{0.0}; bool gravity_enabled{false}; };
struct SurfaceRelativeMotion {
    Vector3d body_relative_velocity_mps{};
    Vector3d vertical_velocity_mps{};
    Vector3d tangential_velocity_mps{};
    double vertical_speed_mps{0.0};
    double tangential_speed_mps{0.0};
    Vector3d surface_normal{};
    double clearance_m{0.0};
    double radial_speed_mps{0.0};
};
struct ControlledDescentEnvelope { double max_descent_speed_mps{5.0}; double max_tangential_speed_mps{2.0}; double minimum_clearance_m{0.0}; };
struct SurfaceContactResolution { Vector3d position_m{}; Vector3d velocity_mps{}; bool corrected{false}; double penetration_depth_m{0.0}; };

enum class SurfaceApproachState { Clear, ControlledDescent, ExcessiveDescentRate, ExcessiveTangentialRate, SurfacePenetration };

[[nodiscard]] inline Vector3d planetary_subtract(Vector3d a, Vector3d b) noexcept { return {a.x-b.x,a.y-b.y,a.z-b.z}; }
[[nodiscard]] inline Vector3d planetary_scale(Vector3d v,double s) noexcept { return {v.x*s,v.y*s,v.z*s}; }
[[nodiscard]] inline double planetary_dot(Vector3d a,Vector3d b) noexcept { return a.x*b.x+a.y*b.y+a.z*b.z; }
[[nodiscard]] inline Vector3d planetary_cross(Vector3d a,Vector3d b) noexcept { return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x}; }
[[nodiscard]] inline double planetary_magnitude(Vector3d v) noexcept { return std::sqrt(planetary_dot(v,v)); }
[[nodiscard]] inline Vector3d planetary_normalized_or_x(Vector3d v) noexcept { const double m=planetary_magnitude(v); return m<=1e-12?Vector3d{1,0,0}:planetary_scale(v,1.0/m); }
[[nodiscard]] inline double altitude_above_reference_surface(Vector3d p,const SphericalPlanetaryBody& b) noexcept { return planetary_magnitude(planetary_subtract(p,b.center_m))-b.radius_m; }
[[nodiscard]] inline Vector3d local_surface_normal(Vector3d p,const SphericalPlanetaryBody& b) noexcept { return planetary_normalized_or_x(planetary_subtract(p,b.center_m)); }
[[nodiscard]] inline PlanetaryLocalFrame local_horizon_frame(Vector3d p,const SphericalPlanetaryBody& b) noexcept { const Vector3d up=local_surface_normal(p,b); const Vector3d z{0,0,1}; const Vector3d nc=planetary_subtract(z,planetary_scale(up,planetary_dot(z,up))); const double nm=planetary_magnitude(nc); const Vector3d north=nm<=1e-12?Vector3d{0,up.z>=0?1.0:-1.0,0}:planetary_scale(nc,1.0/nm); return {up,planetary_normalized_or_x(planetary_cross(north,up)),north}; }
[[nodiscard]] inline Vector3d body_relative_velocity(Vector3d v,const SphericalPlanetaryBody& b) noexcept { return planetary_subtract(v,b.velocity_mps); }
[[nodiscard]] inline Vector3d gravitational_acceleration(Vector3d p,const SphericalPlanetaryBody& b) noexcept { const double mu=b.gravitational_parameter_m3_s2; if(!std::isfinite(mu)||mu<=0) return {}; const Vector3d d=planetary_subtract(b.center_m,p); const double r2=planetary_dot(d,d); if(r2<=1e-12) return {}; const double r=std::sqrt(r2); return planetary_scale(d,mu/(r2*r)); }
[[nodiscard]] inline SurfaceRelativeMotion surface_relative_motion(Vector3d p,Vector3d v,const SphericalPlanetaryBody& b) noexcept {
    const Vector3d rel=body_relative_velocity(v,b);
    const Vector3d up=local_surface_normal(p,b);
    const double vs=planetary_dot(rel,up);
    const Vector3d vertical=planetary_scale(up,vs);
    const Vector3d tangent=planetary_subtract(rel,vertical);
    return {rel,vertical,tangent,vs,planetary_magnitude(tangent),up,altitude_above_reference_surface(p,b),vs};
}
[[nodiscard]] inline OrbitalContext orbital_context(Vector3d p,Vector3d v,const SphericalPlanetaryBody& b) noexcept { const double r=planetary_magnitude(planetary_subtract(p,b.center_m)); const auto m=surface_relative_motion(p,v,b); const double mu=b.gravitational_parameter_m3_s2; const bool g=std::isfinite(mu)&&mu>0&&r>1e-12; return {r,m.vertical_speed_mps,m.tangential_speed_mps,g?std::sqrt(mu/r):0.0,g}; }

[[nodiscard]] inline SurfaceContactResolution resolve_surface_contact(Vector3d p,Vector3d v,const SphericalPlanetaryBody& b,double clearance=0.0) noexcept {
    clearance=std::max(0.0,clearance); const double altitude=altitude_above_reference_surface(p,b); if(altitude>=clearance) return {p,v,false,0};
    const Vector3d n=local_surface_normal(p,b); const double radius=b.radius_m+clearance; p={b.center_m.x+n.x*radius,b.center_m.y+n.y*radius,b.center_m.z+n.z*radius};
    Vector3d rel=body_relative_velocity(v,b); const double ns=planetary_dot(rel,n); if(ns<0) rel=planetary_subtract(rel,planetary_scale(n,ns));
    v={b.velocity_mps.x+rel.x,b.velocity_mps.y+rel.y,b.velocity_mps.z+rel.z}; return {p,v,true,clearance-altitude};
}

// Continuous contact resolution for a fixed simulation step. The segment is
// tested against the permitted clearance sphere so a step cannot tunnel from
// one clear side of a body to the other. Resolution occurs at first contact.
[[nodiscard]] inline SurfaceContactResolution resolve_swept_surface_contact(
    Vector3d previous_position_m, Vector3d proposed_position_m, Vector3d velocity_mps,
    const SphericalPlanetaryBody& body, double minimum_clearance_m = 0.0) noexcept {
    const double clearance = std::max(0.0, minimum_clearance_m);
    const double radius = body.radius_m + clearance;
    const Vector3d start = planetary_subtract(previous_position_m, body.center_m);
    const Vector3d delta = planetary_subtract(proposed_position_m, previous_position_m);
    const double a = planetary_dot(delta, delta);
    const double c = planetary_dot(start, start) - radius * radius;
    if (c <= 0.0) return resolve_surface_contact(proposed_position_m, velocity_mps, body, clearance);
    if (a <= 1e-12) return {proposed_position_m, velocity_mps, false, 0.0};
    const double half_b = planetary_dot(start, delta);
    const double discriminant = half_b * half_b - a * c;
    if (discriminant < 0.0) return resolve_surface_contact(proposed_position_m, velocity_mps, body, clearance);
    const double root = std::sqrt(discriminant);
    const double t = (-half_b - root) / a;
    if (t < 0.0 || t > 1.0) return resolve_surface_contact(proposed_position_m, velocity_mps, body, clearance);
    const Vector3d contact{previous_position_m.x + delta.x*t, previous_position_m.y + delta.y*t, previous_position_m.z + delta.z*t};
    const Vector3d normal = local_surface_normal(contact, body);
    Vector3d relative = body_relative_velocity(velocity_mps, body);
    const double normal_speed = planetary_dot(relative, normal);
    if (normal_speed < 0.0) relative=planetary_subtract(relative,planetary_scale(normal,normal_speed));
    velocity_mps={body.velocity_mps.x+relative.x,body.velocity_mps.y+relative.y,body.velocity_mps.z+relative.z};
    return {contact,velocity_mps,true,planetary_magnitude(delta)*(1.0-t)};
}

[[nodiscard]] inline SurfaceApproachState classify_surface_approach(Vector3d p,Vector3d v,const SphericalPlanetaryBody& b,const ControlledDescentEnvelope& e={}) noexcept { const double a=altitude_above_reference_surface(p,b); if(a<e.minimum_clearance_m) return SurfaceApproachState::SurfacePenetration; const auto m=surface_relative_motion(p,v,b); if(m.vertical_speed_mps<-std::fabs(e.max_descent_speed_mps)) return SurfaceApproachState::ExcessiveDescentRate; if(m.tangential_speed_mps>std::fabs(e.max_tangential_speed_mps)) return SurfaceApproachState::ExcessiveTangentialRate; if(m.vertical_speed_mps<0) return SurfaceApproachState::ControlledDescent; return SurfaceApproachState::Clear; }
[[nodiscard]] inline bool is_below_reference_surface(Vector3d p,const SphericalPlanetaryBody& b) noexcept { return altitude_above_reference_surface(p,b)<0; }

} // namespace everward::simulation
