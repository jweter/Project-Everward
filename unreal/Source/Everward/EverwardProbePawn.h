#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "EverwardProbePawn.generated.h"

class UCameraComponent;
class UInputComponent;
class UProbeSimulationAdapter;
class USceneComponent;
class USphereComponent;
class USpringArmComponent;
class UStaticMeshComponent;
class UStaticMesh;
enum class EEverwardManipulatorArmId : uint8;
enum class EEverwardManipulatorJoint : uint8;

UCLASS()
class EVERWARD_API AEverwardProbePawn : public APawn
{
    GENERATED_BODY()

public:
    AEverwardProbePawn();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    UFUNCTION(BlueprintPure, Category="Everward|Probe")
    UProbeSimulationAdapter* GetSimulationAdapter() const { return SimulationAdapter; }

    UFUNCTION(BlueprintCallable, Category="Everward|Camera")
    void AdjustCameraZoom(float DeltaCentimeters);

    // Presentation-only selection feedback. Simulation still owns joint
    // motion; this only makes the arm segment about to move visually obvious.
    void SetManipulatorSelectionHighlight(
        bool bEnabled,
        EEverwardManipulatorArmId ArmId,
        EEverwardManipulatorJoint Joint);

private:
    // Optional unit-normalized GLB -> Unreal static mesh replacements. Existing
    // component pivots, centimetre transforms, collisions, commands and
    // simulation telemetry remain unchanged. Missing assets use the original
    // BasicShapes fallback, so no upstream tools ship with the game.
    void ApplyProceduralArmMeshesIfAssigned();
    void ApplyPrimeFunctionalMaterials();
    void UpdateManipulatorVisuals();
    void BeginOrCancelCameraAlignedRighting();
    void AdvanceCameraAlignedRighting(float DeltaSeconds);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Everward|Probe", meta=(AllowPrivateAccess="true"))
    TObjectPtr<USceneComponent> ProbeRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Everward|Probe", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UStaticMeshComponent> ProbeMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Everward|Physics", meta=(AllowPrivateAccess="true"))
    TObjectPtr<USphereComponent> ProbeCollisionEnvelope;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Everward|Probe|Systems", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UStaticMeshComponent> CoreHousing;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Everward|Probe|Systems", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UStaticMeshComponent> ReactorHousing;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Everward|Probe|Systems", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UStaticMeshComponent> MainEngine;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Everward|Probe|Systems", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UStaticMeshComponent> ForwardSensor;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Everward|Probe|Systems", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UStaticMeshComponent> PortRadiator;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Everward|Probe|Systems", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UStaticMeshComponent> StarboardRadiator;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Everward|Probe|Systems", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UStaticMeshComponent> PortManeuverPod;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Everward|Probe|Systems", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UStaticMeshComponent> StarboardManeuverPod;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Everward|Probe|Systems", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UStaticMeshComponent> DorsalMarker;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Everward|Probe|Manipulators", meta=(AllowPrivateAccess="true"))
    TObjectPtr<USceneComponent> PortShoulderPivot;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Everward|Probe|Manipulators", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UStaticMeshComponent> PortShoulder;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Everward|Probe|Manipulators", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UStaticMeshComponent> PortUpperArm;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Everward|Probe|Manipulators", meta=(AllowPrivateAccess="true"))
    TObjectPtr<USceneComponent> PortElbowPivot;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Everward|Probe|Manipulators", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UStaticMeshComponent> PortForearm;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Everward|Probe|Manipulators", meta=(AllowPrivateAccess="true"))
    TObjectPtr<USceneComponent> PortWristPivot;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Everward|Probe|Manipulators", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UStaticMeshComponent> PortToolHead;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Everward|Probe|Manipulators", meta=(AllowPrivateAccess="true"))
    TObjectPtr<USceneComponent> StarboardShoulderPivot;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Everward|Probe|Manipulators", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UStaticMeshComponent> StarboardShoulder;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Everward|Probe|Manipulators", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UStaticMeshComponent> StarboardUpperArm;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Everward|Probe|Manipulators", meta=(AllowPrivateAccess="true"))
    TObjectPtr<USceneComponent> StarboardElbowPivot;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Everward|Probe|Manipulators", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UStaticMeshComponent> StarboardForearm;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Everward|Probe|Manipulators", meta=(AllowPrivateAccess="true"))
    TObjectPtr<USceneComponent> StarboardWristPivot;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Everward|Probe|Manipulators", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UStaticMeshComponent> StarboardToolHead;

    // Designed for normalized zero-origin 1m mesh modules from
    // tools/procedural_mechanics/prime-arm/export_glb.mjs. Set these after
    // importing and validating the GLB in Unreal Editor. Nothing is loaded
    // or changed in a default build; these do not create gameplay actors.
    UPROPERTY(EditAnywhere, Category="Everward|Probe|ProceduralArm")
    TSoftObjectPtr<UStaticMesh> ProceduralUpperArmMesh;
    UPROPERTY(EditAnywhere, Category="Everward|Probe|ProceduralArm")
    TSoftObjectPtr<UStaticMesh> ProceduralForearmMesh;
    UPROPERTY(EditAnywhere, Category="Everward|Probe|ProceduralArm")
    TSoftObjectPtr<UStaticMesh> ProceduralMiningToolMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Everward|Probe", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UProbeSimulationAdapter> SimulationAdapter;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Everward|Camera", meta=(AllowPrivateAccess="true"))
    TObjectPtr<USpringArmComponent> CameraBoom;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Everward|Camera", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UCameraComponent> ProbeCamera;

    UPROPERTY(EditAnywhere, Category="Everward|Camera", meta=(ClampMin="500.0"))
    float MinCameraDistanceCentimeters = 1200.0f;
    UPROPERTY(EditAnywhere, Category="Everward|Camera", meta=(ClampMin="500.0"))
    float MaxCameraDistanceCentimeters = 5000.0f;
    UPROPERTY(EditAnywhere, Category="Everward|Phase2", meta=(ClampMin="1.0"))
    float RightingDegreesPerSecond = 36.0f;
    UPROPERTY(EditAnywhere, Category="Everward|Phase2", meta=(ClampMin="0.02", ClampMax="0.5"))
    float RightingCommandIntervalSeconds = 0.10f;
    UPROPERTY(EditAnywhere, Category="Everward|Phase2", meta=(ClampMin="0.05"))
    float RightingCompletionToleranceDegrees = 0.5f;

    bool bCameraAlignedRighting = false;
    float RightingAccumulatorSeconds = 0.0f;
    FRotator CameraAlignedRightingTarget = FRotator::ZeroRotator;
};