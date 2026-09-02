#include "EverwardPhase2TestEnvironment.h"

#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "EverwardProbePawn.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "ProbeSimulationAdapter.h"
#include "UObject/ConstructorHelpers.h"

AEverwardPhase2TestEnvironment::AEverwardPhase2TestEnvironment()
{
    PrimaryActorTick.bCanEverTick = true;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));

    ScanTargetMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BootstrapScanTarget"));
    ScanTargetMesh->SetupAttachment(SceneRoot);
    ScanTargetMesh->SetRelativeLocation(FVector(
        BootstrapBodyCenterXMeters * 100.0,
        BootstrapBodyCenterYMeters * 100.0,
        BootstrapBodyCenterZMeters * 100.0));
    const double BootstrapSphereScale = BootstrapBodyRadiusMeters * 100.0 / 50.0;
    ScanTargetMesh->SetRelativeScale3D(FVector(BootstrapSphereScale));
    ScanTargetMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    ScanTargetMesh->SetCollisionResponseToAllChannels(ECR_Block);
    if (SphereMesh.Succeeded())
    {
        ScanTargetMesh->SetStaticMesh(SphereMesh.Object);
    }

    ScanTargetLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("BootstrapScanTargetLabel"));
    ScanTargetLabel->SetupAttachment(SceneRoot);
    ScanTargetLabel->SetRelativeLocation(FVector(
        BootstrapBodyCenterXMeters * 100.0,
        BootstrapBodyCenterYMeters * 100.0,
        BootstrapBodyCenterZMeters * 100.0 + 700.0));
    ScanTargetLabel->SetRelativeRotation(FRotator(0.0, 180.0, 0.0));
    ScanTargetLabel->SetHorizontalAlignment(EHTA_Center);
    // The former 105 cm label produced small, low-contrast instructions in
    // the current laptop Product Reality capture. Keep target knowledge
    // spatially anchored, but size it for ordinary reading distance.
    ScanTargetLabel->SetWorldSize(170.0f);
    ScanTargetLabel->SetTextRenderColor(FColor(110, 220, 255));
    ScanTargetLabel->SetText(FText::FromString(TEXT(
        "SCAN-001 // UNSURVEYED RESOURCE BODY\nUSE SENSORS TO IDENTIFY MINEABLE MATERIAL")));

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

void AEverwardPhase2TestEnvironment::BeginPlay()
{
    Super::BeginPlay();
    ApplyEnvironmentMaterialScaffold();
    RefreshResourceReadout();
}

void AEverwardPhase2TestEnvironment::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    RefreshResourceReadout();
    RefreshTargetSelectionHighlight();
    RefreshScanTargetPosition();
}

const UProbeSimulationAdapter* AEverwardPhase2TestEnvironment::ResolvePlayerAdapter() const
{
    if (GetWorld() == nullptr)
    {
        return nullptr;
    }

    const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    const AEverwardProbePawn* Probe = Cast<AEverwardProbePawn>(PlayerPawn);
    return Probe != nullptr ? Probe->GetSimulationAdapter() : nullptr;
}

void AEverwardPhase2TestEnvironment::RefreshResourceReadout()
{
    if (ScanTargetLabel == nullptr)
    {
        return;
    }

    const UProbeSimulationAdapter* Adapter = ResolvePlayerAdapter();
    if (Adapter == nullptr)
    {
        return;
    }

    const FEverwardMiningStatus Mining = Adapter->GetMiningStatus();
    if (!Mining.bSurveyed)
    {
        ScanTargetLabel->SetTextRenderColor(FColor(110, 220, 255));
        ScanTargetLabel->SetText(FText::FromString(TEXT(
            "SCAN-001 // UNSURVEYED RESOURCE BODY\nUSE SENSORS TO IDENTIFY MINEABLE MATERIAL")));
        return;
    }

    if (Mining.DepositRemainingKilograms <= 0.0)
    {
        ScanTargetLabel->SetTextRenderColor(FColor(150, 165, 175));
        ScanTargetLabel->SetText(FText::FromString(FString::Printf(
            TEXT("SCAN-001 // %s\nDEPOSIT EXHAUSTED // %.1f KG RECOVERED"),
            *Mining.MaterialName,
            Mining.ExtractedMaterialKilograms)));
        return;
    }

    ScanTargetLabel->SetTextRenderColor(FColor(120, 255, 165));
    ScanTargetLabel->SetText(FText::FromString(FString::Printf(
        TEXT("SCAN COMPLETE // %s\n%.1f KG REMAINING // APPROACH + ARM/TOOL // [G] MINE %.1f KG"),
        *Mining.MaterialName,
        Mining.DepositRemainingKilograms,
        Mining.ExtractionKilogramsPerCycle)));
}

void AEverwardPhase2TestEnvironment::ApplyEnvironmentMaterialScaffold()
{
    auto ApplyMaterial = [this](
        UStaticMeshComponent* Component,
        const FLinearColor& BaseColor,
        float Metallic,
        float Roughness) -> UMaterialInstanceDynamic*
    {
        if (Component == nullptr) return nullptr;
        UMaterialInterface* BaseMaterial = Component->GetMaterial(0);
        if (BaseMaterial == nullptr) return nullptr;

        UMaterialInstanceDynamic* DynamicMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
        if (DynamicMaterial == nullptr) return nullptr;

        DynamicMaterial->SetVectorParameterValue(TEXT("Color"), BaseColor);
        DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), BaseColor);
        DynamicMaterial->SetScalarParameterValue(TEXT("Metallic"), Metallic);
        DynamicMaterial->SetScalarParameterValue(TEXT("Roughness"), Roughness);
        Component->SetMaterial(0, DynamicMaterial);
        return DynamicMaterial;
    };

    const FLinearColor RegolithRock(0.20f, 0.18f, 0.16f, 1.0f);
    const FLinearColor NavigationMarker(0.075f, 0.095f, 0.11f, 1.0f);

    // Kept for RefreshTargetSelectionHighlight to retint in place rather than
    // creating a second material instance for the same component.
    ScanTargetDynamicMaterial = ApplyMaterial(ScanTargetMesh, RegolithRock, 0.04f, 0.88f);
    for (UStaticMeshComponent* Marker : ReferenceMarkers)
    {
        ApplyMaterial(Marker, NavigationMarker, 0.18f, 0.62f);
    }
}

void AEverwardPhase2TestEnvironment::RefreshScanTargetPosition()
{
    if (ScanTargetMesh == nullptr)
    {
        return;
    }

    const UProbeSimulationAdapter* Adapter = ResolvePlayerAdapter();
    if (Adapter == nullptr)
    {
        return;
    }

    // Fails closed (leaves the mesh where it already is) if the body was
    // deregistered -- matches Core->static_bodies()' own fail-closed
    // contract; never fabricates a position.
    FVector PositionMeters;
    if (!Adapter->GetStaticBodyPositionMeters(BootstrapScanTargetId, PositionMeters))
    {
        return;
    }

    const FVector PositionCentimeters = PositionMeters * 100.0;
    ScanTargetMesh->SetRelativeLocation(PositionCentimeters);
    if (ScanTargetLabel != nullptr)
    {
        // Preserve the label's original fixed offset above the target rather
        // than recomputing a new one, so it keeps tracking the target the
        // same way it did while stationary.
        ScanTargetLabel->SetRelativeLocation(PositionCentimeters + FVector(0.0, 0.0, 700.0));
    }
}

void AEverwardPhase2TestEnvironment::RefreshTargetSelectionHighlight()
{
    if (ScanTargetDynamicMaterial == nullptr)
    {
        return;
    }

    const UProbeSimulationAdapter* Adapter = ResolvePlayerAdapter();
    if (Adapter == nullptr)
    {
        return;
    }

    const FEverwardTargetSelectionStatus TargetSelection = Adapter->GetSelectedTargetStatus();
    const bool bIsSelected = TargetSelection.bHasSelection
        && TargetSelection.TargetId == BootstrapScanTargetId;

    // Adapter->GetSelectedTargetStatus() is already recomputed authoritative
    // state every call; only touch the material when the selection outcome
    // actually changes instead of writing identical parameters every tick.
    if (bIsSelected == bScanTargetHighlightActive)
    {
        return;
    }
    bScanTargetHighlightActive = bIsSelected;

    const FLinearColor RegolithRock(0.20f, 0.18f, 0.16f, 1.0f);
    const FLinearColor SelectedHighlight(0.15f, 0.95f, 1.0f, 1.0f);
    const FLinearColor& TintColor = bIsSelected ? SelectedHighlight : RegolithRock;

    ScanTargetDynamicMaterial->SetVectorParameterValue(TEXT("Color"), TintColor);
    ScanTargetDynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), TintColor);
    ScanTargetDynamicMaterial->SetScalarParameterValue(TEXT("Metallic"), bIsSelected ? 0.35f : 0.04f);
    ScanTargetDynamicMaterial->SetScalarParameterValue(TEXT("Roughness"), bIsSelected ? 0.30f : 0.88f);
}
