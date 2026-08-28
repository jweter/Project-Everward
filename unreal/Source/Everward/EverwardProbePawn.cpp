#include "EverwardProbePawn.h"

#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputCoreTypes.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "ProbeSimulationAdapter.h"
#include "UObject/ConstructorHelpers.h"

AEverwardProbePawn::AEverwardProbePawn()
{
    PrimaryActorTick.bCanEverTick = true;

    ProbeRoot = CreateDefaultSubobject<USceneComponent>(TEXT("ProbeRoot"));
    SetRootComponent(ProbeRoot);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshAsset(
        TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshAsset(
        TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMeshAsset(
        TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

    auto ConfigureMesh = [](UStaticMeshComponent* Component, USceneComponent* Parent)
    {
        Component->SetupAttachment(Parent);
        Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Component->SetGenerateOverlapEvents(false);
        Component->SetCanEverAffectNavigation(false);
    };

    // Simple Prime A embodiment: one coherent tube, two thermal/radiator wings,
    // a clear aft engine, forward science/sensor head, and real manipulator
    // geometry. This intentionally favors a readable spacecraft silhouette over
    // the previous scatter of disconnected engineering primitives.
    ProbeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PrimeCentralTube"));
    ConfigureMesh(ProbeMesh, ProbeRoot);
    if (CylinderMeshAsset.Succeeded()) ProbeMesh->SetStaticMesh(CylinderMeshAsset.Object);
    ProbeMesh->SetRelativeRotation(FRotator(0.0, 90.0, 0.0));
    ProbeMesh->SetRelativeScale3D(FVector(1.05, 1.05, 10.8));

    CoreHousing = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ComputationCoreSleeve"));
    ConfigureMesh(CoreHousing, ProbeRoot);
    if (CylinderMeshAsset.Succeeded()) CoreHousing->SetStaticMesh(CylinderMeshAsset.Object);
    CoreHousing->SetRelativeLocation(FVector(150.0, 0.0, 0.0));
    CoreHousing->SetRelativeRotation(FRotator(0.0, 90.0, 0.0));
    CoreHousing->SetRelativeScale3D(FVector(1.22, 1.22, 1.55));

    ReactorHousing = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PowerReactorSleeve"));
    ConfigureMesh(ReactorHousing, ProbeRoot);
    if (CylinderMeshAsset.Succeeded()) ReactorHousing->SetStaticMesh(CylinderMeshAsset.Object);
    ReactorHousing->SetRelativeLocation(FVector(-260.0, 0.0, 0.0));
    ReactorHousing->SetRelativeRotation(FRotator(0.0, 90.0, 0.0));
    ReactorHousing->SetRelativeScale3D(FVector(1.18, 1.18, 1.85));

    MainEngine = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MainPropulsionAssembly"));
    ConfigureMesh(MainEngine, ProbeRoot);
    if (CylinderMeshAsset.Succeeded()) MainEngine->SetStaticMesh(CylinderMeshAsset.Object);
    MainEngine->SetRelativeLocation(FVector(-620.0, 0.0, 0.0));
    MainEngine->SetRelativeRotation(FRotator(0.0, 90.0, 0.0));
    MainEngine->SetRelativeScale3D(FVector(1.40, 1.40, 1.55));

    ForwardSensor = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ForwardScienceSensor"));
    ConfigureMesh(ForwardSensor, ProbeRoot);
    if (CylinderMeshAsset.Succeeded()) ForwardSensor->SetStaticMesh(CylinderMeshAsset.Object);
    ForwardSensor->SetRelativeLocation(FVector(620.0, 0.0, 0.0));
    ForwardSensor->SetRelativeRotation(FRotator(0.0, 90.0, 0.0));
    ForwardSensor->SetRelativeScale3D(FVector(0.72, 0.72, 1.35));

    PortRadiator = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PortThermalWing"));
    ConfigureMesh(PortRadiator, ProbeRoot);
    if (CubeMeshAsset.Succeeded()) PortRadiator->SetStaticMesh(CubeMeshAsset.Object);
    PortRadiator->SetRelativeLocation(FVector(-30.0, -195.0, 0.0));
    PortRadiator->SetRelativeScale3D(FVector(3.25, 1.85, 0.065));

    StarboardRadiator = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StarboardThermalWing"));
    ConfigureMesh(StarboardRadiator, ProbeRoot);
    if (CubeMeshAsset.Succeeded()) StarboardRadiator->SetStaticMesh(CubeMeshAsset.Object);
    StarboardRadiator->SetRelativeLocation(FVector(-30.0, 195.0, 0.0));
    StarboardRadiator->SetRelativeScale3D(FVector(3.25, 1.85, 0.065));

    PortManeuverPod = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PortManeuveringPod"));
    ConfigureMesh(PortManeuverPod, ProbeRoot);
    if (SphereMeshAsset.Succeeded()) PortManeuverPod->SetStaticMesh(SphereMeshAsset.Object);
    PortManeuverPod->SetRelativeLocation(FVector(-315.0, -118.0, 32.0));
    PortManeuverPod->SetRelativeScale3D(FVector(0.38, 0.32, 0.32));

    StarboardManeuverPod = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StarboardManeuveringPod"));
    ConfigureMesh(StarboardManeuverPod, ProbeRoot);
    if (SphereMeshAsset.Succeeded()) StarboardManeuverPod->SetStaticMesh(SphereMeshAsset.Object);
    StarboardManeuverPod->SetRelativeLocation(FVector(-315.0, 118.0, 32.0));
    StarboardManeuverPod->SetRelativeScale3D(FVector(0.38, 0.32, 0.32));

    DorsalMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DorsalSensorMast"));
    ConfigureMesh(DorsalMarker, ProbeRoot);
    if (CubeMeshAsset.Succeeded()) DorsalMarker->SetStaticMesh(CubeMeshAsset.Object);
    DorsalMarker->SetRelativeLocation(FVector(260.0, 0.0, 112.0));
    DorsalMarker->SetRelativeScale3D(FVector(0.72, 0.14, 0.42));

    auto ConfigureArm = [&](bool bPort)
    {
        const TCHAR* Prefix = bPort ? TEXT("Port") : TEXT("Starboard");
        const float Side = bPort ? -1.0f : 1.0f;

        USceneComponent*& ShoulderPivot = bPort ? PortShoulderPivot : StarboardShoulderPivot;
        UStaticMeshComponent*& Shoulder = bPort ? PortShoulder : StarboardShoulder;
        UStaticMeshComponent*& UpperArm = bPort ? PortUpperArm : StarboardUpperArm;
        USceneComponent*& ElbowPivot = bPort ? PortElbowPivot : StarboardElbowPivot;
        UStaticMeshComponent*& Forearm = bPort ? PortForearm : StarboardForearm;
        USceneComponent*& WristPivot = bPort ? PortWristPivot : StarboardWristPivot;
        UStaticMeshComponent*& ToolHead = bPort ? PortToolHead : StarboardToolHead;

        ShoulderPivot = CreateDefaultSubobject<USceneComponent>(*FString::Printf(TEXT("%sShoulderPivot"), Prefix));
        ShoulderPivot->SetupAttachment(ProbeRoot);
        ShoulderPivot->SetRelativeLocation(FVector(120.0, Side * 105.0, -92.0));

        Shoulder = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("%sShoulder"), Prefix));
        ConfigureMesh(Shoulder, ShoulderPivot);
        if (CylinderMeshAsset.Succeeded()) Shoulder->SetStaticMesh(CylinderMeshAsset.Object);
        Shoulder->SetRelativeRotation(FRotator(90.0, 0.0, 0.0));
        Shoulder->SetRelativeScale3D(FVector(0.34, 0.34, 0.42));

        UpperArm = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("%sUpperArm"), Prefix));
        ConfigureMesh(UpperArm, ShoulderPivot);
        if (CubeMeshAsset.Succeeded()) UpperArm->SetStaticMesh(CubeMeshAsset.Object);
        UpperArm->SetRelativeLocation(FVector(90.0, 0.0, 0.0));
        UpperArm->SetRelativeScale3D(FVector(1.80, 0.18, 0.18));

        ElbowPivot = CreateDefaultSubobject<USceneComponent>(*FString::Printf(TEXT("%sElbowPivot"), Prefix));
        ElbowPivot->SetupAttachment(ShoulderPivot);
        ElbowPivot->SetRelativeLocation(FVector(180.0, 0.0, 0.0));

        Forearm = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("%sForearm"), Prefix));
        ConfigureMesh(Forearm, ElbowPivot);
        if (CubeMeshAsset.Succeeded()) Forearm->SetStaticMesh(CubeMeshAsset.Object);
        Forearm->SetRelativeLocation(FVector(75.0, 0.0, 0.0));
        Forearm->SetRelativeScale3D(FVector(1.50, 0.16, 0.16));

        WristPivot = CreateDefaultSubobject<USceneComponent>(*FString::Printf(TEXT("%sWristPivot"), Prefix));
        WristPivot->SetupAttachment(ElbowPivot);
        WristPivot->SetRelativeLocation(FVector(150.0, 0.0, 0.0));

        ToolHead = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("%sMiningToolHead"), Prefix));
        ConfigureMesh(ToolHead, WristPivot);
        if (CylinderMeshAsset.Succeeded()) ToolHead->SetStaticMesh(CylinderMeshAsset.Object);
        ToolHead->SetRelativeLocation(FVector(38.0, 0.0, 0.0));
        ToolHead->SetRelativeRotation(FRotator(0.0, 90.0, 0.0));
        ToolHead->SetRelativeScale3D(FVector(0.28, 0.28, 0.70));
    };

    ConfigureArm(true);
    ConfigureArm(false);

    // This remains the existing simulation-owned temporary spherical contact
    // proxy. The user's Product Reality feedback correctly identifies its poor
    // fit around an elongated spacecraft; a compound/capsule authoritative
    // contact shape is tracked as the next physics correction rather than being
    // hidden by cosmetic mesh changes here.
    ProbeCollisionEnvelope = CreateDefaultSubobject<USphereComponent>(TEXT("ProbeCollisionEnvelope"));
    ProbeCollisionEnvelope->SetupAttachment(ProbeRoot);
    ProbeCollisionEnvelope->SetSphereRadius(800.0f);
    ProbeCollisionEnvelope->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    ProbeCollisionEnvelope->SetCollisionResponseToAllChannels(ECR_Overlap);
    ProbeCollisionEnvelope->SetGenerateOverlapEvents(false);

    SimulationAdapter = CreateDefaultSubobject<UProbeSimulationAdapter>(TEXT("ProbeSimulationAdapter"));

    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("ProbeCameraBoom"));
    CameraBoom->SetupAttachment(ProbeRoot);
    CameraBoom->TargetArmLength = 2200.0f;
    CameraBoom->SetRelativeRotation(FRotator(-12.0, 0.0, 0.0));
    CameraBoom->bUsePawnControlRotation = true;
    CameraBoom->bDoCollisionTest = false;

    ProbeCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ProbeCamera"));
    ProbeCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    ProbeCamera->bUsePawnControlRotation = false;
}

void AEverwardProbePawn::BeginPlay()
{
    Super::BeginPlay();
    ApplyPrimeFunctionalMaterials();
    UpdateManipulatorVisuals();
}

void AEverwardProbePawn::ApplyPrimeFunctionalMaterials()
{
    auto ApplyMaterialFamily = [this](
        UStaticMeshComponent* Component,
        const FLinearColor& BaseColor,
        float Metallic,
        float Roughness)
    {
        if (Component == nullptr) return;
        UMaterialInterface* BaseMaterial = Component->GetMaterial(0);
        if (BaseMaterial == nullptr) return;
        UMaterialInstanceDynamic* DynamicMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
        if (DynamicMaterial == nullptr) return;
        DynamicMaterial->SetVectorParameterValue(TEXT("Color"), BaseColor);
        DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), BaseColor);
        DynamicMaterial->SetScalarParameterValue(TEXT("Metallic"), Metallic);
        DynamicMaterial->SetScalarParameterValue(TEXT("Roughness"), Roughness);
        Component->SetMaterial(0, DynamicMaterial);
    };

    const FLinearColor StructuralAlloy(0.34f, 0.37f, 0.40f, 1.0f);
    const FLinearColor ProtectedCore(0.09f, 0.11f, 0.13f, 1.0f);
    const FLinearColor ReactorMetal(0.23f, 0.25f, 0.27f, 1.0f);
    const FLinearColor EngineRefractory(0.18f, 0.105f, 0.065f, 1.0f);
    const FLinearColor OpticalSurface(0.025f, 0.055f, 0.075f, 1.0f);
    const FLinearColor RadiatorSurface(0.055f, 0.065f, 0.075f, 1.0f);
    const FLinearColor ManeuverHardware(0.28f, 0.30f, 0.32f, 1.0f);
    const FLinearColor SensorMast(0.16f, 0.19f, 0.21f, 1.0f);
    const FLinearColor JointHardware(0.12f, 0.13f, 0.14f, 1.0f);
    const FLinearColor ArmStructure(0.27f, 0.29f, 0.31f, 1.0f);
    const FLinearColor ToolMaterial(0.16f, 0.12f, 0.09f, 1.0f);

    ApplyMaterialFamily(ProbeMesh, StructuralAlloy, 0.82f, 0.42f);
    ApplyMaterialFamily(CoreHousing, ProtectedCore, 0.30f, 0.50f);
    ApplyMaterialFamily(ReactorHousing, ReactorMetal, 0.78f, 0.38f);
    ApplyMaterialFamily(MainEngine, EngineRefractory, 0.72f, 0.48f);
    ApplyMaterialFamily(ForwardSensor, OpticalSurface, 0.08f, 0.12f);
    ApplyMaterialFamily(PortRadiator, RadiatorSurface, 0.34f, 0.72f);
    ApplyMaterialFamily(StarboardRadiator, RadiatorSurface, 0.34f, 0.72f);
    ApplyMaterialFamily(PortManeuverPod, ManeuverHardware, 0.76f, 0.44f);
    ApplyMaterialFamily(StarboardManeuverPod, ManeuverHardware, 0.76f, 0.44f);
    ApplyMaterialFamily(DorsalMarker, SensorMast, 0.52f, 0.36f);

    ApplyMaterialFamily(PortShoulder, JointHardware, 0.84f, 0.46f);
    ApplyMaterialFamily(StarboardShoulder, JointHardware, 0.84f, 0.46f);
    ApplyMaterialFamily(PortUpperArm, ArmStructure, 0.80f, 0.43f);
    ApplyMaterialFamily(StarboardUpperArm, ArmStructure, 0.80f, 0.43f);
    ApplyMaterialFamily(PortForearm, ArmStructure, 0.80f, 0.43f);
    ApplyMaterialFamily(StarboardForearm, ArmStructure, 0.80f, 0.43f);
    ApplyMaterialFamily(PortToolHead, ToolMaterial, 0.72f, 0.52f);
    ApplyMaterialFamily(StarboardToolHead, ToolMaterial, 0.72f, 0.52f);
}

void AEverwardProbePawn::UpdateManipulatorVisuals()
{
    if (SimulationAdapter == nullptr) return;

    const TArray<FEverwardManipulatorArmState> States = SimulationAdapter->GetManipulatorArmStates();
    for (const FEverwardManipulatorArmState& State : States)
    {
        const bool bPort = State.ArmId == EEverwardManipulatorArmId::Port;
        const float Side = bPort ? -1.0f : 1.0f;
        USceneComponent* ShoulderPivot = bPort ? PortShoulderPivot : StarboardShoulderPivot;
        USceneComponent* ElbowPivot = bPort ? PortElbowPivot : StarboardElbowPivot;
        USceneComponent* WristPivot = bPort ? PortWristPivot : StarboardWristPivot;
        UStaticMeshComponent* ToolHead = bPort ? PortToolHead : StarboardToolHead;
        if (ShoulderPivot == nullptr || ElbowPivot == nullptr || WristPivot == nullptr) continue;

        const float Deploy = FMath::Clamp(static_cast<float>(State.DeploymentFraction), 0.0f, 1.0f);
        const float FoldPitch = FMath::Lerp(-102.0f, -38.0f, Deploy);
        ShoulderPivot->SetRelativeRotation(FRotator(
            FoldPitch + static_cast<float>(State.ShoulderDegrees),
            0.0f,
            Side * FMath::Lerp(8.0f, 24.0f, Deploy)));
        ElbowPivot->SetRelativeRotation(FRotator(static_cast<float>(State.ElbowDegrees), 0.0f, 0.0f));
        WristPivot->SetRelativeRotation(FRotator(static_cast<float>(State.WristDegrees), 0.0f, 0.0f));

        if (ToolHead != nullptr)
        {
            ToolHead->SetRelativeScale3D(State.bToolAttached
                ? FVector(0.34, 0.34, 0.82)
                : FVector(0.24, 0.24, 0.56));
        }
    }
}

void AEverwardProbePawn::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    AdvanceCameraAlignedRighting(DeltaSeconds);
    UpdateManipulatorVisuals();
}

void AEverwardProbePawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    if (PlayerInputComponent != nullptr)
    {
        PlayerInputComponent->BindKey(
            EKeys::R,
            IE_Pressed,
            this,
            &AEverwardProbePawn::BeginOrCancelCameraAlignedRighting);
    }
}

void AEverwardProbePawn::AdjustCameraZoom(float DeltaCentimeters)
{
    if (CameraBoom == nullptr) return;
    CameraBoom->TargetArmLength = FMath::Clamp(
        CameraBoom->TargetArmLength + DeltaCentimeters,
        MinCameraDistanceCentimeters,
        MaxCameraDistanceCentimeters);
}

void AEverwardProbePawn::BeginOrCancelCameraAlignedRighting()
{
    if (bCameraAlignedRighting)
    {
        bCameraAlignedRighting = false;
        RightingAccumulatorSeconds = 0.0f;
        return;
    }
    if (Controller == nullptr || SimulationAdapter == nullptr) return;

    const FRotator ViewRotation = Controller->GetControlRotation().GetNormalized();
    CameraAlignedRightingTarget = FRotator(ViewRotation.Pitch, ViewRotation.Yaw, 0.0f);
    bCameraAlignedRighting = true;
    RightingAccumulatorSeconds = RightingCommandIntervalSeconds;
}

void AEverwardProbePawn::AdvanceCameraAlignedRighting(float DeltaSeconds)
{
    if (!bCameraAlignedRighting || SimulationAdapter == nullptr) return;

    RightingAccumulatorSeconds += DeltaSeconds;
    if (RightingAccumulatorSeconds < RightingCommandIntervalSeconds) return;
    RightingAccumulatorSeconds = 0.0f;

    const FRotator Current = SimulationAdapter->GetProbeTelemetry().AttitudeDegrees.GetNormalized();
    const float PitchRemaining = FMath::FindDeltaAngleDegrees(Current.Pitch, CameraAlignedRightingTarget.Pitch);
    const float YawRemaining = FMath::FindDeltaAngleDegrees(Current.Yaw, CameraAlignedRightingTarget.Yaw);
    const float RollRemaining = FMath::FindDeltaAngleDegrees(Current.Roll, 0.0f);

    const bool bPitchDone = FMath::Abs(PitchRemaining) <= RightingCompletionToleranceDegrees;
    const bool bYawDone = FMath::Abs(YawRemaining) <= RightingCompletionToleranceDegrees;
    const bool bRollDone = FMath::Abs(RollRemaining) <= RightingCompletionToleranceDegrees;

    if (bPitchDone && bYawDone && bRollDone)
    {
        (void)SimulationAdapter->CommandAdjustAttitudeDegrees(
            FRotator(PitchRemaining, YawRemaining, RollRemaining));
        bCameraAlignedRighting = false;
        return;
    }

    const float MaxStep = RightingDegreesPerSecond * RightingCommandIntervalSeconds;
    const FRotator Step(
        FMath::Clamp(PitchRemaining, -MaxStep, MaxStep),
        FMath::Clamp(YawRemaining, -MaxStep, MaxStep),
        FMath::Clamp(RollRemaining, -MaxStep, MaxStep));

    const FEverwardProbeCommandResult Result = SimulationAdapter->CommandAdjustAttitudeDegrees(Step);
    if (!Result.bAccepted)
    {
        bCameraAlignedRighting = false;
    }
}