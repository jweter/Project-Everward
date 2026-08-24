#include "EverwardPhase2TestEnvironment.h"

#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AEverwardPhase2TestEnvironment::AEverwardPhase2TestEnvironment()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(
        TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
        TEXT("/Engine/BasicShapes/Cube.Cube"));

    ScanTargetMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BootstrapScanTarget"));
    ScanTargetMesh->SetupAttachment(SceneRoot);
    ScanTargetMesh->SetRelativeLocation(FVector(5000.0, 0.0, 0.0));
    ScanTargetMesh->SetRelativeScale3D(FVector(4.0, 4.0, 4.0));
    ScanTargetMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    if (SphereMesh.Succeeded())
    {
        ScanTargetMesh->SetStaticMesh(SphereMesh.Object);
    }

    ScanTargetLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("BootstrapScanTargetLabel"));
    ScanTargetLabel->SetupAttachment(SceneRoot);
    ScanTargetLabel->SetRelativeLocation(FVector(5000.0, 0.0, 500.0));
    ScanTargetLabel->SetRelativeRotation(FRotator(0.0, 180.0, 0.0));
    ScanTargetLabel->SetHorizontalAlignment(EHTA_Center);
    ScanTargetLabel->SetWorldSize(120.0f);
    ScanTargetLabel->SetTextRenderColor(FColor(110, 220, 255));
    ScanTargetLabel->SetText(FText::FromString(TEXT("PHASE-2 TARGET // SCAN-001")));

    KeyLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("Phase2KeyLight"));
    KeyLight->SetupAttachment(SceneRoot);
    KeyLight->SetRelativeLocation(FVector(2500.0, -2200.0, 2400.0));
    KeyLight->SetIntensity(40000.0f);
    KeyLight->SetAttenuationRadius(14000.0f);

    const FVector MarkerLocations[] = {
        FVector(1000.0, 1200.0, 0.0),
        FVector(1800.0, -1500.0, 700.0),
        FVector(2800.0, 1800.0, -900.0),
        FVector(3800.0, -2200.0, 1200.0),
        FVector(6200.0, 1500.0, 500.0),
        FVector(7200.0, -1800.0, -700.0)
    };

    for (int32 Index = 0; Index < UE_ARRAY_COUNT(MarkerLocations); ++Index)
    {
        const FName ComponentName(*FString::Printf(TEXT("SpatialReference_%02d"), Index + 1));
        UStaticMeshComponent* Marker = CreateDefaultSubobject<UStaticMeshComponent>(ComponentName);
        Marker->SetupAttachment(SceneRoot);
        Marker->SetRelativeLocation(MarkerLocations[Index]);
        Marker->SetRelativeScale3D(FVector(0.45, 0.45, 0.45));
        Marker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        if (CubeMesh.Succeeded())
        {
            Marker->SetStaticMesh(CubeMesh.Object);
        }
        ReferenceMarkers.Add(Marker);
    }
}
