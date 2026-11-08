#include "EverwardPlayerController.h"

#include "Engine/Engine.h"
#include "EverwardHUD.h"
#include "EverwardPhase2TestEnvironment.h"
#include "EverwardProbePawn.h"
#include "ProbeSimulationAdapter.h"

namespace
{
struct FMiningTargetChoice
{
    FString TargetId;
    FVector CenterMeters = FVector::ZeroVector;
    double RadiusMeters = 0.0;
};

TArray<FMiningTargetChoice> GetAvailableMiningTargets()
{
    // The target-selection interface is intentionally an array from day one.
    // Phase 2 currently exposes one mineable body; later resource bodies can
    // be added here (and then moved behind the scan/resource registry) without
    // changing the player-control contract.
    return {
        FMiningTargetChoice{
            FString(AEverwardPhase2TestEnvironment::BootstrapScanTargetId),
            FVector(
                AEverwardPhase2TestEnvironment::BootstrapBodyCenterXMeters,
                AEverwardPhase2TestEnvironment::BootstrapBodyCenterYMeters,
                AEverwardPhase2TestEnvironment::BootstrapBodyCenterZMeters),
            AEverwardPhase2TestEnvironment::BootstrapBodyRadiusMeters}
    };
}

void ShowMiningControlMessage(const FString& Message)
{
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, Message);
    }
}
}

void AEverwardPlayerController::ToggleSelectedManipulatorTool()
{
    UProbeSimulationAdapter* Adapter = GetProbeAdapter();
    const AEverwardHUD* EverwardHUD = Cast<AEverwardHUD>(GetHUD());
    if (Adapter == nullptr || EverwardHUD == nullptr)
    {
        return;
    }

    const EEverwardManipulatorArmId SelectedArm =
        EverwardHUD->GetSelectedManipulatorArmIndex() == 1
            ? EEverwardManipulatorArmId::Starboard
            : EEverwardManipulatorArmId::Port;

    for (const FEverwardManipulatorArmState& ArmState : Adapter->GetManipulatorArmStates())
    {
        if (ArmState.ArmId != SelectedArm)
        {
            continue;
        }

        if (!ArmState.bIsDeployed)
        {
            ShowMiningControlMessage(TEXT("Selected arm must be deployed before attaching a mining tool"));
            return;
        }

        if (ArmState.bToolAttached)
        {
            (void)Adapter->CommandDetachManipulatorTool(SelectedArm);
            ShowMiningControlMessage(SelectedArm == EEverwardManipulatorArmId::Port
                ? TEXT("Mining tool detached from PORT arm")
                : TEXT("Mining tool detached from STARBOARD arm"));
        }
        else
        {
            (void)Adapter->CommandAttachManipulatorTool(SelectedArm);
            ShowMiningControlMessage(SelectedArm == EEverwardManipulatorArmId::Port
                ? TEXT("Mining tool attached to PORT arm")
                : TEXT("Mining tool attached to STARBOARD arm"));
        }
        return;
    }
}

void AEverwardPlayerController::CycleMiningTarget()
{
    const TArray<FMiningTargetChoice> Targets = GetAvailableMiningTargets();
    if (Targets.IsEmpty())
    {
        ShowMiningControlMessage(TEXT("No mining targets available"));
        return;
    }

    SelectedMiningTargetIndex = (SelectedMiningTargetIndex + 1) % Targets.Num();
    bAutoApproachMiningTarget = false;

    const FMiningTargetChoice& Target = Targets[SelectedMiningTargetIndex];
    ShowMiningControlMessage(FString::Printf(TEXT("Mining target selected: %s"), *Target.TargetId));
}

void AEverwardPlayerController::ToggleAutoApproachMiningTarget()
{
    UProbeSimulationAdapter* Adapter = GetProbeAdapter();
    if (Adapter == nullptr)
    {
        return;
    }

    const FEverwardMiningStatus MiningStatus = Adapter->GetMiningStatus();
    if (!MiningStatus.bSurveyed)
    {
        ShowMiningControlMessage(TEXT("Scan and survey the mining target before auto-approach"));
        return;
    }

    if (bAutoApproachMiningTarget)
    {
        bAutoApproachMiningTarget = false;
        (void)Adapter->CommandSetVelocityMetersPerSecond(FVector::ZeroVector);
        ShowMiningControlMessage(TEXT("Mining auto-approach cancelled"));
        return;
    }

    const AEverwardHUD* EverwardHUD = Cast<AEverwardHUD>(GetHUD());
    const TArray<FMiningTargetChoice> Targets = GetAvailableMiningTargets();
    if (EverwardHUD == nullptr || Targets.IsEmpty())
    {
        return;
    }
    SelectedMiningTargetIndex = FMath::Clamp(SelectedMiningTargetIndex, 0, Targets.Num() - 1);
    const bool bStarboardArm = EverwardHUD->GetSelectedManipulatorArmIndex() == 1;
    const EEverwardManipulatorArmId SelectedArm = bStarboardArm ? EEverwardManipulatorArmId::Starboard : EEverwardManipulatorArmId::Port;

    // Issue #372 P0-1: the staging point comes from the selected arm's real kinematics at the probe's current attitude,
    // not a fixed world-axis side offset (from which no joint pose could reach the deposit). Solved once, here.
    const FEverwardMiningStagingSolution Solution =
        Adapter->GetMiningStagingSolution(SelectedArm, Targets[SelectedMiningTargetIndex].TargetId);
    if (!Solution.bHasSolution)
    {
        ShowMiningControlMessage(FString::Printf(
            TEXT("No collision-free mining pose for the %s arm at this attitude; rotate the probe and press P again"),
            bStarboardArm ? TEXT("STARBOARD") : TEXT("PORT")));
        return;
    }
    AutoApproachStagingPointMeters = Solution.ProbePositionMeters;
    AutoApproachShoulderDegrees = Solution.ShoulderDegrees;
    AutoApproachElbowDegrees = Solution.ElbowDegrees;
    AutoApproachWristDegrees = Solution.WristDegrees;
    AutoApproachToolGapMeters = Solution.ToolSurfaceGapMeters;
    bAutoApproachMiningTarget = true;
    ShowMiningControlMessage(TEXT("Mining auto-approach engaged; SPACE cancels/holds position"));
}

void AEverwardPlayerController::AlignSelectedArmToMiningPose()
{
    UProbeSimulationAdapter* Adapter = GetProbeAdapter();
    const AEverwardHUD* EverwardHUD = Cast<AEverwardHUD>(GetHUD());
    if (Adapter == nullptr || EverwardHUD == nullptr)
    {
        return;
    }
    const bool bStarboardArm = EverwardHUD->GetSelectedManipulatorArmIndex() == 1;
    const EEverwardManipulatorArmId SelectedArm = bStarboardArm ? EEverwardManipulatorArmId::Starboard : EEverwardManipulatorArmId::Port;
    const TCHAR* ArmName = bStarboardArm ? TEXT("STARBOARD") : TEXT("PORT");

    for (const FEverwardManipulatorArmState& ArmState : Adapter->GetManipulatorArmStates())
    {
        if (ArmState.ArmId != SelectedArm)
        {
            continue;
        }
        if (!ArmState.bIsDeployed || ArmState.bIsDeploying || ArmState.bIsStowing || !ArmState.bToolAttached)
        {
            // Staging alone is not reach: say exactly what is still missing (QA-1 / P0-1).
            ShowMiningControlMessage(FString::Printf(
                TEXT("Mining pose reached; fully deploy the %s arm and attach its tool, then press P to align it"), ArmName));
            return;
        }
        // The joints slew at their real rate; G stays the authoritative judge of tool-tip reach.
        (void)Adapter->CommandSetManipulatorJointTargetDegrees(SelectedArm, EEverwardManipulatorJoint::Shoulder, AutoApproachShoulderDegrees);
        (void)Adapter->CommandSetManipulatorJointTargetDegrees(SelectedArm, EEverwardManipulatorJoint::Elbow, AutoApproachElbowDegrees);
        (void)Adapter->CommandSetManipulatorJointTargetDegrees(SelectedArm, EEverwardManipulatorJoint::Wrist, AutoApproachWristDegrees);
        ShowMiningControlMessage(FString::Printf(
            TEXT("Mining pose reached: aligning %s arm (planned tool-tip gap %.2f m); press G once aligned"),
            ArmName, AutoApproachToolGapMeters));
        return;
    }
}

void AEverwardPlayerController::AdvanceAutoApproachMiningTarget(float DeltaSeconds)
{
    (void)DeltaSeconds;
    if (!bAutoApproachMiningTarget)
    {
        return;
    }

    UProbeSimulationAdapter* Adapter = GetProbeAdapter();
    AEverwardProbePawn* Probe = Cast<AEverwardProbePawn>(GetPawn());
    const AEverwardHUD* EverwardHUD = Cast<AEverwardHUD>(GetHUD());
    if (Adapter == nullptr || Probe == nullptr || EverwardHUD == nullptr)
    {
        bAutoApproachMiningTarget = false;
        return;
    }

    const TArray<FMiningTargetChoice> Targets = GetAvailableMiningTargets();
    if (Targets.IsEmpty())
    {
        bAutoApproachMiningTarget = false;
        return;
    }

    // The pose solved from the arm's kinematics when P engaged (see ToggleAutoApproachMiningTarget).
    const FVector StagingPointMeters = AutoApproachStagingPointMeters;

    const FVector ProbePositionMeters = Probe->GetActorLocation() * 0.01;
    const FVector DeltaMeters = StagingPointMeters - ProbePositionMeters;
    const double DistanceMeters = DeltaMeters.Size();

    if (DistanceMeters <= AutoApproachStopToleranceMeters)
    {
        (void)Adapter->CommandSetVelocityMetersPerSecond(FVector::ZeroVector);
        bAutoApproachMiningTarget = false;
        AlignSelectedArmToMiningPose();
        return;
    }

    const double Speed = FMath::Min(AutoApproachSpeedMetersPerSecond, FMath::Max(0.25, DistanceMeters));
    const FVector CommandVelocity = DeltaMeters.GetSafeNormal() * Speed;
    const FEverwardProbeCommandResult Result = Adapter->CommandSetVelocityMetersPerSecond(CommandVelocity);
    if (!Result.bAccepted)
    {
        bAutoApproachMiningTarget = false;
        ShowMiningControlMessage(FString::Printf(TEXT("Auto-approach stopped: %s"), *Result.Detail));
    }
}
