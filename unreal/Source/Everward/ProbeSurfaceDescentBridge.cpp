#include "ProbeSimulationAdapter.h"

#include "everward/simulation/impact_damage.hpp"
#include "everward/simulation/surface_descent_guidance.hpp"

namespace
{
everward::simulation::ControlledDescentEnvelope MakeEnvelope(
    double MaxDescentSpeedMetersPerSecond,
    double MaxTangentialSpeedMetersPerSecond,
    double MinimumClearanceMeters)
{
    everward::simulation::ControlledDescentEnvelope Envelope;
    Envelope.max_descent_speed_mps = MaxDescentSpeedMetersPerSecond;
    Envelope.max_tangential_speed_mps = MaxTangentialSpeedMetersPerSecond;
    Envelope.minimum_clearance_m = MinimumClearanceMeters;
    return Envelope;
}

everward::simulation::AltitudeAwareDescentProfile MakeProfile(
    double FullSpeedAltitudeMeters,
    double TouchdownDescentSpeedMetersPerSecond)
{
    everward::simulation::AltitudeAwareDescentProfile Profile;
    Profile.full_speed_altitude_m = FullSpeedAltitudeMeters;
    Profile.touchdown_descent_speed_mps = TouchdownDescentSpeedMetersPerSecond;
    return Profile;
}

everward::simulation::SurfaceHoverProfile MakeHoverProfile(
    double TargetAltitudeMeters,
    double AltitudeGainPerSecond,
    double MaxVerticalCorrectionSpeedMetersPerSecond)
{
    everward::simulation::SurfaceHoverProfile Profile;
    Profile.target_altitude_m = TargetAltitudeMeters;
    Profile.altitude_gain_per_second = AltitudeGainPerSecond;
    Profile.max_vertical_correction_speed_mps = MaxVerticalCorrectionSpeedMetersPerSecond;
    return Profile;
}
} // namespace

// docs/PHASE2_VERTICAL_SLICE_PLAN.md's Slice 10 status named this exact gap:
// surface_descent_guidance.hpp's command-shaping math landed as a standalone,
// ctest-covered module but no authoritative command, ProbeRuntime/adapter
// method, or player input called it. This follows the same read-only-query
// pattern GetJoseGuidanceCommand() already established for Slice 7's
// destination navigation -- it only reads Core's live pose and registered
// planetary body (Slice 9's set_planetary_body/clear_planetary_body) plus
// the caller's tunable envelope/profile parameters, then maps the pure
// controlled_descent_velocity_command() result onto the Blueprint-visible
// struct. It does not move the probe or accept/reject anything itself.
FEverwardControlledDescentCommand UProbeSimulationAdapter::GetControlledDescentVelocityCommand(
    FVector RequestedVelocityMetersPerSecond,
    double MaxDescentSpeedMetersPerSecond,
    double MaxTangentialSpeedMetersPerSecond,
    double MinimumClearanceMeters,
    double FullSpeedAltitudeMeters,
    double TouchdownDescentSpeedMetersPerSecond) const
{
    FEverwardControlledDescentCommand Result;
    if (Core == nullptr)
    {
        return Result;
    }

    const auto Envelope = MakeEnvelope(
        MaxDescentSpeedMetersPerSecond, MaxTangentialSpeedMetersPerSecond, MinimumClearanceMeters);
    const auto Profile = MakeProfile(FullSpeedAltitudeMeters, TouchdownDescentSpeedMetersPerSecond);

    const auto Command = everward::simulation::controlled_descent_velocity_command(
        Core->snapshot().position_m,
        {RequestedVelocityMetersPerSecond.X, RequestedVelocityMetersPerSecond.Y, RequestedVelocityMetersPerSecond.Z},
        Core->planetary_body(),
        Envelope,
        Profile);
    if (!Command.has_value())
    {
        return Result;
    }

    Result.bHasResult = true;
    Result.CommandVelocityMetersPerSecond = FVector(
        Command->velocity_mps.x, Command->velocity_mps.y, Command->velocity_mps.z);
    Result.bDescentRateLimited = Command->descent_rate_limited;
    Result.bTangentialRateLimited = Command->tangential_rate_limited;
    return Result;
}

// Computes the same constrained command as GetControlledDescentVelocityCommand()
// and, unlike that read-only query, actually issues it through the exact
// same Core->set_velocity_mps() boundary CommandSetVelocityMetersPerSecond()
// already uses -- no second velocity-mutation path. Fails closed (rejected,
// no mutation) whenever no planetary body is registered, the same
// "destination unavailable" contract GetJoseGuidanceCommand() already
// applies to an unregistered destination, rather than silently falling back
// to an unconstrained raw velocity command near empty space.
FEverwardProbeCommandResult UProbeSimulationAdapter::CommandSetControlledDescentVelocityMetersPerSecond(
    FVector RequestedVelocityMetersPerSecond,
    double MaxDescentSpeedMetersPerSecond,
    double MaxTangentialSpeedMetersPerSecond,
    double MinimumClearanceMeters,
    double FullSpeedAltitudeMeters,
    double TouchdownDescentSpeedMetersPerSecond)
{
    const FName CommandId(TEXT("set_controlled_descent_velocity"));
    if (Core == nullptr)
    {
        return RecordCommandResult(CommandId, false, TEXT("simulation unavailable"));
    }
    if (!Core->planetary_body().has_value())
    {
        return RecordCommandResult(CommandId, false, TEXT("no planetary body registered"));
    }

    const auto Envelope = MakeEnvelope(
        MaxDescentSpeedMetersPerSecond, MaxTangentialSpeedMetersPerSecond, MinimumClearanceMeters);
    const auto Profile = MakeProfile(FullSpeedAltitudeMeters, TouchdownDescentSpeedMetersPerSecond);

    const auto Command = everward::simulation::controlled_descent_velocity_command(
        Core->snapshot().position_m,
        {RequestedVelocityMetersPerSecond.X, RequestedVelocityMetersPerSecond.Y, RequestedVelocityMetersPerSecond.Z},
        Core->planetary_body(),
        Envelope,
        Profile);
    if (!Command.has_value())
    {
        return RecordCommandResult(CommandId, false, TEXT("no planetary body registered"));
    }

    try
    {
        Core->set_velocity_mps(Command->velocity_mps);
        return RecordCommandResult(CommandId, true,
            FString::Printf(TEXT("controlled descent velocity accepted: [%.2f, %.2f, %.2f] m/s%s%s"),
                Command->velocity_mps.x, Command->velocity_mps.y, Command->velocity_mps.z,
                Command->descent_rate_limited ? TEXT(" (descent rate limited)") : TEXT(""),
                Command->tangential_rate_limited ? TEXT(" (tangential rate limited)") : TEXT("")));
    }
    catch (const std::exception& Error)
    {
        return RecordCommandResult(CommandId, false, UTF8_TO_TCHAR(Error.what()));
    }
}

// Wires surface_descent_guidance.hpp's controlled_hover_velocity_command()
// the same way GetControlledDescentVelocityCommand() wires its descent
// counterpart: a read-only query over Core's live pose and registered
// planetary body plus the caller's tunable envelope/hover parameters.
// bHasResult is false with no registered planetary body, matching the
// existing descent query's fail-closed contract rather than fabricating a
// hover target for deep space.
FEverwardControlledDescentCommand UProbeSimulationAdapter::GetControlledHoverVelocityCommand(
    FVector RequestedVelocityMetersPerSecond,
    double MaxTangentialSpeedMetersPerSecond,
    double MinimumClearanceMeters,
    double TargetAltitudeMeters,
    double AltitudeGainPerSecond,
    double MaxVerticalCorrectionSpeedMetersPerSecond) const
{
    FEverwardControlledDescentCommand Result;
    if (Core == nullptr)
    {
        return Result;
    }

    const auto Envelope = MakeEnvelope(0.0, MaxTangentialSpeedMetersPerSecond, MinimumClearanceMeters);
    const auto Hover = MakeHoverProfile(
        TargetAltitudeMeters, AltitudeGainPerSecond, MaxVerticalCorrectionSpeedMetersPerSecond);

    const auto Command = everward::simulation::controlled_hover_velocity_command(
        Core->snapshot().position_m,
        {RequestedVelocityMetersPerSecond.X, RequestedVelocityMetersPerSecond.Y, RequestedVelocityMetersPerSecond.Z},
        Core->planetary_body(),
        Envelope,
        Hover);
    if (!Command.has_value())
    {
        return Result;
    }

    Result.bHasResult = true;
    Result.CommandVelocityMetersPerSecond = FVector(
        Command->velocity_mps.x, Command->velocity_mps.y, Command->velocity_mps.z);
    Result.bDescentRateLimited = Command->descent_rate_limited;
    Result.bTangentialRateLimited = Command->tangential_rate_limited;
    return Result;
}

// Computes the same constrained hover command as
// GetControlledHoverVelocityCommand() and, unlike that read-only query,
// actually issues it through the exact same Core->set_velocity_mps()
// boundary CommandSetControlledDescentVelocityMetersPerSecond() already
// uses -- no second velocity-mutation path. Fails closed (rejected, no
// mutation) whenever no planetary body is registered.
FEverwardProbeCommandResult UProbeSimulationAdapter::CommandSetControlledHoverVelocityMetersPerSecond(
    FVector RequestedVelocityMetersPerSecond,
    double MaxTangentialSpeedMetersPerSecond,
    double MinimumClearanceMeters,
    double TargetAltitudeMeters,
    double AltitudeGainPerSecond,
    double MaxVerticalCorrectionSpeedMetersPerSecond)
{
    const FName CommandId(TEXT("set_controlled_hover_velocity"));
    if (Core == nullptr)
    {
        return RecordCommandResult(CommandId, false, TEXT("simulation unavailable"));
    }
    if (!Core->planetary_body().has_value())
    {
        return RecordCommandResult(CommandId, false, TEXT("no planetary body registered"));
    }

    const auto Envelope = MakeEnvelope(0.0, MaxTangentialSpeedMetersPerSecond, MinimumClearanceMeters);
    const auto Hover = MakeHoverProfile(
        TargetAltitudeMeters, AltitudeGainPerSecond, MaxVerticalCorrectionSpeedMetersPerSecond);

    const auto Command = everward::simulation::controlled_hover_velocity_command(
        Core->snapshot().position_m,
        {RequestedVelocityMetersPerSecond.X, RequestedVelocityMetersPerSecond.Y, RequestedVelocityMetersPerSecond.Z},
        Core->planetary_body(),
        Envelope,
        Hover);
    if (!Command.has_value())
    {
        return RecordCommandResult(CommandId, false, TEXT("no planetary body registered"));
    }

    try
    {
        Core->set_velocity_mps(Command->velocity_mps);
        return RecordCommandResult(CommandId, true,
            FString::Printf(TEXT("controlled hover velocity accepted: [%.2f, %.2f, %.2f] m/s%s%s"),
                Command->velocity_mps.x, Command->velocity_mps.y, Command->velocity_mps.z,
                Command->descent_rate_limited ? TEXT(" (altitude correction limited)") : TEXT(""),
                Command->tangential_rate_limited ? TEXT(" (tangential rate limited)") : TEXT("")));
    }
    catch (const std::exception& Error)
    {
        return RecordCommandResult(CommandId, false, UTF8_TO_TCHAR(Error.what()));
    }
}
