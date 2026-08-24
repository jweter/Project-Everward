#include "EverwardPlayerController.h"

#include "EverwardHUD.h"
#include "InputCoreTypes.h"

void AEverwardPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    if (InputComponent == nullptr)
    {
        return;
    }

    InputComponent->BindKey(EKeys::Tab, IE_Pressed, this, &AEverwardPlayerController::ToggleSystemsPanel);
    InputComponent->BindKey(EKeys::RightBracket, IE_Pressed, this, &AEverwardPlayerController::SelectNextCapability);
    InputComponent->BindKey(EKeys::LeftBracket, IE_Pressed, this, &AEverwardPlayerController::SelectPreviousCapability);
}

void AEverwardPlayerController::ToggleSystemsPanel()
{
    if (AEverwardHUD* EverwardHUD = Cast<AEverwardHUD>(GetHUD()))
    {
        EverwardHUD->ToggleSystemsPanel();
    }
}

void AEverwardPlayerController::SelectNextCapability()
{
    if (AEverwardHUD* EverwardHUD = Cast<AEverwardHUD>(GetHUD()))
    {
        EverwardHUD->SelectNextCapability();
    }
}

void AEverwardPlayerController::SelectPreviousCapability()
{
    if (AEverwardHUD* EverwardHUD = Cast<AEverwardHUD>(GetHUD()))
    {
        EverwardHUD->SelectPreviousCapability();
    }
}
