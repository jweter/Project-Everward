#include "EverwardPlayerController.h"

#include "EverwardHUD.h"
#include "EverwardPhase2TestEnvironment.h"
#include "EverwardProbePawn.h"
#include "InputCoreTypes.h"
#include "ProbeSimulationAdapter.h"

namespace
{
bool ResolvePowerSubsystem(FName CapabilityId, EEverwardPowerSubsystem& OutSubsystem)
{
    if (CapabilityId == FName(TEXT("sensors")))
    {
        OutSubsystem = EEverwardPowerSubsystem::Sensors;
        return true;
    }
    if (CapabilityId == FName(TEXT("propulsion")))
    {
        OutSubsystem = EEverwardPowerSubsystem::Propulsion;
        return true;
    }
    if (CapabilityId == FName(TEXT("computation")))
    {
        OutSubsystem = EEverwardPowerSubsystem::Computation;
        return true;
    }
    if (CapabilityId == FName(TEXT("thermal")))
    {
        OutSubsystem = EEverwardPowerSubsystem::Thermal;
        return true;
    }
    return false;
}
}

void AEverwardPlayerController::BeginPlay()
{
    Super::BeginPlay();

    bShowMouseCursor = false;
    FInputModeGameOnly InputMode;
    SetInputMode(InputMode);
}

void AEverwardPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    if (InputComponent == nullptr)
    {
        return;
    }

    InputComponent->BindAxis(TEXT("EverwardLookYaw"), this, &AEverwardPlayerController::LookYaw);
    InputComponent->BindAxis(TEXT("EverwardLookPitch"), this, &AEverwardPlayerController::LookPitch);
    InputComponent->BindAxis(TEXT("EverwardCameraZoom"), this, &AEverwardPlayerController::ZoomCamera);

    InputComponent->BindKey(EKeys::Tab, IE_Pressed, this, &AEverwardPlayerController::ToggleSystemsPanel);
    InputComponent->BindKey(EKeys::RightBracket, IE_Pressed, this, &AEverwardPlayerController::SelectNextCapability);
    InputComponent->BindKey(EKeys::LeftBracket, IE_Pressed, this, &AEverwardPlayerController::SelectPreviousCapability);

    InputComponent->BindKey(EKeys::Enter, IE_Pressed, this, &AEverwardPlayerController::ExecutePrimarySystemAction);
    InputComponent->BindKey(EKeys::BackSpace, IE_Pressed, this, &AEverwardPlayerController::ExecuteSecondarySystemAction);
    InputComponent->BindKey(EKeys::PageUp, IE_Pressed, this, &AEverwardPlayerController::IncreaseSelectedSystemPower);
    InputComponent->BindKey(EKeys::PageDown, IE_Pressed, this, &AEverwardPlayerController::DecreaseSelectedSystemPower);

    InputComponent->BindKey(EKeys::W, IE_Pressed, this, &AEverwardPlayerController::IncreaseForwardVelocity);
    InputComponent->BindKey(EKeys::S, IE_Pressed, this, &AEverwardPlayerController::DecreaseForwardVelocity);
    InputComponent->BindKey(EKeys::D, IE_Pressed, this, &AEverwardPlayerController::IncreaseLateralVelocity);
    InputComponent->BindKey(EKeys::A, IE_Pressed, this, &AEverwardPlayerController::DecreaseLateralVelocity);
    InputComponent->BindKey(EKeys::E, IE_Pressed, this, &AEverwardPlayerController::IncreaseVerticalVelocity);
    InputComponent->BindKey(EKeys::Q, IE_Pressed, this, &AEverwardPlayerController::DecreaseVerticalVelocity);

    InputComponent->BindKey(EKeys::J, IE_Pressed, this, &AEverwardPlayerController::YawProbeLeft);
    InputComponent->BindKey(EKeys::L, IE_Pressed, this, &AEverwardPlayerController::YawProbeRight);
    InputComponent->BindKey(EKeys::I, IE_Pressed, this, &AEverwardPlayerController::PitchProbeUp);
    InputComponent->BindKey(EKeys::K, IE_Pressed, this, &AEverwardPlayerController::PitchProbeDown);
    InputComponent->BindKey(EKeys::U, IE_Pressed, this, &AEverwardPlayerController::RollProbeLeft);
    InputComponent->BindKey(EKeys::O, IE_Pressed, this, &AEverwardPlayerController::RollProbeRight);

    // Slice 6 foundation: deploy/stow toggle per arm, plus a single tool
    // attach/detach toggle. Joint articulation input remains a follow-up.
    InputComponent->BindKey(EKeys::One, IE_Pressed, this, &AEverwardPlayerController::TogglePortManipulatorArm);
    InputComponent->BindKey(EKeys::Two, IE_Pressed, this, &AEverwardPlayerController::ToggleStarboardManipulatorArm);
    InputComponent->BindKey(EKeys::Three, IE_Pressed, this, &AEverwardPlayerController::ToggleManipulatorTool);

    // Retain the original engineering-shell aliases while the final input
    // model remains intentionally unsettled.
    InputComponent->BindKey(EKeys::Up, IE_Pressed, this, &AEverwardPlayerController::IncreaseForwardVelocity);
    InputComponent->BindKey(EKeys::Down, IE_Pressed, this, &AEverwardPlayerController::DecreaseForwardVelocity);
    InputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &AEverwardPlayerController::StopPropulsion);
}

void AEverwardPlayerController::ToggleSystemsPanel()
{
    if (AEverwardHUD* EverwardHUD = Cast<AEverwardHUD>(GetHUD()))
    {
        EverwardHUD->ToggleSystemsPanel();
    }
}

void AEverwardPlayerController::SelectNextCapability()
{
    if (AEverwardHUD* EverwardHUD = Cast<AEverwardHUD>(GetHUD()))
    {
        EverwardHUD->SelectNextCapability();
    }
}

void AEverwardPlayerController::SelectPreviousCapability()
{
    if (AEverwardHUD* EverwardHUD = Cast<AEverwardHUD>(GetHUD()))
    {
        EverwardHUD->SelectPreviousCapability();
    }
}

void AEverwardPlayerController::ExecutePrimarySystemAction()
{
    const AEverwardHUD* EverwardHUD = Cast<AEverwardHUD>(GetHUD());
    UProbeSimulationAdapter* Adapter = GetProbeAdapter();
    if (EverwardHUD == nullptr || !EverwardHUD->IsSystemsPanelExpanded() || Adapter == nullptr)
    {
        return;
    }

    const FName CapabilityId = GetSelectedCapabilityId();
    if (CapabilityId == FName(TEXT("sensors")))
    {
        (void)Adapter->CommandStartScan(
            FString(AEverwardPhase2TestEnvironment::BootstrapScanTargetId),
            Phase2ScanDurationSeconds);
    }
    else if (CapabilityId == FName(TEXT("computation")))
    {
        (void)Adapter->CommandInstallBasicSurvivalPolicy();
    }
}

void AEverwardPlayerController::ExecuteSecondarySystemAction()
{
    const AEverwardHUD* EverwardHUD = Cast<AEverwardHUD>(GetHUD());
    UProbeSimulationAdapter* Adapter = GetProbeAdapter();
    if (EverwardHUD == nullptr || !EverwardHUD->IsSystemsPanelExpanded() || Adapter == nullptr)
    {
        return;
    }

    const FName CapabilityId = GetSelectedCapabilityId();
    if (CapabilityId == FName(TEXT("sensors")))
    {
        (void)Adapter->CommandCancelScan();
    }
    else if (CapabilityId == FName(TEXT("computation")))
    {
        (void)Adapter->CommandClearSoftwarePolicy();
    }
}

void AEverwardPlayerController::IncreaseSelectedSystemPower()
{
    AdjustSelectedSystemPower(PowerAdjustmentWatts);
}

void AEverwardPlayerController::DecreaseSelectedSystemPower()
{
    AdjustSelectedSystemPower(-PowerAdjustmentWatts);
}

void AEverwardPlayerController::IncreaseForwardVelocity()
{
    AdjustLocalVelocityMetersPerSecond(FVector(VelocityAdjustmentMetersPerSecond, 0.0, 0.0));
}

void AEverwardPlayerController::DecreaseForwardVelocity()
{
    AdjustLocalVelocityMetersPerSecond(FVector(-VelocityAdjustmentMetersPerSecond, 0.0, 0.0));
}

void AEverwardPlayerController::IncreaseLateralVelocity()
{
    AdjustLocalVelocityMetersPerSecond(FVector(0.0, VelocityAdjustmentMetersPerSecond, 0.0));
}

void AEverwardPlayerController::DecreaseLateralVelocity()
{
    AdjustLocalVelocityMetersPerSecond(FVector(0.0, -VelocityAdjustmentMetersPerSecond, 0.0));
}

void AEverwardPlayerController::IncreaseVerticalVelocity()
{
    AdjustLocalVelocityMetersPerSecond(FVector(0.0, 0.0, VelocityAdjustmentMetersPerSecond));
}

void AEverwardPlayerController::DecreaseVerticalVelocity()
{
    AdjustLocalVelocityMetersPerSecond(FVector(0.0, 0.0, -VelocityAdjustmentMetersPerSecond));
}

void AEverwardPlayerController::StopPropulsion()
{
    // Spacebar is the probe's global emergency brake. It must not depend on which
    // contextual systems page happens to be selected; scan/computation inspection
    // can never make the player lose access to stop authority.
    UProbeSimulationAdapter* Adapter = GetProbeAdapter();
    if (Adapter == nullptr)
    {
        return;
    }

    (void)Adapter->CommandSetVelocityMetersPerSecond(FVector::ZeroVector);
}

void AEverwardPlayerController::YawProbeLeft()
{
    AdjustAttitudeDegrees(FRotator(0.0, -AttitudeAdjustmentDegrees, 0.0));
}

void AEverwardPlayerController::YawProbeRight()
{
    AdjustAttitudeDegrees(FRotator(0.0, AttitudeAdjustmentDegrees, 0.0));
}

void AEverwardPlayerController::PitchProbeUp()
{
    AdjustAttitudeDegrees(FRotator(AttitudeAdjustmentDegrees, 0.0, 0.0));
}

void AEverwardPlayerController::PitchProbeDown()
{
    AdjustAttitudeDegrees(FRotator(-AttitudeAdjustmentDegrees, 0.0, 0.0));
}

void AEverwardPlayerController::RollProbeLeft()
{
    AdjustAttitudeDegrees(FRotator(0.0, 0.0, -AttitudeAdjustmentDegrees));
}

void AEverwardPlayerController::RollProbeRight()
{
    AdjustAttitudeDegrees(FRotator(0.0, 0.0, AttitudeAdjustmentDegrees));
}

void AEverwardPlayerController::TogglePortManipulatorArm()
{
    ToggleManipulatorArmDeployment(EEverwardManipulatorArmId::Port);
}

void AEverwardPlayerController::ToggleStarboardManipulatorArm()
{
    ToggleManipulatorArmDeployment(EEverwardManipulatorArmId::Starboard);
}

void AEverwardPlayerController::ToggleManipulatorTool()
{
    UProbeSimulationAdapter* Adapter = GetProbeAdapter();
    if (Adapter == nullptr)
    {
        return;
    }

    // Attach/detach targets whichever arm is deployed; Port takes priority
    // when both are deployed. This single-key toggle is a Slice 6 foundation
    // control, not a final per-arm tool interface.
    for (const FEverwardManipulatorArmState& ArmState : Adapter->GetManipulatorArmStates())
    {
        if (!ArmState.bIsDeployed)
        {
            continue;
        }
        if (ArmState.bToolAttached)
        {
            (void)Adapter->CommandDetachManipulatorTool(ArmState.ArmId);
        }
        else
        {
            (void)Adapter->CommandAttachManipulatorTool(ArmState.ArmId);
        }
        return;
    }
}

void AEverwardPlayerController::LookYaw(float Value)
{
    if (!FMath::IsNearlyZero(Value))
    {
        AddYawInput(Value * MouseLookSensitivity);
    }
}

void AEverwardPlayerController::LookPitch(float Value)
{
    if (!FMath::IsNearlyZero(Value))
    {
        AddPitchInput(Value * MouseLookSensitivity);
    }
}

void AEverwardPlayerController::ZoomCamera(float Value)
{
    if (FMath::IsNearlyZero(Value))
    {
        return;
    }

    if (AEverwardProbePawn* Probe = Cast<AEverwardProbePawn>(GetPawn()))
    {
        Probe->AdjustCameraZoom(-Value * CameraZoomStepCentimeters);
    }
}

UProbeSimulationAdapter* AEverwardPlayerController::GetProbeAdapter() const
{
    const AEverwardProbePawn* Probe = Cast<AEverwardProbePawn>(GetPawn());
    return Probe != nullptr ? Probe->GetSimulationAdapter() : nullptr;
}

FName AEverwardPlayerController::GetSelectedCapabilityId() const
{
    const AEverwardHUD* EverwardHUD = Cast<AEverwardHUD>(GetHUD());
    UProbeSimulationAdapter* Adapter = GetProbeAdapter();
    if (EverwardHUD == nullptr || Adapter == nullptr)
    {
        return NAME_None;
    }

    const TArray<FEverwardProbeCapability> Capabilities = Adapter->GetInstalledCapabilities();
    if (Capabilities.IsEmpty())
    {
        return NAME_None;
    }

    const int32 Index = FMath::Clamp(EverwardHUD->GetSelectedCapabilityIndex(), 0, Capabilities.Num() - 1);
    return Capabilities[Index].CapabilityId;
}

double AEverwardPlayerController::GetSelectedCapabilityAllocatedPowerWatts() const
{
    const AEverwardHUD* EverwardHUD = Cast<AEverwardHUD>(GetHUD());
    UProbeSimulationAdapter* Adapter = GetProbeAdapter();
    if (EverwardHUD == nullptr || Adapter == nullptr)
    {
        return 0.0;
    }

    const TArray<FEverwardProbeCapability> Capabilities = Adapter->GetInstalledCapabilities();
    if (Capabilities.IsEmpty())
    {
        return 0.0;
    }

    const int32 Index = FMath::Clamp(EverwardHUD->GetSelectedCapabilityIndex(), 0, Capabilities.Num() - 1);
    return Capabilities[Index].AllocatedPowerWatts;
}

void AEverwardPlayerController::AdjustSelectedSystemPower(double DeltaWatts)
{
    const AEverwardHUD* EverwardHUD = Cast<AEverwardHUD>(GetHUD());
    UProbeSimulationAdapter* Adapter = GetProbeAdapter();
    if (EverwardHUD == nullptr || !EverwardHUD->IsSystemsPanelExpanded() || Adapter == nullptr)
    {
        return;
    }

    EEverwardPowerSubsystem Subsystem = EEverwardPowerSubsystem::Sensors;
    if (!ResolvePowerSubsystem(GetSelectedCapabilityId(), Subsystem))
    {
        return;
    }

    const double RequestedWatts = FMath::Max(0.0, GetSelectedCapabilityAllocatedPowerWatts() + DeltaWatts);
    (void)Adapter->CommandAllocatePower(Subsystem, RequestedWatts);
}

void AEverwardPlayerController::AdjustLocalVelocityMetersPerSecond(
    const FVector& DeltaLocalVelocity)
{
    const AEverwardHUD* EverwardHUD = Cast<AEverwardHUD>(GetHUD());
    UProbeSimulationAdapter* Adapter = GetProbeAdapter();
    if (EverwardHUD == nullptr || !EverwardHUD->IsSystemsPanelExpanded() || Adapter == nullptr)
    {
        return;
    }

    if (GetSelectedCapabilityId() != FName(TEXT("propulsion")))
    {
        return;
    }

    (void)Adapter->CommandAdjustLocalVelocityMetersPerSecond(DeltaLocalVelocity);
}

void AEverwardPlayerController::AdjustAttitudeDegrees(const FRotator& DeltaAttitude)
{
    const AEverwardHUD* EverwardHUD = Cast<AEverwardHUD>(GetHUD());
    UProbeSimulationAdapter* Adapter = GetProbeAdapter();
    if (EverwardHUD == nullptr || !EverwardHUD->IsSystemsPanelExpanded() || Adapter == nullptr)
    {
        return;
    }

    if (GetSelectedCapabilityId() != FName(TEXT("propulsion")))
    {
        return;
    }

    (void)Adapter->CommandAdjustAttitudeDegrees(DeltaAttitude);
}

void AEverwardPlayerController::ToggleManipulatorArmDeployment(EEverwardManipulatorArmId ArmId)
{
    UProbeSimulationAdapter* Adapter = GetProbeAdapter();
    if (Adapter == nullptr)
    {
        return;
    }

    for (const FEverwardManipulatorArmState& ArmState : Adapter->GetManipulatorArmStates())
    {
        if (ArmState.ArmId != ArmId)
        {
            continue;
        }

        // A single key toggles direction rather than requiring separate
        // deploy/stow keys per arm. Mid-transition presses reverse cleanly
        // (ManipulatorRig::begin_deploy/begin_stow both support this).
        if (ArmState.bIsDeployed || ArmState.bIsDeploying)
        {
            (void)Adapter->CommandStowManipulatorArm(ArmId);
        }
        else
        {
            (void)Adapter->CommandDeployManipulatorArm(ArmId);
        }
        return;
    }
}
