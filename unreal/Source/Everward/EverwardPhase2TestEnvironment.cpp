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

    // Slice 8 (partial): additional registered physical targets at different
    // ranges so target cycling's nearest-to-farthest wraparound has more
    // than one eligible body to actually cycle through and highlight. Each
    // follows the exact same mesh/label/collision construction as the
    // bootstrap scan target above, minus the mining-specific label text.
    struct FReferenceTargetSpawnInfo
    {
        FString Id;
        FVector CenterMeters;
        double RadiusMeters;
    };
    const FReferenceTargetSpawnInfo ReferenceTargetSpawns[] = {
        {
            ReferenceTarget1Id,
            FVector(ReferenceTarget1CenterXMeters, ReferenceTarget1CenterYMeters, ReferenceTarget1CenterZMeters),
            ReferenceTarget1RadiusMeters,
        },
        {
            ReferenceTarget2Id,
            FVector(ReferenceTarget2CenterXMeters, ReferenceTarget2CenterYMeters, ReferenceTarget2CenterZMeters),
            ReferenceTarget2RadiusMeters,
        },
    };

    for (int32 Index = 0; Index < UE_ARRAY_COUNT(ReferenceTargetSpawns); ++Index)
    {
        const FReferenceTargetSpawnInfo& Spawn = ReferenceTargetSpawns[Index];
        const FVector CenterCentimeters = Spawn.CenterMeters * 100.0;

        const FName MeshName(*FString::Printf(TEXT("ReferenceTarget_%02d"), Index + 1));
        UStaticMeshComponent* TargetMesh = CreateDefaultSubobject<UStaticMeshComponent>(MeshName);
        TargetMesh->SetupAttachment(SceneRoot);
        TargetMesh->SetRelativeLocation(CenterCentimeters);
        TargetMesh->SetRelativeScale3D(FVector(Spawn.RadiusMeters * 100.0 / 50.0));
        TargetMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        TargetMesh->SetCollisionResponseToAllChannels(ECR_Block);
        if (SphereMesh.Succeeded())
        {
            TargetMesh->SetStaticMesh(SphereMesh.Object);
        }
        ReferenceTargetMeshes.Add(TargetMesh);

        const FName LabelName(*FString::Printf(TEXT("ReferenceTargetLabel_%02d"), Index + 1));
        UTextRenderComponent* TargetLabel = CreateDefaultSubobject<UTextRenderComponent>(LabelName);
        TargetLabel->SetupAttachment(SceneRoot);
        TargetLabel->SetRelativeLocation(CenterCentimeters + FVector(0.0, 0.0, Spawn.RadiusMeters * 100.0 + 500.0));
        TargetLabel->SetRelativeRotation(FRotator(0.0, 180.0, 0.0));
        TargetLabel->SetHorizontalAlignment(EHTA_Center);
        TargetLabel->SetWorldSize(150.0f);
        TargetLabel->SetTextRenderColor(FColor(110, 220, 255));
        TargetLabel->SetText(FText::FromString(FString::Printf(
            TEXT("REF-%03d // REGISTERED PHYSICAL BODY"), Index + 2)));
        ReferenceTargetLabels.Add(TargetLabel);

        ReferenceTargetIds.Add(Spawn.Id);
        ReferenceTargetDynamicMaterials.Add(nullptr);
        ReferenceTargetHighlightActive.Add(false);
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
    RefreshReferenceTargets();
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

    // Same regolith-rock treatment as the bootstrap scan target, and the
    // same "keep the dynamic material for later retinting" reasoning --
    // RefreshReferenceTargets() below retints these in place exactly the
    // way RefreshTargetSelectionHighlight() already does for ScanTargetMesh.
    for (int32 Index = 0; Index < ReferenceTargetMeshes.Num(); ++Index)
    {
        ReferenceTargetDynamicMaterials[Index] =
            ApplyMaterial(ReferenceTargetMeshes[Index], RegolithRock, 0.04f, 0.88f);
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

void AEverwardPhase2TestEnvironment::RefreshReferenceTargets()
{
    const UProbeSimulationAdapter* Adapter = ResolvePlayerAdapter();
    if (Adapter == nullptr)
    {
        return;
    }

    // Read once per tick and reuse for every reference target below, rather
    // than one authoritative-status query per target -- GetSelectedTargetStatus()
    // is already recomputed live on every call, exactly as
    // RefreshTargetSelectionHighlight() relies on for the bootstrap target.
    const FEverwardTargetSelectionStatus TargetSelection = Adapter->GetSelectedTargetStatus();
    const FLinearColor RegolithRock(0.20f, 0.18f, 0.16f, 1.0f);
    const FLinearColor SelectedHighlight(0.15f, 0.95f, 1.0f, 1.0f);

    for (int32 Index = 0; Index < ReferenceTargetIds.Num(); ++Index)
    {
        // Position mirroring: identical fail-closed contract to
        // RefreshScanTargetPosition() -- a deregistered body simply leaves
        // the mesh where it already is rather than fabricating a position.
        // This keeps a reference target visually correct even if it is ever
        // grasped and moved by a manipulator arm, the same as the bootstrap
        // target already is.
        UStaticMeshComponent* TargetMesh = ReferenceTargetMeshes.IsValidIndex(Index)
            ? ReferenceTargetMeshes[Index].Get() : nullptr;
        if (TargetMesh != nullptr)
        {
            FVector PositionMeters;
            if (Adapter->GetStaticBodyPositionMeters(ReferenceTargetIds[Index], PositionMeters))
            {
                const FVector PositionCentimeters = PositionMeters * 100.0;
                TargetMesh->SetRelativeLocation(PositionCentimeters);
                if (ReferenceTargetLabels.IsValidIndex(Index) && ReferenceTargetLabels[Index] != nullptr)
                {
                    ReferenceTargetLabels[Index]->SetRelativeLocation(PositionCentimeters + FVector(0.0, 0.0, 500.0));
                }
            }
        }

        // Selection highlight: identical only-touch-on-change contract to
        // RefreshTargetSelectionHighlight().
        UMaterialInstanceDynamic* DynamicMaterial = ReferenceTargetDynamicMaterials.IsValidIndex(Index)
            ? ReferenceTargetDynamicMaterials[Index].Get() : nullptr;
        if (DynamicMaterial == nullptr)
        {
            continue;
        }

        const bool bIsSelected = TargetSelection.bHasSelection
            && TargetSelection.TargetId == ReferenceTargetIds[Index];
        if (bIsSelected == ReferenceTargetHighlightActive[Index])
        {
            continue;
        }
        ReferenceTargetHighlightActive[Index] = bIsSelected;

        const FLinearColor& TintColor = bIsSelected ? SelectedHighlight : RegolithRock;
        DynamicMaterial->SetVectorParameterValue(TEXT("Color"), TintColor);
        DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), TintColor);
        DynamicMaterial->SetScalarParameterValue(TEXT("Metallic"), bIsSelected ? 0.35f : 0.04f);
        DynamicMaterial->SetScalarParameterValue(TEXT("Roughness"), bIsSelected ? 0.30f : 0.88f);
    }
}
