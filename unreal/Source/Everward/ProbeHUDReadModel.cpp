#include "ProbeHUDReadModel.h"

#include "ProbeSimulationAdapter.h"

namespace
{
float SafeFraction(double Amount, double Capacity)
{
    if (!FMath::IsFinite(Amount) || !FMath::IsFinite(Capacity) || Capacity <= 0.0)
    {
        return 0.0f;
    }
    return static_cast<float>(FMath::Clamp(Amount / Capacity, 0.0, 1.0));
}

float SafeIntegrity(double Integrity)
{
    return FMath::IsFinite(Integrity)
        ? static_cast<float>(FMath::Clamp(Integrity, 0.0, 1.0))
        : 0.0f;
}
}

FEverwardHUDReadModel FEverwardHUDReadModel::FromAuthoritativeState(
    const FEverwardProbeTelemetry& Telemetry,
    const FEverwardTargetSelectionStatus& Target,
    const FString& RuntimeFixItStatus)
{
    FEverwardHUDReadModel Out;
    Out.bHasProbeState = !Telemetry.ProbeId.IsEmpty();
    if (!Out.bHasProbeState)
    {
        Out.CurrentAction = TEXT("INITIALIZING");
        Out.FixItStatus = TEXT("FIX_IT // AWAITING PROBE");
        return Out;
    }

    Out.ProbeId = Telemetry.ProbeId;
    Out.EnergyFraction = SafeFraction(Telemetry.StoredEnergyJoules, Telemetry.EnergyCapacityJoules);
    Out.WeakestSubsystemFraction = FMath::Min(
        FMath::Min(SafeIntegrity(Telemetry.SensorsIntegrity), SafeIntegrity(Telemetry.PropulsionIntegrity)),
        FMath::Min(SafeIntegrity(Telemetry.ComputationIntegrity), SafeIntegrity(Telemetry.ThermalIntegrity)));
    Out.StorageFraction = SafeFraction(Telemetry.StorageUsedKilograms, Telemetry.StorageCapacityKilograms);
    Out.CoreTemperatureKelvin = FMath::IsFinite(Telemetry.TemperatureKelvin)
        ? Telemetry.TemperatureKelvin : 0.0;
    Out.bHasTarget = Target.bHasSelection;
    if (Out.bHasTarget)
    {
        Out.TargetId = Target.TargetId;
        Out.TargetSurfaceRangeMeters = FMath::IsFinite(Target.SurfaceRangeMeters)
            ? FMath::Max(0.0, Target.SurfaceRangeMeters) : 0.0;
    }

    if (Telemetry.bIsEnergyDepleted)
    {
        Out.CurrentAction = TEXT("ENERGY DEPLETED");
    }
    else if (Telemetry.bIsOverheated)
    {
        Out.CurrentAction = TEXT("THERMAL LOCKOUT");
    }
    else if (Telemetry.bIsScanning)
    {
        Out.CurrentAction = TEXT("SCANNING");
    }
    else if (Telemetry.VelocityMetersPerSecond.SizeSquared() > 0.0001)
    {
        Out.CurrentAction = TEXT("MOVING");
    }
    else
    {
        Out.CurrentAction = TEXT("STANDBY");
    }

    // The live Fix_It executor is an existing runtime actor. Never infer
    // "repairing" merely because subsystem integrity is low.
    Out.FixItStatus = RuntimeFixItStatus.IsEmpty()
        ? TEXT("FIX_IT // UNAVAILABLE")
        : RuntimeFixItStatus;
    return Out;
}
