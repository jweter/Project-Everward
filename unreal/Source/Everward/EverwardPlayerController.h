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

    void SelectNearestPhysicalTarget();
    void ToggleManipulatorGrasp();

    void SaveGame();
    void LoadGame();

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

    bool bJoseAutopilotEngaged = false;
    FString JoseDestinationTargetId;

    bool bControlledDescentEngaged = false;

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
    UPROPERTY(EditAnywhere, Category="Everward|Mining", meta=(ClampMin="0.1"))
    double AutoApproachSideStandoffMeters = 6.5;
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
    UPROPERTY(EditAnywhere, Category="Everward|Target", meta=(ClampMin="1.0"))
    double TargetSelectionRangeMeters = 500.0;
    UPROPERTY(EditAnywhere, Category="Everward|Camera", meta=(ClampMin="0.01"))
    float MouseLookSensitivity = 0.75f;
    UPROPERTY(EditAnywhere, Category="Everward|Camera", meta=(ClampMin="1.0"))
    float CameraZoomStepCentimeters = 120.0f;
};
