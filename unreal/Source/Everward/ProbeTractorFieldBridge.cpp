#include "ProbeSimulationAdapter.h"

#include "EverwardPhase2TestEnvironment.h"
#include "everward/simulation/impact_damage.hpp"
#include "everward/simulation/manipulator.hpp"
#include "everward/simulation/tractor_field.hpp"

#include <algorithm>
#include <cmath>
#include <string>

namespace
{
const everward::simulation::StaticSphereBody* FindRegisteredBody(
    const everward::simulation::DamageAwareProbeRuntime& Core,
    const FString& BodyId)
{
    const std::string SimulationId(TCHAR_TO_UTF8(*BodyId));
    for (const everward::simulation::StaticSphereBody& Body : Core.static_bodies())
    {
        if (Body.body_id == SimulationId)
        {
            return &Body;
        }
    }
    return nullptr;
}

double ResolvePhase2BodyMassKilograms(const FString& BodyId, double RadiusMeters)
{
    if (BodyId == AEverwardPhase2TestEnvironment::BootstrapScanTargetId)
    {
        return AEverwardPhase2TestEnvironment::BootstrapBodyMassKilograms;
    }
    if (BodyId == AEverwardPhase2TestEnvironment::ReferenceTarget1Id)
    {
        return AEverwardPhase2TestEnvironment::ReferenceTarget1MassKilograms;
    }
    if (BodyId == AEverwardPhase2TestEnvironment::ReferenceTarget2Id)
    {
        return AEverwardPhase2TestEnvironment::ReferenceTarget2MassKilograms;
    }

    // The current StaticSphereBody contract predates movable-body mass. Known
    // Phase-2 bodies therefore use explicit masses above. This conservative
    // fallback only keeps future test spheres usable until mass becomes part
    // of the registered-body schema itself; it is not a material-density
    // simulation claim.
    constexpr double PrototypeFallbackDensityKgPerCubicMeter = 500.0;
    constexpr double FourThirdsPi = 4.1887902047863909846;
    return FMath::Max(
        1.0,
        FourThirdsPi * RadiusMeters * RadiusMeters * RadiusMeters *
            PrototypeFallbackDensityKgPerCubicMeter);
}

FString ExpectedMotionText(double ProbeMassKilograms, double TargetMassKilograms)
{
    if (TargetMassKilograms < ProbeMassKilograms)
    {
        return TEXT("TARGET MOVES MORE");
    }
    if (TargetMassKilograms > ProbeMassKilograms)
    {
        return TEXT("PROBE MOVES MORE // TARGET ACTS AS ANCHOR");
    }
    return TEXT("BOTH MOVE EQUALLY");
}

bool IsBodyHeldByManipulator(
    const everward::simulation::ManipulatorRig* Manipulators,
    const FString& BodyId)
{
    if (Manipulators == nullptr)
    {
        return false;
    }

    const std::string SimulationId(TCHAR_TO_UTF8(*BodyId));
    for (const everward::simulation::ManipulatorArmId ArmId :
        {everward::simulation::ManipulatorArmId::Port,
         everward::simulation::ManipulatorArmId::Starboard})
    {
        if (Manipulators->arm(ArmId).grasped_target_body_id == SimulationId)
        {
            return true;
        }
    }
    return false;
}

FVector ToUnrealVector(const everward::simulation::Vector3d& Value)
{
    return FVector(Value.x, Value.y, Value.z);
}
} // namespace

FEverwardTractorFieldStatus UProbeSimulationAdapter::GetTractorFieldStatus() const
{
    FEverwardTractorFieldStatus Status;
    Status.bEngaged = bTractorFieldEngaged;
    Status.MaxSurfaceRangeMeters = TractorMaxSurfaceRangeMeters;
    Status.RequestedForceNewtons = TractorRequestedForceNewtons;
    Status.AppliedForceNewtons = TractorAppliedForceNewtons;
    Status.Detail = TractorFieldDetail;

    if (Core == nullptr)
    {
        Status.Detail = TEXT("simulation unavailable");
        return Status;
    }

    const auto& Probe = Core->snapshot();
    Status.ProbeMassKilograms = Probe.mass_kg;

    FString TargetId = TractorTargetId;
    if (TargetId.IsEmpty())
    {
        const everward::simulation::TargetSelectionStatus Selection = Core->selected_target_status();
        if (Selection.has_selection)
        {
            TargetId = UTF8_TO_TCHAR(Selection.body_id.c_str());
        }
    }

    if (TargetId.IsEmpty())
    {
        if (Status.Detail.IsEmpty())
        {
            Status.Detail = TEXT("select a physical target with T, then hold B to couple");
        }
        return Status;
    }

    const everward::simulation::StaticSphereBody* Body = FindRegisteredBody(*Core, TargetId);
    if (Body == nullptr)
    {
        Status.Detail = TEXT("tractor target is no longer registered");
        return Status;
    }

    Status.bHasTarget = true;
    Status.TargetId = TargetId;
    Status.TargetMassKilograms = ResolvePhase2BodyMassKilograms(TargetId, Body->radius_m);
    Status.TargetToProbeMassRatio = Probe.mass_kg > 0.0
        ? Status.TargetMassKilograms / Probe.mass_kg
        : 0.0;

    const everward::simulation::Vector3d Delta{
        Body->center_m.x - Probe.position_m.x,
        Body->center_m.y - Probe.position_m.y,
        Body->center_m.z - Probe.position_m.z,
    };
    const double CenterDistanceMeters = std::sqrt(
        Delta.x * Delta.x + Delta.y * Delta.y + Delta.z * Delta.z);
    Status.SurfaceRangeMeters = FMath::Max(
        0.0,
        CenterDistanceMeters - (Probe.collision_envelope_radius_m + Body->radius_m));

    if (const FVector* Velocity = TractorTargetVelocitiesMetersPerSecond.Find(TargetId))
    {
        Status.TargetVelocityMetersPerSecond = *Velocity;
    }
    Status.ExpectedMotion = ExpectedMotionText(Probe.mass_kg, Status.TargetMassKilograms);

    if (Status.Detail.IsEmpty())
    {
        Status.Detail = Status.bEngaged
            ? TEXT("tractor field coupled")
            : TEXT("hold B to couple selected target");
    }
    return Status;
}

FEverwardProbeCommandResult UProbeSimulationAdapter::CommandEngageTractorField(
    double RequestedForceNewtons)
{
    const FName CommandId(TEXT("engage_tractor_field"));
    if (Core == nullptr)
    {
        return RecordCommandResult(CommandId, false, TEXT("simulation unavailable"));
    }
    if (!std::isfinite(RequestedForceNewtons) || RequestedForceNewtons <= 0.0)
    {
        return RecordCommandResult(
            CommandId,
            false,
            TEXT("tractor force must be finite and positive"));
    }

    const everward::simulation::TargetSelectionStatus Selection = Core->selected_target_status();
    if (!Selection.has_selection)
    {
        return RecordCommandResult(
            CommandId,
            false,
            TEXT("no tractor target selected // press T to select a physical target"));
    }

    const FString TargetId = UTF8_TO_TCHAR(Selection.body_id.c_str());
    const everward::simulation::StaticSphereBody* Body = FindRegisteredBody(*Core, TargetId);
    if (Body == nullptr)
    {
        return RecordCommandResult(CommandId, false, TEXT("selected target is not registered"));
    }
    if (IsBodyHeldByManipulator(Manipulators, TargetId))
    {
        return RecordCommandResult(
            CommandId,
            false,
            TEXT("selected target is already held by a manipulator"));
    }

    const auto& Probe = Core->snapshot();
    const double TargetMassKilograms = ResolvePhase2BodyMassKilograms(TargetId, Body->radius_m);
    const FString ExpectedMotion = ExpectedMotionText(Probe.mass_kg, TargetMassKilograms);

    if (Selection.surface_range_m > TractorMaxSurfaceRangeMeters)
    {
        return RecordCommandResult(
            CommandId,
            false,
            FString::Printf(
                TEXT("tractor target outside %.1f m field range // current %.1f m"),
                TractorMaxSurfaceRangeMeters,
                Selection.surface_range_m));
    }

    bTractorFieldEngaged = true;
    TractorTargetId = TargetId;
    TractorRequestedForceNewtons = FMath::Min(RequestedForceNewtons, TractorMaxForceNewtons);
    TractorAppliedForceNewtons = 0.0;
    TractorStepAccumulatorSeconds = 0.0;
    TractorTargetVelocitiesMetersPerSecond.FindOrAdd(TargetId, FVector::ZeroVector);
    TractorFieldDetail = FString::Printf(
        TEXT("coupled %s // target %.0f kg // probe %.0f kg // %s"),
        *TargetId,
        TargetMassKilograms,
        Probe.mass_kg,
        *ExpectedMotion);

    return RecordCommandResult(CommandId, true, TractorFieldDetail);
}

FEverwardProbeCommandResult UProbeSimulationAdapter::CommandDisengageTractorField()
{
    const FName CommandId(TEXT("disengage_tractor_field"));
    if (!bTractorFieldEngaged)
    {
        return RecordCommandResult(CommandId, true, TEXT("tractor field already disengaged"));
    }

    const FString ReleasedTarget = TractorTargetId;
    bTractorFieldEngaged = false;
    TractorTargetId.Reset();
    TractorAppliedForceNewtons = 0.0;
    TractorStepAccumulatorSeconds = 0.0;
    TractorFieldDetail = FString::Printf(
        TEXT("tractor released %s // target retains zero-g drift"),
        *ReleasedTarget);
    return RecordCommandResult(CommandId, true, TractorFieldDetail);
}

void UProbeSimulationAdapter::AdvanceTractorField(double DeltaSeconds)
{
    if (Core == nullptr || !std::isfinite(DeltaSeconds) || DeltaSeconds <= 0.0)
    {
        return;
    }

    // Cap one frame's contribution so an editor pause or debugger break does
    // not turn into a giant catch-up impulse when play resumes.
    TractorStepAccumulatorSeconds += FMath::Min(DeltaSeconds, 0.25);

    while (TractorStepAccumulatorSeconds >= FixedStepSeconds)
    {
        // Released objects keep the velocity they acquired while coupled.
        // StaticSphereBody predates per-body velocity, so this bridge owns only
        // that temporary velocity component while the canonical body center_m
        // remains the authoritative position consumed by target selection,
        // manipulator reach, collision, mining presentation, and save data.
        for (auto& Entry : TractorTargetVelocitiesMetersPerSecond)
        {
            if (bTractorFieldEngaged && Entry.Key == TractorTargetId)
            {
                continue;
            }
            if (Entry.Value.IsNearlyZero(1.0e-6))
            {
                continue;
            }
            if (IsBodyHeldByManipulator(Manipulators, Entry.Key))
            {
                Entry.Value = FVector::ZeroVector;
                continue;
            }

            const everward::simulation::StaticSphereBody* Body = FindRegisteredBody(*Core, Entry.Key);
            if (Body == nullptr)
            {
                Entry.Value = FVector::ZeroVector;
                continue;
            }

            const FVector NextPositionMeters =
                FVector(Body->center_m.x, Body->center_m.y, Body->center_m.z) +
                Entry.Value * FixedStepSeconds;
            Core->update_static_sphere_body_position(
                std::string(TCHAR_TO_UTF8(*Entry.Key)),
                {NextPositionMeters.X, NextPositionMeters.Y, NextPositionMeters.Z});
        }

        if (bTractorFieldEngaged)
        {
            if (IsBodyHeldByManipulator(Manipulators, TractorTargetId))
            {
                TractorTargetVelocitiesMetersPerSecond.FindOrAdd(TractorTargetId) = FVector::ZeroVector;
                bTractorFieldEngaged = false;
                TractorAppliedForceNewtons = 0.0;
                TractorFieldDetail = TEXT("tractor released because manipulator captured the target");
                TractorTargetId.Reset();
            }
            else
            {
                const everward::simulation::StaticSphereBody* Body = FindRegisteredBody(*Core, TractorTargetId);
                if (Body == nullptr)
                {
                    bTractorFieldEngaged = false;
                    TractorAppliedForceNewtons = 0.0;
                    TractorFieldDetail = TEXT("tractor target deregistered // field released");
                    TractorTargetId.Reset();
                }
                else
                {
                    const auto& ProbeSnapshot = Core->snapshot();
                    const FVector TargetVelocity =
                        TractorTargetVelocitiesMetersPerSecond.FindRef(TractorTargetId);

                    everward::simulation::TractorBodyState ProbeBody;
                    ProbeBody.body_id = ProbeSnapshot.probe_id;
                    ProbeBody.position_m = ProbeSnapshot.position_m;
                    ProbeBody.velocity_mps = ProbeSnapshot.velocity_mps;
                    ProbeBody.mass_kg = ProbeSnapshot.mass_kg;
                    ProbeBody.radius_m = ProbeSnapshot.collision_envelope_radius_m;

                    everward::simulation::TractorBodyState TargetBody;
                    TargetBody.body_id = Body->body_id;
                    TargetBody.position_m = Body->center_m;
                    TargetBody.velocity_mps = {
                        TargetVelocity.X,
                        TargetVelocity.Y,
                        TargetVelocity.Z,
                    };
                    TargetBody.mass_kg = ResolvePhase2BodyMassKilograms(
                        TractorTargetId,
                        Body->radius_m);
                    TargetBody.radius_m = Body->radius_m;

                    const everward::simulation::TractorFieldSystem Field({
                        TractorMaxSurfaceRangeMeters,
                        TractorMaxForceNewtons,
                    });
                    const everward::simulation::TractorFieldStepResult Result = Field.step(
                        ProbeBody,
                        TargetBody,
                        FixedStepSeconds,
                        TractorRequestedForceNewtons);

                    if (!Result.accepted)
                    {
                        bTractorFieldEngaged = false;
                        TractorAppliedForceNewtons = 0.0;
                        TractorFieldDetail = UTF8_TO_TCHAR(Result.detail.c_str());
                        TractorTargetId.Reset();
                    }
                    else
                    {
                        // The probe's position remains owned by SimulationCore's
                        // normal fixed-step integration; only its velocity is
                        // updated here. The target has no core integrator yet,
                        // so its tractor result writes directly into the same
                        // registered center_m every other world system reads.
                        Core->set_velocity_mps(Result.probe_after.velocity_mps);
                        Core->update_static_sphere_body_position(
                            Result.target_after.body_id,
                            Result.target_after.position_m);
                        TractorTargetVelocitiesMetersPerSecond.FindOrAdd(TractorTargetId) =
                            ToUnrealVector(Result.target_after.velocity_mps);
                        TractorAppliedForceNewtons = Result.applied_beam_force_n;
                        TractorFieldDetail = UTF8_TO_TCHAR(Result.detail.c_str());
                    }
                }
            }
        }

        TractorStepAccumulatorSeconds -= FixedStepSeconds;
    }
}
