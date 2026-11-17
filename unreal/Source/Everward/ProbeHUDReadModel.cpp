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

FEverwardHUDActionHint FEverwardHUDActionHint::FromAuthoritativeState(
    const FEverwardProbeTelemetry& Telemetry,
    const FEverwardTargetSelectionStatus& Target,
    const FEverwardMiningStatus& Mining,
    const TArray<FEverwardProbeCapability>& Capabilities,
    const TArray<FEverwardManipulatorArmState>& ManipulatorArms)
{
    FEverwardHUDActionHint Hint;
    // The command-acceptance gates remain in the adapter/simulation. This
    // projection only suggests existing keys, never asserts "mining ready".
    if (Telemetry.bIsEnergyDepleted)
    {
        Hint.Text = TEXT("ENERGY DEPLETED // [TAB] SYSTEMS");
        Hint.bWarning = true;
        return Hint;
    }
    if (Telemetry.bIsOverheated)
    {
        Hint.Text = TEXT("THERMAL LOCKOUT // [TAB] SYSTEMS");
        Hint.bWarning = true;
        return Hint;
    }
    if (Telemetry.bIsScanning)
    {
        Hint.Text = FString::Printf(TEXT("SCANNING // %.1f S REMAINING"),
            FMath::Max(0.0, Telemetry.ScanRemainingSeconds));
        return Hint;
    }
    if (!Target.bHasSelection)
    {
        Hint.Text = TEXT("NEXT [T] CYCLE PHYSICAL TARGET");
        return Hint;
    }

    // GetMiningStatus() is currently specific to the bootstrap deposit;
    // selected reference bodies and collectible samples must not inherit its
    // survey state, working reach or assumed extractability.
    if (Target.TargetId != Mining.TargetId || Mining.TargetId.IsEmpty())
    {
        Hint.Text = TEXT("NEXT [M] ARM VIEW // [F] TRY GRASP");
        return Hint;
    }

    if (!Mining.bSurveyed)
    {
        // Enter dispatches a scan ONLY in TAB systems with sensors selected.
        // It is not a global scan key, and the first-phase controller scans
        // this one bootstrap target rather than the general selection.
        bool bSensorActionAvailable = false;
        for (const FEverwardProbeCapability& Capability : Capabilities)
        {
            if (Capability.CapabilityId == FName(TEXT("sensors")))
            {
                bSensorActionAvailable = Capability.bOperational && Capability.bAvailable;
                break;
            }
        }
        if (!bSensorActionAvailable)
        {
            Hint.Text = TEXT("SENSORS BLOCKED // [TAB] INSPECT");
            Hint.bWarning = true;
            return Hint;
        }
        Hint.Text = TEXT("NEXT [TAB] SELECT SENSORS // [ENTER] SCAN");
        return Hint;
    }

    if (Mining.DepositRemainingKilograms <= 0.0)
    {
        Hint.Text = TEXT("DEPOSIT EXHAUSTED // [T] NEXT TARGET");
        return Hint;
    }

    if (Telemetry.StorageCapacityKilograms > 0.0 &&
        Telemetry.StorageUsedKilograms >= Telemetry.StorageCapacityKilograms)
    {
        Hint.Text = TEXT("STORAGE FULL // [TAB] SYSTEMS");
        Hint.bWarning = true;
        return Hint;
    }

    bool bHasOperationalToolArm = false;
    for (const FEverwardManipulatorArmState& Arm : ManipulatorArms)
    {
        if (Arm.bIsDeployed && !Arm.bIsDeploying && !Arm.bIsStowing && Arm.bToolAttached)
        {
            bHasOperationalToolArm = true;
            break;
        }
    }
    if (!bHasOperationalToolArm)
    {
        Hint.Text = TEXT("NEXT [1/2] DEPLOY ARM // [7] TOOL");
        return Hint;
    }

    // P computes a pose on demand; G tests the *actual tip-to-target* reach.
    // Probe hull surface range must NOT be compared to tool working reach.
    // Neither action is presented as guaranteed to succeed.
    Hint.Text = TEXT("NEXT [P] TRY STAGING // [G] TRY MINE");
    return Hint;
}
