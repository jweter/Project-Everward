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

private:
    bool bSystemsExpanded = false;
    int32 SelectedCapabilityIndex = 0;
};
