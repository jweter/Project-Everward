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
    const FEverwardTargetKnowledgeStatus& Knowledge,
    const FEverwardMiningStatus& Mining,
    const TArray<FEverwardProbeCapability>& Capabilities,
    const TArray<FEverwardManipulatorArmState>& ManipulatorArms)
{
    FEverwardHUDActionHint Hint;
    // Presentation only. Only an accepted adapter/core command can start a
    // scan or extract material. These strings never authorize gameplay.
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

    bool bSensorsAvailable = false;
    for (const FEverwardProbeCapability& Capability : Capabilities)
    {
        if (Capability.CapabilityId == FName(TEXT("sensors")))
        {
            bSensorsAvailable = Capability.bOperational && Capability.bAvailable;
            break;
        }
    }

    if (Telemetry.bIsScanning)
    {
        // The bound scan identity need not equal today's physical selection.
        // Reserve the time suffix before abbreviating the target ID.
        const FString Prefix = bSensorsAvailable ? TEXT("SCAN ") : TEXT("PAUSED ");
        const FString Suffix = FString::Printf(TEXT(" // %.1f S LEFT"),
            FMath::Max(0.0, Telemetry.ScanRemainingSeconds));
        const int32 IdBudget = FMath::Max(0, 49 - Prefix.Len() - Suffix.Len());
        FString ShortId = Telemetry.ActiveScanTargetId;
        if (ShortId.Len() > IdBudget)
        {
            ShortId = IdBudget >= 7
                ? ShortId.Left(IdBudget - 6) + TEXT("...") + ShortId.Right(3)
                : ShortId.Left(IdBudget);
        }
        Hint.Text = Prefix + ShortId + Suffix;
        Hint.bWarning = !bSensorsAvailable;
        return Hint;
    }
    if (!Target.bHasSelection)
    {
        Hint.Text = TEXT("NEXT [T] CYCLE PHYSICAL TARGET");
        return Hint;
    }

    // The current mining bridge describes *only* the bootstrap deposit.
    // A reference or sample can be scanned, but mining status is not proof
    // of that other object's material composition or mineability.
    if (Target.TargetId != Mining.TargetId || Mining.TargetId.IsEmpty())
    {
        if (!Knowledge.bHasKnowledge ||
            Knowledge.Level == EEverwardKnowledgeLevel::Unknown ||
            Knowledge.Confidence < 0.999)
        {
            if (!bSensorsAvailable)
            {
                Hint.Text = TEXT("SENSORS BLOCKED // [TAB] INSPECT");
                Hint.bWarning = true;
            }
            else
            {
                Hint.Text = TEXT("NEXT [TAB] SELECT SENSORS // [ENTER] SCAN");
            }
            return Hint;
        }
        Hint.Text = Knowledge.Level == EEverwardKnowledgeLevel::Characterized
            ? TEXT("CHARACTERIZED // [M] ARM / [F] TRY GRASP")
            : TEXT("OBSERVED // [T] NEXT / [F] TRY GRASP");
        return Hint;
    }

    if (!Mining.bSurveyed)
    {
        // A bootstrap mining survey requires its own completed scan event,
        // not merely some passive knowledge confidence.
        if (!bSensorsAvailable)
        {
            Hint.Text = TEXT("SENSORS BLOCKED // [TAB] INSPECT");
            Hint.bWarning = true;
        }
        else
        {
            Hint.Text = TEXT("NEXT [TAB] SELECT SENSORS // [ENTER] SCAN");
        }
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

    // P tries a physical staging solution; G validates tip-to-deposit reach.
    // Never compare probe hull surface range with mining tool working reach.
    Hint.Text = TEXT("NEXT [P] TRY STAGING // [G] TRY MINE");
    return Hint;
}
