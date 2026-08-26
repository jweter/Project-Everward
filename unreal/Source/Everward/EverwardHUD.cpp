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
    const FEverwardSoftwarePolicyStatus PolicyStatus = Adapter->GetSoftwarePolicyStatus();
    const FEverwardProbeCommandResult LastCommand = Adapter->GetLastCommandResult();

    // Persist visible payoff when an authoritative scan lifecycle transitions from
    // active to complete. Cancellation is explicitly distinguished by the shared
    // command result, so cancelled work can never fabricate a discovery.
    if (Telemetry.bIsScanning)
    {
        bWasScanning = true;
        LastObservedScanTargetId = Telemetry.ActiveScanTargetId;
    }
    else if (bWasScanning)
    {
        bWasScanning = false;
        const bool bWasCancelled = LastCommand.bAccepted && LastCommand.CommandId == FName(TEXT("cancel_scan"));
        if (!bWasCancelled && !LastObservedScanTargetId.IsEmpty())
        {
            bHasScanDiscovery = true;
            LastScanTargetId = LastObservedScanTargetId;
            LastScanObjectClass = TEXT("ROCKY BODY / SURVEY REFERENCE");
            LastScanComposition = TEXT("SILICATE-RICH // IRON-BEARING MATERIAL");
            LastScanConfidence = 0.92;
            LastScanCompletedAtSeconds = Telemetry.SimulationTimeSeconds;
        }
        LastObservedScanTargetId.Reset();
    }

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
    else if (bHasScanDiscovery)
    {
        DrawText(
            FString::Printf(TEXT("SCAN COMPLETE  //  %s  //  DISCOVERY STORED"), *LastScanTargetId),
            TextColor,
            Margin,
            AlertY,
            nullptr,
            0.95f,
            false);
    }

    // Flight state is safety-critical and should never disappear just because the
    // player is inspecting Sensors/Computation/Thermal. Convert authoritative world
    // velocity back into probe-local axes so the readout answers the useful question:
    // am I moving forward/back, right/left, or up/down relative to my current attitude?
    const double SpeedMetersPerSecond = Telemetry.VelocityMetersPerSecond.Size();
    if (SpeedMetersPerSecond > 0.01)
    {
        const FVector LocalVelocity = Probe->GetActorTransform().InverseTransformVectorNoScale(
            Telemetry.VelocityMetersPerSecond);
        const float FlightWidth = 430.0f;
        const float FlightHeight = 72.0f;
        const float FlightX = (Canvas->ClipX - FlightWidth) * 0.5f;
        const float FlightY = Margin;
        DrawRect(PanelColor, FlightX, FlightY, FlightWidth, FlightHeight);
        DrawText(
            FString::Printf(TEXT("FLIGHT  %.2f m/s   [SPACE] BRAKE"), SpeedMetersPerSecond),
            TextColor,
            FlightX + 14.0f,
            FlightY + 10.0f,
            nullptr,
            0.95f,
            false);
        DrawText(
            FString::Printf(
                TEXT("FWD %+0.2f   RIGHT %+0.2f   UP %+0.2f m/s"),
                LocalVelocity.X,
                LocalVelocity.Y,
                LocalVelocity.Z),
            MutedColor,
            FlightX + 14.0f,
            FlightY + 38.0f,
            nullptr,
            0.82f,
            false);
    }

    const float SystemPanelX = Canvas->ClipX - Margin - PanelWidth;
    const float CompactHeaderHeight = 42.0f;
    const float CompactRowHeight = 20.0f;
    const float CompactHeight = CompactHeaderHeight + CompactRowHeight * FMath::Max(Capabilities.Num(), 1);
    const float CompactY = Canvas->ClipY - Margin - CompactHeight;

    if (!bSystemsExpanded)
    {
        DrawRect(PanelColor, SystemPanelX, CompactY, PanelWidth, CompactHeight);
        DrawText(
            FString::Printf(TEXT("SYSTEMS  %d INSTALLED   [TAB DETAILS]"), Capabilities.Num()),
            TextColor,
            SystemPanelX + 14.0f,
            CompactY + 10.0f,
            nullptr,
            0.90f,
            false);

        if (Capabilities.IsEmpty())
        {
            DrawText(
                TEXT("NO INSTALLED CAPABILITIES"),
                MutedColor,
                SystemPanelX + 14.0f,
                CompactY + CompactHeaderHeight,
                nullptr,
                0.78f,
                false);
        }
        else
        {
            float CompactRowY = CompactY + CompactHeaderHeight;
            for (const FEverwardProbeCapability& Capability : Capabilities)
            {
                const bool bHealthy = Capability.bOperational && Capability.bAvailable;
                DrawText(
                    FString::Printf(
                        TEXT("%s   %.0f W   [%s]"),
                        *Capability.DisplayName,
                        Capability.AllocatedPowerWatts,
                        *CapabilityState(Capability)),
                    bHealthy ? TextColor : AlertColor,
                    SystemPanelX + 14.0f,
                    CompactRowY,
                    nullptr,
                    0.76f,
                    false);
                CompactRowY += CompactRowHeight;
            }
        }
        return;
    }

    const float ExpandedHeight = 450.0f;
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

        if (bHasScanDiscovery)
        {
            DrawText(TEXT("LAST DISCOVERY"), TextColor, SystemPanelX + 14.0f, Y, nullptr, 0.82f, false);
            Y += 20.0f;
            DrawText(LastScanTargetId, MutedColor, SystemPanelX + 14.0f, Y, nullptr, 0.76f, false);
            Y += 19.0f;
            DrawText(LastScanObjectClass, TextColor, SystemPanelX + 14.0f, Y, nullptr, 0.74f, false);
            Y += 19.0f;
            DrawText(LastScanComposition, MutedColor, SystemPanelX + 14.0f, Y, nullptr, 0.70f, false);
            Y += 19.0f;
            DrawText(
                FString::Printf(TEXT("CONFIDENCE %.0f%%  //  ACQUIRED T+%.1fs"), LastScanConfidence * 100.0, LastScanCompletedAtSeconds),
                MutedColor,
                SystemPanelX + 14.0f,
                Y,
                nullptr,
                0.70f,
                false);
            Y += 24.0f;
        }
    }
    else if (Selected.CapabilityId == FName(TEXT("propulsion")))
    {
        DrawText(TEXT("[W/S] X  [A/D] Y  [Q/E] Z  [SPACE] GLOBAL BRAKE"), TextColor, SystemPanelX + 14.0f, Y, nullptr, 0.72f, false);
        Y += 22.0f;
        DrawText(
            FString::Printf(TEXT("WORLD VECTOR [%.2f, %.2f, %.2f] m/s"),
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
    else if (Selected.CapabilityId == FName(TEXT("computation")))
    {
        DrawText(TEXT("[ENTER] INSTALL BASIC SURVIVAL   [BACKSPACE] CLEAR"), TextColor, SystemPanelX + 14.0f, Y, nullptr, 0.76f, false);
        Y += 22.0f;
        DrawText(
            PolicyStatus.bInstalled
                ? FString::Printf(TEXT("POLICY: %s  //  %d RULES"), *PolicyStatus.PolicyId, PolicyStatus.RuleCount)
                : TEXT("POLICY: NONE"),
            MutedColor,
            SystemPanelX + 14.0f,
            Y,
            nullptr,
            0.8f,
            false);
        Y += 22.0f;
        DrawText(
            PolicyStatus.bExecutorAvailable
                ? TEXT("EXECUTOR: RUNNING")
                : FString::Printf(TEXT("EXECUTOR: NEED >= %.0f W COMPUTE"), PolicyStatus.MinimumComputationPowerWatts),
            PolicyStatus.bExecutorAvailable ? TextColor : AlertColor,
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

    DrawText(TEXT("[ / ] SELECT SYSTEM   //   MOUSE LOOK   WHEEL ZOOM"), MutedColor, SystemPanelX + 14.0f, ExpandedY + ExpandedHeight - 26.0f, nullptr, 0.68f, false);
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