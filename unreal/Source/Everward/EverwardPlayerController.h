#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "EverwardPlayerController.generated.h"

class UProbeSimulationAdapter;
enum class EEverwardManipulatorArmId : uint8;

UCLASS()
class EVERWARD_API AEverwardPlayerController : public APlayerController
{
    GENERATED_BODY()

protected:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;
    virtual void Tick(float DeltaSeconds) override;

private:
    void ToggleControlsReference();
    void ToggleSystemsPanel();
    void SelectNextCapability();
    void SelectPreviousCapability();

    void ExecutePrimarySystemAction();
    void ExecuteSecondarySystemAction();
    void IncreaseSelectedSystemPower();
    void DecreaseSelectedSystemPower();

    void IncreaseForwardVelocity();
    void DecreaseForwardVelocity();
    void IncreaseLateralVelocity();
    void DecreaseLateralVelocity();
    void IncreaseVerticalVelocity();
    void DecreaseVerticalVelocity();
    void StopPropulsion();

    void YawProbeLeft();
    void YawProbeRight();
    void PitchProbeUp();
    void PitchProbeDown();
    void RollProbeLeft();
    void RollProbeRight();

    void TogglePortManipulatorArm();
    void ToggleStarboardManipulatorArm();
    void ToggleManipulatorTool();
    void ToggleSelectedManipulatorTool();

    void ToggleManipulatorPanel();
    void CycleManipulatorArmSelection();
    void SelectManipulatorJointShoulder();
    void SelectManipulatorJointElbow();
    void SelectManipulatorJointWrist();
    void IncreaseManipulatorJointTarget();
    void DecreaseManipulatorJointTarget();

    void CycleMiningTarget();
    void ToggleAutoApproachMiningTarget();
    void AdvanceAutoApproachMiningTarget(float DeltaSeconds);

    void ToggleJoseTakeTheWheel();
    void AdvanceJoseTakeTheWheel(float DeltaSeconds);
    void CancelJoseTakeTheWheel(bool bStopVelocity, bool bShowMessage);

    void ToggleControlledDescent();
    void AdvanceControlledDescent(float DeltaSeconds);
    void CancelControlledDescent(bool bStopVelocity, bool bShowMessage);

    void ToggleControlledHover();
    void AdvanceControlledHover(float DeltaSeconds);
    void CancelControlledHover(bool bStopVelocity, bool bShowMessage);

    void SelectNearestPhysicalTarget();
    void ToggleManipulatorGrasp();
    void CollectGraspedSample();

    void SaveGame();
    void LoadGame();

    // First-playable-loop step 9 HUD wiring: starts the next repeatable restoration objective cycle
    // (UProbeSimulationAdapter::CommandTriggerNextRestorationCycle()). Cycle rules stay in
    // objective_cycle.hpp; a rejected re-trigger while one is already Active surfaces through the
    // existing LastCommandResult banner EverwardHUD::DrawHUD() already draws, so no new rejection
    // path is introduced here.
    void TriggerNextRestorationCycle();

    void LookYaw(float Value);
    void LookPitch(float Value);
    void ZoomCamera(float Value);

    UProbeSimulationAdapter* GetProbeAdapter() const;
    FName GetSelectedCapabilityId() const;
    double GetSelectedCapabilityAllocatedPowerWatts() const;
    void AdjustSelectedSystemPower(double DeltaWatts);
    void AdjustLocalVelocityMetersPerSecond(const FVector& DeltaLocalVelocity);
    void AdjustAttitudeDegrees(const FRotator& DeltaAttitude);
    void ToggleManipulatorArmDeployment(EEverwardManipulatorArmId ArmId);
    void AdjustSelectedManipulatorJointTargetDegrees(double DeltaDegrees);

    int32 SelectedMiningTargetIndex = 0;
    bool bAutoApproachMiningTarget = false;
    // The staging pose solved when P engaged (issue #372 P0-1): where the probe stops and the selected arm's joints.
    FVector AutoApproachStagingPointMeters = FVector::ZeroVector;
    double AutoApproachShoulderDegrees = 0.0;
    double AutoApproachElbowDegrees = 0.0;
    double AutoApproachWristDegrees = 0.0;
    double AutoApproachToolGapMeters = 0.0;
    void AlignSelectedArmToMiningPose();

    bool bJoseAutopilotEngaged = false;
    FString JoseDestinationTargetId;

    bool bControlledDescentEngaged = false;
    bool bControlledHoverEngaged = false;
    // Last UProbeSimulationAdapter::GetControlledHoverGovernorNotice()
    // sequence this controller has already reacted to -- lets
    // AdvanceControlledHover() detect a fixed-step rejection exactly once.
    int64 LastSeenControlledHoverGovernorSequence = 0;
    // Same pattern (issue #239) for controlled descent's and José's own
    // fixed-step governors.
    int64 LastSeenControlledDescentGovernorSequence = 0;
    int64 LastSeenJoseAutopilotGovernorSequence = 0;

    UPROPERTY(EditAnywhere, Category="Everward|Phase2", meta=(ClampMin="0.1"))
    double Phase2ScanDurationSeconds = 10.0;
    UPROPERTY(EditAnywhere, Category="Everward|Phase2", meta=(ClampMin="1.0"))
    double PowerAdjustmentWatts = 25.0;
    UPROPERTY(EditAnywhere, Category="Everward|Phase2", meta=(ClampMin="0.1"))
    double VelocityAdjustmentMetersPerSecond = 1.0;
    UPROPERTY(EditAnywhere, Category="Everward|Phase2", meta=(ClampMin="0.1"))
    double AttitudeAdjustmentDegrees = 5.0;
    UPROPERTY(EditAnywhere, Category="Everward|Phase2", meta=(ClampMin="0.1"))
    double ManipulatorJointAdjustmentDegrees = 5.0;
    UPROPERTY(EditAnywhere, Category="Everward|Mining", meta=(ClampMin="0.1"))
    double AutoApproachSpeedMetersPerSecond = 2.0;
    UPROPERTY(EditAnywhere, Category="Everward|Mining", meta=(ClampMin="0.01"))
    double AutoApproachStopToleranceMeters = 0.25;
    UPROPERTY(EditAnywhere, Category="Everward|Autopilot", meta=(ClampMin="0.1"))
    double JoseCruiseSpeedMetersPerSecond = 6.0;
    UPROPERTY(EditAnywhere, Category="Everward|Autopilot", meta=(ClampMin="0.1"))
    double JoseArrivalSurfaceRangeMeters = 20.0;
    UPROPERTY(EditAnywhere, Category="Everward|Autopilot", meta=(ClampMin="0.01"))
    double JoseArrivalToleranceMeters = 0.5;
    UPROPERTY(EditAnywhere, Category="Everward|Autopilot", meta=(ClampMin="0.01"))
    double JoseApproachGainPerSecond = 0.35;
    UPROPERTY(EditAnywhere, Category="Everward|Descent", meta=(ClampMin="0.1"))
    double ControlledDescentMaxDescentSpeedMetersPerSecond = 4.0;
    UPROPERTY(EditAnywhere, Category="Everward|Descent", meta=(ClampMin="0.1"))
    double ControlledDescentMaxTangentialSpeedMetersPerSecond = 2.0;
    UPROPERTY(EditAnywhere, Category="Everward|Descent", meta=(ClampMin="0.0"))
    double ControlledDescentMinimumClearanceMeters = 2.0;
    UPROPERTY(EditAnywhere, Category="Everward|Descent", meta=(ClampMin="0.1"))
    double ControlledDescentFullSpeedAltitudeMeters = 50.0;
    UPROPERTY(EditAnywhere, Category="Everward|Descent", meta=(ClampMin="0.0"))
    double ControlledDescentTouchdownSpeedMetersPerSecond = 0.5;
    UPROPERTY(EditAnywhere, Category="Everward|Descent", meta=(ClampMin="0.1"))
    double ControlledHoverMaxTangentialSpeedMetersPerSecond = 2.0;
    UPROPERTY(EditAnywhere, Category="Everward|Descent", meta=(ClampMin="0.0"))
    double ControlledHoverMinimumClearanceMeters = 2.0;
    UPROPERTY(EditAnywhere, Category="Everward|Descent", meta=(ClampMin="0.1"))
    double ControlledHoverTargetAltitudeMeters = 5.0;
    UPROPERTY(EditAnywhere, Category="Everward|Descent", meta=(ClampMin="0.01"))
    double ControlledHoverAltitudeGainPerSecond = 0.5;
    UPROPERTY(EditAnywhere, Category="Everward|Descent", meta=(ClampMin="0.1"))
    double ControlledHoverMaxVerticalCorrectionSpeedMetersPerSecond = 2.0;
    UPROPERTY(EditAnywhere, Category="Everward|Target", meta=(ClampMin="1.0"))
    double TargetSelectionRangeMeters = 500.0;
    UPROPERTY(EditAnywhere, Category="Everward|Camera", meta=(ClampMin="0.01"))
    float MouseLookSensitivity = 0.75f;
    UPROPERTY(EditAnywhere, Category="Everward|Camera", meta=(ClampMin="1.0"))
    float CameraZoomStepCentimeters = 120.0f;

#if WITH_DEV_AUTOMATION_TESTS
    // Test-only: lets the native HUD/trigger-wiring automation test drive this exact bound input
    // handler (the same one F7 invokes) without a full PIE input pipeline, mirroring
    // UProbeSimulationAdapter::TestOnlySetSubsystemIntegrity()'s own WITH_DEV_AUTOMATION_TESTS-only
    // test hook. Compiled out of shipping builds.
    friend class FEverwardG03RestorationCycleHudTriggerTest;
#endif
};
