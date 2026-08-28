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

private:
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

    // Slice 6 foundation input: deploy/stow toggle per arm and a single tool
    // attach/detach toggle.
    void TogglePortManipulatorArm();
    void ToggleStarboardManipulatorArm();
    void ToggleManipulatorTool();

    // Slice 6 joint-articulation follow-up: a dedicated manipulator HUD page
    // plus explicit arm/joint selection, since two arms can each be
    // mid-pose independently and PageUp/PageDown are already the systems
    // panel's power-adjustment keys.
    void ToggleManipulatorPanel();
    void CycleManipulatorArmSelection();
    void SelectManipulatorJointShoulder();
    void SelectManipulatorJointElbow();
    void SelectManipulatorJointWrist();
    void IncreaseManipulatorJointTarget();
    void DecreaseManipulatorJointTarget();

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

    UPROPERTY(EditAnywhere, Category="Everward|Camera", meta=(ClampMin="0.01"))
    float MouseLookSensitivity = 0.75f;

    UPROPERTY(EditAnywhere, Category="Everward|Camera", meta=(ClampMin="1.0"))
    float CameraZoomStepCentimeters = 120.0f;
};
