#include "EverwardZeroGTestEnvironment.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AEverwardZeroGTestEnvironment::AEverwardZeroGTestEnvironment()
{
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(
        TEXT("/Engine/BasicShapes/Sphere.Sphere"));

    DistantStarLight = CreateDefaultSubobject<UDirectionalLightComponent>(
        TEXT("ZeroGDistantStarLight"));
    DistantStarLight->SetupAttachment(GetRootComponent());
    DistantStarLight->SetRelativeRotation(FRotator(-18.0, 132.0, 0.0));
    DistantStarLight->SetIntensity(6.0f);

    // Large visual reference body only. Collision remains disabled here
    // because local physical-body authority is already provided by the
    // registered simulation bodies inherited from the Phase-2 environment.
    AsteroidReferenceMesh = CreateDefaultSubobject<UStaticMeshComponent>(
        TEXT("ZeroGAsteroidReference"));
    AsteroidReferenceMesh->SetupAttachment(GetRootComponent());
    AsteroidReferenceMesh->SetRelativeLocation(FVector(45000.0, 26000.0, -18000.0));
    AsteroidReferenceMesh->SetRelativeScale3D(FVector(32.0, 27.0, 30.0));
    AsteroidReferenceMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    if (SphereMesh.Succeeded())
    {
        AsteroidReferenceMesh->SetStaticMesh(SphereMesh.Object);
    }

    AsteroidReferenceLabel = CreateDefaultSubobject<UTextRenderComponent>(
        TEXT("ZeroGAsteroidReferenceLabel"));
    AsteroidReferenceLabel->SetupAttachment(GetRootComponent());
    AsteroidReferenceLabel->SetRelativeLocation(FVector(45000.0, 26000.0, -14500.0));
    AsteroidReferenceLabel->SetRelativeRotation(FRotator(0.0, 180.0, 0.0));
    AsteroidReferenceLabel->SetHorizontalAlignment(EHTA_Center);
    AsteroidReferenceLabel->SetWorldSize(180.0f);
    AsteroidReferenceLabel->SetTextRenderColor(FColor(175, 205, 225));
    AsteroidReferenceLabel->SetText(
        FText::FromString(TEXT("ZERO-G REFERENCE BODY // VISUAL NAVIGATION LANDMARK")));
}
