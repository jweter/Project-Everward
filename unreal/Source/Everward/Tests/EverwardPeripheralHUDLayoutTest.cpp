#include "PeripheralHUDLayout.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEverwardG03PeripheralHUDLayoutTest,
    "Everward.G03.Native.PeripheralHUDLayout",
    EAutomationTestFlags::EditorContext |
    EAutomationTestFlags::CommandletContext |
    EAutomationTestFlags::ProductFilter)

namespace
{
bool Overlaps(const FBox2D& A, const FBox2D& B)
{
    return A.Min.X < B.Max.X && A.Max.X > B.Min.X &&
        A.Min.Y < B.Max.Y && A.Max.Y > B.Min.Y;
}
}

bool FEverwardG03PeripheralHUDLayoutTest::RunTest(const FString& Parameters)
{
    const FVector2D Viewports[] = {
        FVector2D(1920.0f, 1080.0f),
        FVector2D(1280.0f, 720.0f)
    };
    for (const FVector2D& Viewport : Viewports)
    {
        const FEverwardPeripheralHUDLayout Layout =
            FEverwardPeripheralHUDLayout::Compute(Viewport.X, Viewport.Y);
        const FBox2D ReservedCenter(
            FVector2D(Viewport.X * 0.20f, Viewport.Y * 0.30f),
            FVector2D(Viewport.X * 0.80f, Viewport.Y * 0.70f));

        TestFalse(TEXT("Vitals leave the central world visible"), Overlaps(Layout.Vitals, ReservedCenter));
        TestFalse(TEXT("Target card leaves the central world visible"), Overlaps(Layout.Target, ReservedCenter));
        TestFalse(TEXT("Action strip leaves the central world visible"), Overlaps(Layout.Action, ReservedCenter));
        TestFalse(TEXT("Vitals do not cover the action strip"), Overlaps(Layout.Vitals, Layout.Action));
        TestFalse(TEXT("Target does not cover the action strip"), Overlaps(Layout.Target, Layout.Action));
        TestTrue(TEXT("Vitals remain within the viewport"),
            Layout.Vitals.Min.X >= 0.0f && Layout.Vitals.Max.X <= Viewport.X &&
            Layout.Vitals.Min.Y >= 0.0f && Layout.Vitals.Max.Y <= Viewport.Y);
        TestTrue(TEXT("Target remains within the viewport"),
            Layout.Target.Min.X >= 0.0f && Layout.Target.Max.X <= Viewport.X &&
            Layout.Target.Min.Y >= 0.0f && Layout.Target.Max.Y <= Viewport.Y);
        TestTrue(TEXT("Action remains within the viewport"),
            Layout.Action.Min.X >= 0.0f && Layout.Action.Max.X <= Viewport.X &&
            Layout.Action.Min.Y >= 0.0f && Layout.Action.Max.Y <= Viewport.Y);
        TestTrue(TEXT("720p HUD layout keeps a stable visual scale"), Layout.Scale >= 0.80f);
        TestTrue(TEXT("Target card belongs to peripheral right"),
            Layout.Target.Min.X >= Viewport.X * 0.70f);
        TestTrue(TEXT("Vitals belong to peripheral left"),
            Layout.Vitals.Max.X <= Viewport.X * 0.30f);
    }
    return !HasAnyErrors();
}
#endif // WITH_DEV_AUTOMATION_TESTS
