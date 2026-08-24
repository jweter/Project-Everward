#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "EverwardPlayerController.generated.h"

UCLASS()
class EVERWARD_API AEverwardPlayerController : public APlayerController
{
    GENERATED_BODY()

protected:
    virtual void SetupInputComponent() override;

private:
    void ToggleSystemsPanel();
    void SelectNextCapability();
    void SelectPreviousCapability();
};
