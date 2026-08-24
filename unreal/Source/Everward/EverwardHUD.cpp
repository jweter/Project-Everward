#include "EverwardHUD.h"

#include "CanvasItem.h"
#include "Engine/Canvas.h"
#include "EverwardProbePawn.h"
#include "ProbeSimulationAdapter.h"

namespace
{
FString PercentString(double Value, double Capacity)
{
    if (Capacity <= 0.0)
    {
        return TEXT("--");
    }
    return FString::Printf(TEXT("%.0f%%"), FMath::Clamp(Value / Capacity, 0.0, 1.0) * 100.0);
}

FString CapabilityState(const FEverwardProbeCapability& Capability)
{
    if (!Capability.bOperational)
    {
        return TEXT("FAILED");
    }
    if (!Capability.bAvailable)
    {
        return TEXT("LOCKED");
    }
    return TEXT("READY");
}
}

void AEverwardHUD::DrawHUD()
{
    Super::DrawHUD();

    if (Canvas == nullptr || PlayerOwner == nullptr)
    {
        return;
    }

    const AEverwardProbePawn* Probe = Cast<AEverwardProbePawn>(PlayerOwner->GetPawn());
    if (Probe == nullptr || Probe->GetSimulationAdapter() == nullptr)
    {
        return;
    }

    const UProbeSimulationAdapter* Adapter = Probe->GetSimulationAdapter();
    const FEverwardProbeTelemetry Telemetry = Adapter->GetProbeTelemetry();
    const TArray<FEverwardProbeCapability> Capabilities = Adapter->GetInstalledCapabilities();
    const FEverwardProbeCommandResult LastCommand = Adapter->GetLastCommandResult();

    const float Margin = 24.0f;
    const float LineHeight = 22.0f;
    const float PanelWidth = 350.0f;
    const FLinearColor PanelColor(0.015f, 0.025f, 0.035f, 0.78f);
    const FLinearColor TextColor(0.86f, 0.92f, 0.95f, 1.0f);
    const FLinearColor MutedColor(0.55f, 0.65f, 0.70f, 1.0f);
    const FLinearColor AlertColor(1.0f, 0.36f, 0.18f, 1.0f);

    const float TelemetryHeight = 178.0f;
    const float TelemetryY = Canvas->ClipY - Margin - TelemetryHeight;
    DrawRect(PanelColor, Margin, TelemetryY, PanelWidth, TelemetryHeight);
    DrawText(
        FString::Printf(TEXT("%s  //  GEN %d"), *Telemetry.ProbeId, Telemetry.Generation),
        TextColor,
        Margin + 14.0f,
        TelemetryY + 10.0f,
        nullptr,
        1.0f,
        false);
    DrawText(
        FString::Printf(TEXT("ENERGY  %s"), *PercentString(Telemetry.StoredEnergyJoules, Telemetry.EnergyCapacityJoules)),
        TextColor,
        Margin + 14.0f,
        TelemetryY + 38.0f,
        nullptr,
        1.0f,
        false);
    DrawText(
        FString::Printf(TEXT("POWER   %.0f / %.0f W"), Telemetry.TotalPowerAllocatedWatts, Telemetry.PowerCapacityWatts),
        TextColor,
        Margin + 14.0f,
        TelemetryY + 38.0f + LineHeight,
        nullptr,
        1.0f,
        false);
    DrawText(
        FString::Printf(TEXT("THERMAL %.1f K"), Telemetry.TemperatureKelvin),
        TextColor,
        Margin + 14.0f,
        TelemetryY + 38.0f + LineHeight * 2.0f,
        nullptr,
        1.0f,
        false);
    DrawText(
        FString::Printf(TEXT("STORAGE %s"), *PercentString(Telemetry.StorageUsedKilograms, Telemetry.StorageCapacityKilograms)),
        TextColor,
        Margin + 14.0f,
        TelemetryY + 38.0f + LineHeight * 3.0f,
        nullptr,
        1.0f,
        false);
    DrawText(
        FString::Printf(TEXT("VELOCITY %.2f m/s"), Telemetry.VelocityMetersPerSecond.Size()),
        TextColor,
        Margin + 14.0f,
        TelemetryY + 38.0f + LineHeight * 4.0f,
        nullptr,
        1.0f,
        false);
    DrawText(
        FString::Printf(TEXT("SIM %.1f s"), Telemetry.SimulationTimeSeconds),
        MutedColor,
        Margin + 14.0f,
        TelemetryY + 38.0f + LineHeight * 5.0f,
        nullptr,
        0.9f,
        false);

    float AlertY = Margin;
    if (Telemetry.bIsEnergyDepleted || Telemetry.bIsOverheated)
    {
        FString AlertText;
        if (Telemetry.bIsEnergyDepleted)
        {
            AlertText += TEXT("ENERGY DEPLETED");
        }
        if (Telemetry.bIsOverheated)
        {
            if (!AlertText.IsEmpty())
            {
                AlertText += TEXT("  //  ");
            }
            AlertText += TEXT("THERMAL LOCKOUT");
        }
        DrawText(AlertText, AlertColor, Margin, AlertY, nullptr, 1.25f, false);
        AlertY += 28.0f;
    }

    if (Telemetry.bIsScanning)
    {
        DrawText(
            FString::Printf(
                TEXT("SCAN  %s  //  %.1f s REMAINING"),
                *Telemetry.ActiveScanTargetId,
                Telemetry.ScanRemainingSeconds),
            TextColor,
            Margin,
            AlertY,
            nullptr,
            0.95f,
            false);
    }

    const float SystemPanelX = Canvas->ClipX - Margin - PanelWidth;
    const float CompactHeight = 48.0f;
    const float CompactY = Canvas->ClipY - Margin - CompactHeight;

    if (!bSystemsExpanded)
    {
        DrawRect(PanelColor, SystemPanelX, CompactY, PanelWidth, CompactHeight);
        DrawText(
            FString::Printf(TEXT("SYSTEMS  %d INSTALLED   [TAB]"), Capabilities.Num()),
            TextColor,
            SystemPanelX + 14.0f,
            CompactY + 14.0f,
            nullptr,
            0.95f,
            false);
        return;
    }

    const float ExpandedHeight = 410.0f;
    const float ExpandedY = Canvas->ClipY - Margin - ExpandedHeight;
    DrawRect(PanelColor, SystemPanelX, ExpandedY, PanelWidth, ExpandedHeight);
    DrawText(TEXT("SYSTEMS / CONTROL   [TAB CLOSE]"), TextColor, SystemPanelX + 14.0f, ExpandedY + 12.0f, nullptr, 0.95f, false);

    if (Capabilities.IsEmpty())
    {
        DrawText(TEXT("NO INSTALLED CAPABILITIES"), MutedColor, SystemPanelX + 14.0f, ExpandedY + 48.0f, nullptr, 0.9f, false);
        return;
    }

    SelectedCapabilityIndex = FMath::Clamp(SelectedCapabilityIndex, 0, Capabilities.Num() - 1);

    float Y = ExpandedY + 44.0f;
    for (int32 Index = 0; Index < Capabilities.Num(); ++Index)
    {
        const FEverwardProbeCapability& Capability = Capabilities[Index];
        const FString Prefix = Index == SelectedCapabilityIndex ? TEXT("> ") : TEXT("  ");
        DrawText(
            FString::Printf(TEXT("%s%s  [%s]"), *Prefix, *Capability.DisplayName, *CapabilityState(Capability)),
            Index == SelectedCapabilityIndex ? TextColor : MutedColor,
            SystemPanelX + 14.0f,
            Y,
            nullptr,
            0.9f,
            false);
        Y += 24.0f;
    }

    const FEverwardProbeCapability& Selected = Capabilities[SelectedCapabilityIndex];
    Y += 8.0f;
    DrawText(Selected.Description, TextColor, SystemPanelX + 14.0f, Y, nullptr, 0.82f, false);
    Y += 26.0f;
    DrawText(
        FString::Printf(TEXT("POWER %.0f W   [PGUP/PGDN ADJUST]"), Selected.AllocatedPowerWatts),
        MutedColor,
        SystemPanelX + 14.0f,
        Y,
        nullptr,
        0.82f,
        false);
    Y += 22.0f;
    DrawText(
        FString::Printf(
            TEXT("MANUAL CONTROL  %s   //   AUTOMATION API  %s"),
            Selected.bSupportsManualControl ? TEXT("YES") : TEXT("N/A"),
            Selected.bSupportsAutomation ? TEXT("YES") : TEXT("N/A")),
        MutedColor,
        SystemPanelX + 14.0f,
        Y,
        nullptr,
        0.82f,
        false);
    Y += 28.0f;

    if (Selected.CapabilityId == FName(TEXT("sensors")))
    {
        DrawText(TEXT("[ENTER] START SCAN   [BACKSPACE] CANCEL"), TextColor, SystemPanelX + 14.0f, Y, nullptr, 0.8f, false);
        Y += 22.0f;
        DrawText(
            Telemetry.bIsScanning
                ? FString::Printf(TEXT("ACTIVE: %s  %.1f s"), *Telemetry.ActiveScanTargetId, Telemetry.ScanRemainingSeconds)
                : TEXT("ACTIVE: NONE"),
            MutedColor,
            SystemPanelX + 14.0f,
            Y,
            nullptr,
            0.8f,
            false);
        Y += 24.0f;
    }
    else if (Selected.CapabilityId == FName(TEXT("propulsion")))
    {
        DrawText(TEXT("[UP/DOWN] X VELOCITY +/-   [SPACE] STOP"), TextColor, SystemPanelX + 14.0f, Y, nullptr, 0.8f, false);
        Y += 22.0f;
        DrawText(
            FString::Printf(TEXT("VECTOR [%.2f, %.2f, %.2f] m/s"),
                Telemetry.VelocityMetersPerSecond.X,
                Telemetry.VelocityMetersPerSecond.Y,
                Telemetry.VelocityMetersPerSecond.Z),
            MutedColor,
            SystemPanelX + 14.0f,
            Y,
            nullptr,
            0.8f,
            false);
        Y += 24.0f;
    }

    if (LastCommand.Sequence > 0)
    {
        DrawText(
            FString::Printf(
                TEXT("CMD %s: %s"),
                LastCommand.bAccepted ? TEXT("OK") : TEXT("REJECTED"),
                *LastCommand.Detail),
            LastCommand.bAccepted ? TextColor : AlertColor,
            SystemPanelX + 14.0f,
            ExpandedY + ExpandedHeight - 52.0f,
            nullptr,
            0.75f,
            false);
    }

    DrawText(TEXT("[ / ] SELECT SYSTEM"), MutedColor, SystemPanelX + 14.0f, ExpandedY + ExpandedHeight - 26.0f, nullptr, 0.8f, false);
}

void AEverwardHUD::ToggleSystemsPanel()
{
    bSystemsExpanded = !bSystemsExpanded;
}

void AEverwardHUD::SelectNextCapability()
{
    ++SelectedCapabilityIndex;
}

void AEverwardHUD::SelectPreviousCapability()
{
    SelectedCapabilityIndex = FMath::Max(0, SelectedCapabilityIndex - 1);
}

bool AEverwardHUD::IsSystemsPanelExpanded() const
{
    return bSystemsExpanded;
}

int32 AEverwardHUD::GetSelectedCapabilityIndex() const
{
    return SelectedCapabilityIndex;
}
