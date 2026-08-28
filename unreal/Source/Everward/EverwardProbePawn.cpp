#include "EverwardProbePawn.h"

#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputCoreTypes.h"
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

    auto ConfigureVisualPiece = [this](UStaticMeshComponent* Component)
    {
        Component->SetupAttachment(ProbeRoot);
        Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Component->SetGenerateOverlapEvents(false);
        Component->SetCanEverAffectNavigation(false);
    };

    // Prime Generation-1 blockout: approximately 15 m long, with a clear
    // fore/aft propulsion axis and visually distinct system modules. These are
    // deliberately simple primitives so silhouette, scale, camera framing and
    // component layout can be Product-Reality-tested before production art.
    ProbeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StructuralSpine"));
    ConfigureVisualPiece(ProbeMesh);
    if (CubeMeshAsset.Succeeded())
    {
        ProbeMesh->SetStaticMesh(CubeMeshAsset.Object);
    }
    ProbeMesh->SetRelativeScale3D(FVector(8.2, 0.42, 0.34));

    CoreHousing = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ComputationCoreHousing"));
    ConfigureVisualPiece(CoreHousing);
    if (SphereMeshAsset.Succeeded())
    {
        CoreHousing->SetStaticMesh(SphereMeshAsset.Object);
    }
    CoreHousing->SetRelativeLocation(FVector(120.0, 0.0, 10.0));
    CoreHousing->SetRelativeScale3D(FVector(1.30, 1.05, 0.90));

    ReactorHousing = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PowerReactorHousing"));
    ConfigureVisualPiece(ReactorHousing);
    if (SphereMeshAsset.Succeeded())
    {
        ReactorHousing->SetStaticMesh(SphereMeshAsset.Object);
    }
    ReactorHousing->SetRelativeLocation(FVector(-250.0, 0.0, -5.0));
    ReactorHousing->SetRelativeScale3D(FVector(1.18, 1.00, 0.95));

    MainEngine = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MainPropulsionAssembly"));
    ConfigureVisualPiece(MainEngine);
    if (CylinderMeshAsset.Succeeded())
    {
        MainEngine->SetStaticMesh(CylinderMeshAsset.Object);
    }
    MainEngine->SetRelativeLocation(FVector(-640.0, 0.0, 0.0));
    MainEngine->SetRelativeRotation(FRotator(0.0, 90.0, 0.0));
    MainEngine->SetRelativeScale3D(FVector(1.45, 1.45, 2.20));

    ForwardSensor = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ForwardSensor"));
    ConfigureVisualPiece(ForwardSensor);
    if (CylinderMeshAsset.Succeeded())
    {
        ForwardSensor->SetStaticMesh(CylinderMeshAsset.Object);
    }
    ForwardSensor->SetRelativeLocation(FVector(610.0, 0.0, 22.0));
    ForwardSensor->SetRelativeRotation(FRotator(0.0, 90.0, 0.0));
    ForwardSensor->SetRelativeScale3D(FVector(0.72, 0.72, 1.55));

    PortRadiator = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PortThermalRadiator"));
    ConfigureVisualPiece(PortRadiator);
    if (CubeMeshAsset.Succeeded())
    {
        PortRadiator->SetStaticMesh(CubeMeshAsset.Object);
    }
    PortRadiator->SetRelativeLocation(FVector(-40.0, -270.0, 8.0));
    PortRadiator->SetRelativeScale3D(FVector(3.70, 2.00, 0.07));

    StarboardRadiator = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StarboardThermalRadiator"));
    ConfigureVisualPiece(StarboardRadiator);
    if (CubeMeshAsset.Succeeded())
    {
        StarboardRadiator->SetStaticMesh(CubeMeshAsset.Object);
    }
    StarboardRadiator->SetRelativeLocation(FVector(-40.0, 270.0, 8.0));
    StarboardRadiator->SetRelativeScale3D(FVector(3.70, 2.00, 0.07));

    PortManeuverPod = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PortManeuveringPod"));
    ConfigureVisualPiece(PortManeuverPod);
    if (SphereMeshAsset.Succeeded())
    {
        PortManeuverPod->SetStaticMesh(SphereMeshAsset.Object);
    }
    PortManeuverPod->SetRelativeLocation(FVector(-360.0, -155.0, 42.0));
    PortManeuverPod->SetRelativeScale3D(FVector(0.58, 0.42, 0.42));

    StarboardManeuverPod = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StarboardManeuveringPod"));
    ConfigureVisualPiece(StarboardManeuverPod);
    if (SphereMeshAsset.Succeeded())
    {
        StarboardManeuverPod->SetStaticMesh(SphereMeshAsset.Object);
    }
    StarboardManeuverPod->SetRelativeLocation(FVector(-360.0, 155.0, 42.0));
    StarboardManeuverPod->SetRelativeScale3D(FVector(0.58, 0.42, 0.42));

    DorsalMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DorsalSensorMast"));
    ConfigureVisualPiece(DorsalMarker);
    if (CubeMeshAsset.Succeeded())
    {
        DorsalMarker->SetStaticMesh(CubeMeshAsset.Object);
    }
    DorsalMarker->SetRelativeLocation(FVector(160.0, 0.0, 125.0));
    DorsalMarker->SetRelativeScale3D(FVector(1.10, 0.16, 0.60));

    PortShoulder = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PortShoulder"));
    ConfigureVisualPiece(PortShoulder);
    if (CylinderMeshAsset.Succeeded())
    {
        PortShoulder->SetStaticMesh(CylinderMeshAsset.Object);
    }
    PortShoulder->SetRelativeLocation(FVector(70.0, -215.0, -45.0));
    PortShoulder->SetRelativeRotation(FRotator(90.0, 0.0, 0.0));
    PortShoulder->SetRelativeScale3D(FVector(0.52, 0.52, 0.60));

    StarboardShoulder = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StarboardShoulder"));
    ConfigureVisualPiece(StarboardShoulder);
    if (CylinderMeshAsset.Succeeded())
    {
        StarboardShoulder->SetStaticMesh(CylinderMeshAsset.Object);
    }
    StarboardShoulder->SetRelativeLocation(FVector(70.0, 215.0, -45.0));
    StarboardShoulder->SetRelativeRotation(FRotator(90.0, 0.0, 0.0));
    StarboardShoulder->SetRelativeScale3D(FVector(0.52, 0.52, 0.60));

    // 8 m is a conservative body-corresponding bounding sphere for the first
    // ~15 m Prime blockout. The engine-neutral runtime still performs swept
    // contact/resolution; this Unreal component only mirrors that envelope.
    ProbeCollisionEnvelope = CreateDefaultSubobject<USphereComponent>(TEXT("ProbeCollisionEnvelope"));
    ProbeCollisionEnvelope->SetupAttachment(ProbeRoot);
    ProbeCollisionEnvelope->SetSphereRadius(800.0f);
    ProbeCollisionEnvelope->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    ProbeCollisionEnvelope->SetCollisionResponseToAllChannels(ECR_Overlap);
    ProbeCollisionEnvelope->SetGenerateOverlapEvents(false);

    SimulationAdapter = CreateDefaultSubobject<UProbeSimulationAdapter>(TEXT("ProbeSimulationAdapter"));

    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("ProbeCameraBoom"));
    CameraBoom->SetupAttachment(ProbeRoot);
    CameraBoom->TargetArmLength = 2600.0f;
    CameraBoom->SetRelativeRotation(FRotator(-12.0, 0.0, 0.0));
    CameraBoom->bUsePawnControlRotation = true;
    CameraBoom->bDoCollisionTest = false;

    ProbeCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ProbeCamera"));
    ProbeCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    ProbeCamera->bUsePawnControlRotation = false;
}

void AEverwardProbePawn::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    AdvanceCameraAlignedRighting(DeltaSeconds);
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
