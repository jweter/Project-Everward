#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EverwardPhase2TestEnvironment.generated.h"

class UMaterialInstanceDynamic;
class UPointLightComponent;
class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class UProbeSimulationAdapter;

// Temporary Phase-2 integration environment. The visible target is both the
// first reproducible physical body and the first scan-to-mining resource body.
UCLASS()
class EVERWARD_API AEverwardPhase2TestEnvironment : public AActor
{
    GENERATED_BODY()

public:
    AEverwardPhase2TestEnvironment();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    static constexpr const TCHAR* BootstrapScanTargetId = TEXT("phase2-test-target-001");
    static constexpr double BootstrapBodyCenterXMeters = 50.0;
    static constexpr double BootstrapBodyCenterYMeters = 0.0;
    static constexpr double BootstrapBodyCenterZMeters = 0.0;
    static constexpr double BootstrapBodyRadiusMeters = 2.0;

    // Slice 8 (partial): a second and third registered physical body at
    // different ranges from the bootstrap scan target. These are plain
    // reference bodies, not mineable resources -- they exist so `T`'s
    // nearest-to-farthest cycling (#149/#150) and the visual selection
    // indicator (#151) can actually be exercised and seen with more than one
    // eligible target, which PHASE2_TARGET_CYCLING_TEST.md's local
    // acceptance section previously called out as impossible with only one
    // registered body. No new selection/cycling/highlight mechanic is
    // introduced; the existing ones are simply given more registered bodies
    // to operate on.
    static constexpr const TCHAR* ReferenceTarget1Id = TEXT("phase2-test-target-002");
    static constexpr double ReferenceTarget1CenterXMeters = 95.0;
    static constexpr double ReferenceTarget1CenterYMeters = 40.0;
    static constexpr double ReferenceTarget1CenterZMeters = 0.0;
    static constexpr double ReferenceTarget1RadiusMeters = 3.0;

    static constexpr const TCHAR* ReferenceTarget2Id = TEXT("phase2-test-target-003");
    static constexpr double ReferenceTarget2CenterXMeters = 160.0;
    static constexpr double ReferenceTarget2CenterYMeters = -60.0;
    static constexpr double ReferenceTarget2CenterZMeters = 15.0;
    static constexpr double ReferenceTarget2RadiusMeters = 4.0;

private:
    void ApplyEnvironmentMaterialScaffold();
    void RefreshResourceReadout();
    // Slice 7: purely reversible presentation over the already-authoritative
    // FEverwardTargetSelectionStatus (parallel-safe lane) -- tints the
    // registered physical body's own mesh when it is the selected target
    // instead of inventing a second, Unreal-owned notion of selection.
    void RefreshTargetSelectionHighlight();
    // Slice 7 "move": the registered physical body's own authoritative
    // center_m is now the single source of truth for where it currently is
    // -- TickComponent already writes a grasped body's wrist-following
    // position there each tick. This mirrors that same position onto the
    // already-existing mesh/label components each tick rather than
    // inventing a second, Unreal-owned notion of "where the target is".
    void RefreshScanTargetPosition();
    // Applies the exact same authoritative-selection-highlight and
    // authoritative-position-mirroring pattern as the two methods above to
    // every additional registered reference target, so grasping/moving or
    // selecting one of them stays visually consistent with the bootstrap
    // target instead of silently desyncing.
    void RefreshReferenceTargets();
    const UProbeSimulationAdapter* ResolvePlayerAdapter() const;

    UPROPERTY(VisibleAnywhere, Category="Everward|Phase2")
    TObjectPtr<USceneComponent> SceneRoot;
    UPROPERTY(VisibleAnywhere, Category="Everward|Phase2")
    TObjectPtr<UStaticMeshComponent> ScanTargetMesh;
    UPROPERTY(VisibleAnywhere, Category="Everward|Phase2")
    TObjectPtr<UTextRenderComponent> ScanTargetLabel;
    UPROPERTY(VisibleAnywhere, Category="Everward|Phase2")
    TObjectPtr<UPointLightComponent> KeyLight;
    UPROPERTY(VisibleAnywhere, Category="Everward|Phase2")
    TArray<TObjectPtr<UStaticMeshComponent>> ReferenceMarkers;
    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> ScanTargetDynamicMaterial;
    bool bScanTargetHighlightActive = false;

    // Parallel arrays (index N describes the same additional reference
    // target across all four): the mesh/label components mirror
    // ReferenceMarkers' pattern above, the dynamic material is UPROPERTY
    // Transient exactly like ScanTargetDynamicMaterial (a loose
    // UMaterialInstanceDynamic needs its own GC reference), and the id/
    // highlight-active bookkeeping are plain values, not UObjects.
    UPROPERTY(VisibleAnywhere, Category="Everward|Phase2")
    TArray<TObjectPtr<UStaticMeshComponent>> ReferenceTargetMeshes;
    UPROPERTY(VisibleAnywhere, Category="Everward|Phase2")
    TArray<TObjectPtr<UTextRenderComponent>> ReferenceTargetLabels;
    UPROPERTY(Transient)
    TArray<TObjectPtr<UMaterialInstanceDynamic>> ReferenceTargetDynamicMaterials;
    TArray<FString> ReferenceTargetIds;
    TArray<bool> ReferenceTargetHighlightActive;
};