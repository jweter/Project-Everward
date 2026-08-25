#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "EverwardHUD.generated.h"

UCLASS()
class EVERWARD_API AEverwardHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void DrawHUD() override;

    UFUNCTION(BlueprintCallable, Category="Everward|HUD")
    void ToggleSystemsPanel();

    UFUNCTION(BlueprintCallable, Category="Everward|HUD")
    void SelectNextCapability();

    UFUNCTION(BlueprintCallable, Category="Everward|HUD")
    void SelectPreviousCapability();

    UFUNCTION(BlueprintPure, Category="Everward|HUD")
    bool IsSystemsPanelExpanded() const;

    UFUNCTION(BlueprintPure, Category="Everward|HUD")
    int32 GetSelectedCapabilityIndex() const;

private:
    bool bSystemsExpanded = false;
    int32 SelectedCapabilityIndex = 0;

    // Phase-2 scan-discovery read model. Completion/cancellation truth still
    // comes from the authoritative adapter telemetry/command boundary; the HUD
    // only persists the most recent completed discovery so the payoff does not
    // disappear the frame scanning ends.
    bool bWasScanning = false;
    bool bHasScanDiscovery = false;
    FString LastObservedScanTargetId;
    FString LastScanTargetId;
    FString LastScanObjectClass;
    FString LastScanComposition;
    double LastScanConfidence = 0.0;
    double LastScanCompletedAtSeconds = 0.0;
};
