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

    // Phase-2 scan-discovery read model. Completion/cancellation truth comes
    // from the adapter's authoritative domain-event projection rather than
    // being inferred from whichever manual command happened most recently.
    bool bHasScanDiscovery = false;
    int64 LastHandledScanNoticeSequence = 0;
    FString LastObservedScanTargetId;
    FString LastScanTargetId;
    FString LastScanObjectClass;
    FString LastScanComposition;
    double LastScanConfidence = 0.0;
    double LastScanCompletedAtSeconds = 0.0;

    // Brief global feedback banners make rejected commands and automation
    // actions visible even when their owning subsystem detail page is closed.
    int64 LastObservedCommandSequence = 0;
    int64 LastObservedAutomationSequence = 0;
    double CommandBannerExpiresAtWorldSeconds = 0.0;
    double AutomationBannerExpiresAtWorldSeconds = 0.0;
};