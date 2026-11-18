#pragma once

#include "CoreMinimal.h"

// Presentation-only geometry for the default, world-first Phase-2 HUD.
// These rectangles never determine simulation range, eligibility or actions.
// The full systems/manipulator overlays are deliberately not constrained by
// the default HUD's unobstructed center; the player explicitly opens them.
struct FEverwardPeripheralHUDLayout
{
    float Scale = 1.0f;
    FBox2D Vitals;
    FBox2D Target;
    FBox2D Action;

    static float ResolveScale(float ViewportWidth, float ViewportHeight)
    {
        if (ViewportWidth <= 0.0f || ViewportHeight <= 0.0f)
        {
            return 1.0f;
        }
        return FMath::Clamp(
            FMath::Min(ViewportWidth / 1920.0f, ViewportHeight / 1080.0f),
            0.80f, 1.75f);
    }

    static FEverwardPeripheralHUDLayout Compute(float ViewportWidth, float ViewportHeight)
    {
        FEverwardPeripheralHUDLayout Layout;
        Layout.Scale = ResolveScale(ViewportWidth, ViewportHeight);
        const float Scale = Layout.Scale;
        const float Margin = 24.0f * Scale;

        // Two compact corner zones leave the central visual field empty.
        // The default action is beneath the central view, not above the probe.
        const float VitalsWidth = FMath::Min(340.0f * Scale, ViewportWidth * 0.28f);
        const float VitalsHeight = 140.0f * Scale;
        const float VitalsY = ViewportHeight - Margin - VitalsHeight;
        Layout.Vitals = FBox2D(
            FVector2D(Margin, VitalsY),
            FVector2D(Margin + VitalsWidth, VitalsY + VitalsHeight));

        const float TargetWidth = FMath::Min(320.0f * Scale, ViewportWidth * 0.28f);
        const float TargetHeight = 132.0f * Scale;
        const float TargetX = ViewportWidth - Margin - TargetWidth;
        const float TargetY = Margin + 62.0f * Scale;
        Layout.Target = FBox2D(
            FVector2D(TargetX, TargetY),
            FVector2D(TargetX + TargetWidth, TargetY + TargetHeight));

        const float ActionWidth = FMath::Min(
            700.0f * Scale, ViewportWidth - 2.0f * Margin);
        const float ActionHeight = 50.0f * Scale;
        const float ActionX = (ViewportWidth - ActionWidth) * 0.5f;
        const float ActionY = ViewportHeight - Margin - ActionHeight;
        Layout.Action = FBox2D(
            FVector2D(ActionX, ActionY),
            FVector2D(ActionX + ActionWidth, ActionY + ActionHeight));
        return Layout;
    }
};
