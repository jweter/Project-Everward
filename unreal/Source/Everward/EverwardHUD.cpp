#include "EverwardHUD.h"

#include "CanvasItem.h"
#include "Engine/Canvas.h"
#include "Engine/World.h"
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

FString ManipulatorArmLine(const FString& Label, const FEverwardManipulatorArmState& Arm)
{
    FString StateText;
    if (Arm.bIsDeploying)
    {
        StateText = FString::Printf(TEXT("DEPLOYING %.0f%%"), Arm.DeploymentFraction * 100.0);
    }
    else if (Arm.bIsStowing)
    {
        StateText = FString::Printf(TEXT("STOWING %.0f%%"), Arm.DeploymentFraction * 100.0);
    }
    else if (Arm.bIsDeployed)
    {
        StateText = TEXT("DEPLOYED");
    }
    else
    {
        StateText = TEXT("STOWED");
    }

    return FString::Printf(
        TEXT("%s ARM  %s%s"),
        *Label,
        *StateText,
        Arm.bToolAttached ? TEXT("  //  TOOL") : TEXT(""));
}

const TCHAR* ManipulatorJointIndexName(int32 JointIndex)
{
    switch (JointIndex)
    {
        case 0: return TEXT("SHOULDER");
        case 1: return TEXT("ELBOW");
        default: return TEXT("WRIST");
    }
}

// Current -> commanded target for one joint, selected by JointIndex
// (0=Shoulder, 1=Elbow, 2=Wrist), matching FEverwardManipulatorArmState's
// field-for-field mirror of ManipulatorArmAngles.
FString ManipulatorJointLine(int32 JointIndex, const FEverwardManipulatorArmState& Arm)
{
    double Current = 0.0;
    double Commanded = 0.0;
    switch (JointIndex)
    {
        case 0: Current = Arm.ShoulderDegrees; Commanded = Arm.CommandedShoulderDegrees; break;
        case 1: Current = Arm.ElbowDegrees; Commanded = Arm.CommandedElbowDegrees; break;
        default: Current = Arm.WristDegrees; Commanded = Arm.CommandedWristDegrees; break;
    }
    return FString::Printf(
        TEXT("%-8s %6.1f DEG -> %6.1f DEG"),
        ManipulatorJointIndexName(JointIndex),
        Current,
        Commanded);
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
    const TArray<FEverwardManipulatorArmState> ManipulatorArms = Adapter->GetManipulatorArmStates();
    const FEverwardSoftwarePolicyStatus PolicyStatus = Adapter->GetSoftwarePolicyStatus();
    const FEverwardProbeCommandResult LastCommand = Adapter->GetLastCommandResult();
    const FEverwardAutomationNotice AutomationNotice = Adapter->GetLastAutomationNotice();
    const FEverwardScanLifecycleNotice ScanNotice = Adapter->GetLastScanLifecycleNotice();
    const double WorldNow = GetWorld() != nullptr ? GetWorld()->GetTimeSeconds() : 0.0;

    if (LastCommand.Sequence > 0 && LastCommand.Sequence != LastObservedCommandSequence)
    {
        LastObservedCommandSequence = LastCommand.Sequence;
        CommandBannerExpiresAtWorldSeconds = WorldNow + 4.0;
    }
    if (AutomationNotice.Sequence > 0 && AutomationNotice.Sequence != LastObservedAutomationSequence)
    {
        LastObservedAutomationSequence = AutomationNotice.Sequence;
        AutomationBannerExpiresAtWorldSeconds = WorldNow + 6.0;
    }

    if (Telemetry.bIsScanning)
    {
        LastObservedScanTargetId = Telemetry.ActiveScanTargetId;
    }

    if (ScanNotice.Sequence > 0 && ScanNotice.Sequence != LastHandledScanNoticeSequence)
    {
        LastHandledScanNoticeSequence = ScanNotice.Sequence;
        if (ScanNotice.bCompleted && !LastObservedScanTargetId.IsEmpty())
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

    const float TelemetryHeight = 178.0f + LineHeight * 2.0f;
    const float TelemetryY = Canvas->ClipY - Margin - TelemetryHeight;
    DrawRect(PanelColor, Margin, TelemetryY, PanelWidth, TelemetryHeight);
    DrawText(
        FString::Printf(TEXT("%s  //  GEN %d"), *Telemetry.ProbeId, Telemetry.Generation),
        TextColor, Margin + 14.0f, TelemetryY + 10.0f, nullptr, 1.0f, false);
    DrawText(
        FString::Printf(TEXT("ENERGY  %s"), *PercentString(Telemetry.StoredEnergyJoules, Telemetry.EnergyCapacityJoules)),
        TextColor, Margin + 14.0f, TelemetryY + 38.0f, nullptr, 1.0f, false);
    DrawText(
        FString::Printf(TEXT("POWER   %.0f / %.0f W"), Telemetry.TotalPowerAllocatedWatts, Telemetry.PowerCapacityWatts),
        TextColor, Margin + 14.0f, TelemetryY + 38.0f + LineHeight, nullptr, 1.0f, false);
    DrawText(
        FString::Printf(TEXT("THERMAL %.1f K"), Telemetry.TemperatureKelvin),
        TextColor, Margin + 14.0f, TelemetryY + 38.0f + LineHeight * 2.0f, nullptr, 1.0f, false);
    DrawText(
        FString::Printf(TEXT("STORAGE %s"), *PercentString(Telemetry.StorageUsedKilograms, Telemetry.StorageCapacityKilograms)),
        TextColor, Margin + 14.0f, TelemetryY + 38.0f + LineHeight * 3.0f, nullptr, 1.0f, false);
    DrawText(
        FString::Printf(TEXT("VELOCITY %.2f m/s"), Telemetry.VelocityMetersPerSecond.Size()),
        TextColor, Margin + 14.0f, TelemetryY + 38.0f + LineHeight * 4.0f, nullptr, 1.0f, false);
    DrawText(
        FString::Printf(TEXT("SIM %.1f s"), Telemetry.SimulationTimeSeconds),
        MutedColor, Margin + 14.0f, TelemetryY + 38.0f + LineHeight * 5.0f, nullptr, 0.9f, false);
    for (int32 ArmIndex = 0; ArmIndex < ManipulatorArms.Num(); ++ArmIndex)
    {
        const FEverwardManipulatorArmState& Arm = ManipulatorArms[ArmIndex];
        const FString Label = Arm.ArmId == EEverwardManipulatorArmId::Port ? TEXT("PORT") : TEXT("STBD");
        DrawText(
            ManipulatorArmLine(Label, Arm),
            Arm.bIsDeployed || Arm.bIsDeploying || Arm.bIsStowing ? TextColor : MutedColor,
            Margin + 14.0f,
            TelemetryY + 38.0f + LineHeight * (6.0f + ArmIndex),
            nullptr,
            0.86f,
            false);
    }

    // Dedicated manipulator HUD page (Slice 6 joint-articulation follow-up).
    // Rendered above the compact telemetry panel so it never overlaps the
    // always-visible PORT/STBD status lines it elaborates on.
    if (bManipulatorPanelExpanded && !ManipulatorArms.IsEmpty())
    {
        SelectedManipulatorArmIndex = FMath::Clamp(SelectedManipulatorArmIndex, 0, ManipulatorArms.Num() - 1);
        SelectedManipulatorJointIndex = FMath::Clamp(SelectedManipulatorJointIndex, 0, 2);

        const float PanelHeight = 34.0f + LineHeight * (2.0f * ManipulatorArms.Num() + 4.0f);
        const float PanelY = TelemetryY - 12.0f - PanelHeight;
        DrawRect(PanelColor, Margin, PanelY, PanelWidth, PanelHeight);
        DrawText(TEXT("MANIPULATOR CONTROL   [M CLOSE]"), TextColor,
            Margin + 14.0f, PanelY + 10.0f, nullptr, 0.90f, false);

        float ArmY = PanelY + 34.0f;
        for (int32 ArmIndex = 0; ArmIndex < ManipulatorArms.Num(); ++ArmIndex)
        {
            const FEverwardManipulatorArmState& Arm = ManipulatorArms[ArmIndex];
            const bool bArmSelected = ArmIndex == SelectedManipulatorArmIndex;
            const FString Label = Arm.ArmId == EEverwardManipulatorArmId::Port ? TEXT("PORT") : TEXT("STBD");
            DrawText(
                FString::Printf(TEXT("%s%s"), bArmSelected ? TEXT("> ") : TEXT("  "), *ManipulatorArmLine(Label, Arm)),
                bArmSelected ? TextColor : MutedColor,
                Margin + 14.0f, ArmY, nullptr, 0.82f, false);
            ArmY += LineHeight;

            if (!Arm.bIsDeployed)
            {
                DrawText(TEXT("    DEPLOY ARM TO COMMAND JOINTS"), MutedColor,
                    Margin + 14.0f, ArmY, nullptr, 0.7f, false);
                ArmY += LineHeight;
                continue;
            }

            for (int32 JointIndex = 0; JointIndex < 3; ++JointIndex)
            {
                const bool bJointSelected = bArmSelected && JointIndex == SelectedManipulatorJointIndex;
                DrawText(
                    FString::Printf(TEXT("  %s%s"), bJointSelected ? TEXT("> ") : TEXT("  "), *ManipulatorJointLine(JointIndex, Arm)),
                    bJointSelected ? TextColor : MutedColor,
                    Margin + 14.0f, ArmY, nullptr, 0.7f, false);
                ArmY += LineHeight;
            }
        }

        DrawText(TEXT("[N] ARM   [4/5/6] SHOULDER/ELBOW/WRIST   [,][.] TARGET"), MutedColor,
            Margin + 14.0f, PanelY + PanelHeight - LineHeight, nullptr, 0.62f, false);
    }

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

    // Contact and damage are authoritative simulation telemetry, not Unreal
    // hit events. Slice 4 replaces the old speed-only warning with the actual
    // energy/severity/component result produced by the damage runtime.
    const double ContactAgeSeconds = Telemetry.bHasContactHistory
        ? FMath::Max(
            0.0,
            static_cast<double>(Telemetry.SimulationTick - Telemetry.LastContactTick) / 1000000.0)
        : 999999.0;
    if (Telemetry.bHasContactHistory && ContactAgeSeconds <= 4.0)
    {
        const bool bDamagingImpact = Telemetry.bHasImpactHistory &&
            Telemetry.LastImpactIntegrityAfter < Telemetry.LastImpactIntegrityBefore;
        DrawText(
            Telemetry.bHasImpactHistory
                ? FString::Printf(
                    TEXT("IMPACT %s // %s // %.1f kJ // NORMAL %.2f m/s"),
                    *Telemetry.LastImpactSeverity,
                    *Telemetry.LastImpactSubsystem,
                    Telemetry.LastImpactEnergyJoules / 1000.0,
                    Telemetry.LastContactNormalSpeedMetersPerSecond)
                : FString::Printf(
                    TEXT("CONTACT // %s // NORMAL %.2f m/s"),
                    *Telemetry.LastContactBodyId,
                    Telemetry.LastContactNormalSpeedMetersPerSecond),
            bDamagingImpact ? AlertColor : TextColor,
            Margin,
            AlertY,
            nullptr,
            0.95f,
            false);
        AlertY += 22.0f;

        if (Telemetry.bHasImpactHistory)
        {
            DrawText(
                FString::Printf(
                    TEXT("%s INTEGRITY %.0f%% -> %.0f%%"),
                    *Telemetry.LastImpactSubsystem,
                    Telemetry.LastImpactIntegrityBefore * 100.0,
                    Telemetry.LastImpactIntegrityAfter * 100.0),
                bDamagingImpact ? AlertColor : MutedColor,
                Margin,
                AlertY,
                nullptr,
                0.78f,
                false);
            AlertY += 20.0f;
        }

        DrawText(
            FString::Printf(
                TEXT("POINT [%.2f %.2f %.2f] m // NORMAL [%.2f %.2f %.2f]"),
                Telemetry.LastContactPointMeters.X,
                Telemetry.LastContactPointMeters.Y,
                Telemetry.LastContactPointMeters.Z,
                Telemetry.LastContactSurfaceNormal.X,
                Telemetry.LastContactSurfaceNormal.Y,
                Telemetry.LastContactSurfaceNormal.Z),
            MutedColor,
            Margin,
            AlertY,
            nullptr,
            0.72f,
            false);
        AlertY += 24.0f;
    }

    if (LastCommand.Sequence > 0 && !LastCommand.bAccepted && WorldNow <= CommandBannerExpiresAtWorldSeconds)
    {
        DrawText(
            FString::Printf(TEXT("COMMAND REJECTED // %s"), *LastCommand.Detail),
            AlertColor, Margin, AlertY, nullptr, 0.92f, false);
        AlertY += 24.0f;
    }

    if (AutomationNotice.Sequence > 0 && WorldNow <= AutomationBannerExpiresAtWorldSeconds)
    {
        DrawText(
            AutomationNotice.Detail,
            AutomationNotice.bRejected ? AlertColor : TextColor,
            Margin, AlertY, nullptr, 0.88f, false);
        AlertY += 24.0f;
    }

    if (Telemetry.bIsScanning)
    {
        DrawText(
            FString::Printf(
                TEXT("SCAN  %s  //  %.1f s REMAINING"),
                *Telemetry.ActiveScanTargetId,
                Telemetry.ScanRemainingSeconds),
            TextColor, Margin, AlertY, nullptr, 0.95f, false);
    }
    else if (bHasScanDiscovery)
    {
        DrawText(
            FString::Printf(TEXT("SCAN COMPLETE  //  %s  //  DISCOVERY STORED"), *LastScanTargetId),
            TextColor, Margin, AlertY, nullptr, 0.95f, false);
    }

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
            TextColor, FlightX + 14.0f, FlightY + 10.0f, nullptr, 0.95f, false);
        DrawText(
            FString::Printf(
                TEXT("FWD %+0.2f   RIGHT %+0.2f   UP %+0.2f m/s"),
                LocalVelocity.X,
                LocalVelocity.Y,
                LocalVelocity.Z),
            MutedColor, FlightX + 14.0f, FlightY + 38.0f, nullptr, 0.82f, false);
    }

    const float SystemPanelX = Canvas->ClipX - Margin - PanelWidth;
    const float CompactHeaderHeight = 42.0f;
    const float CompactRowHeight = 36.0f;
    const float CompactHeight = CompactHeaderHeight + CompactRowHeight * FMath::Max(Capabilities.Num(), 1);
    const float CompactY = Canvas->ClipY - Margin - CompactHeight;

    if (!bSystemsExpanded)
    {
        DrawRect(PanelColor, SystemPanelX, CompactY, PanelWidth, CompactHeight);
        DrawText(
            FString::Printf(TEXT("SYSTEMS  %d INSTALLED   [TAB DETAILS]"), Capabilities.Num()),
            TextColor, SystemPanelX + 14.0f, CompactY + 10.0f, nullptr, 0.90f, false);

        if (Capabilities.IsEmpty())
        {
            DrawText(TEXT("NO INSTALLED CAPABILITIES"), MutedColor,
                SystemPanelX + 14.0f, CompactY + CompactHeaderHeight, nullptr, 0.78f, false);
        }
        else
        {
            float CompactRowY = CompactY + CompactHeaderHeight;
            for (const FEverwardProbeCapability& Capability : Capabilities)
            {
                const bool bHealthy = Capability.bOperational && Capability.bAvailable;
                DrawText(
                    FString::Printf(TEXT("%s   %.0f W   %.0f%%   [%s]"),
                        *Capability.DisplayName,
                        Capability.AllocatedPowerWatts,
                        Capability.IntegrityFraction * 100.0,
                        *CapabilityState(Capability)),
                    bHealthy ? TextColor : AlertColor,
                    SystemPanelX + 14.0f, CompactRowY, nullptr, 0.76f, false);
                DrawText(
                    Capability.StatusReason,
                    bHealthy ? MutedColor : AlertColor,
                    SystemPanelX + 22.0f, CompactRowY + 17.0f, nullptr, 0.63f, false);
                CompactRowY += CompactRowHeight;
            }
        }
        return;
    }

    const float ExpandedHeight = 500.0f;
    const float ExpandedY = Canvas->ClipY - Margin - ExpandedHeight;
    DrawRect(PanelColor, SystemPanelX, ExpandedY, PanelWidth, ExpandedHeight);
    DrawText(TEXT("SYSTEMS / CONTROL   [TAB CLOSE]"), TextColor,
        SystemPanelX + 14.0f, ExpandedY + 12.0f, nullptr, 0.95f, false);

    if (Capabilities.IsEmpty())
    {
        DrawText(TEXT("NO INSTALLED CAPABILITIES"), MutedColor,
            SystemPanelX + 14.0f, ExpandedY + 48.0f, nullptr, 0.9f, false);
        return;
    }

    SelectedCapabilityIndex = FMath::Clamp(SelectedCapabilityIndex, 0, Capabilities.Num() - 1);

    float Y = ExpandedY + 44.0f;
    for (int32 Index = 0; Index < Capabilities.Num(); ++Index)
    {
        const FEverwardProbeCapability& Capability = Capabilities[Index];
        const FString Prefix = Index == SelectedCapabilityIndex ? TEXT("> ") : TEXT("  ");
        DrawText(
            FString::Printf(TEXT("%s%s  %.0f%%  [%s]"),
                *Prefix,
                *Capability.DisplayName,
                Capability.IntegrityFraction * 100.0,
                *CapabilityState(Capability)),
            Index == SelectedCapabilityIndex ? TextColor : MutedColor,
            SystemPanelX + 14.0f, Y, nullptr, 0.9f, false);
        Y += 24.0f;
    }

    const FEverwardProbeCapability& Selected = Capabilities[SelectedCapabilityIndex];
    Y += 8.0f;
    DrawText(Selected.Description, TextColor, SystemPanelX + 14.0f, Y, nullptr, 0.82f, false);
    Y += 26.0f;
    DrawText(
        FString::Printf(TEXT("POWER %.0f W   //   INTEGRITY %.0f%%   [PGUP/PGDN ADJUST]"),
            Selected.AllocatedPowerWatts,
            Selected.IntegrityFraction * 100.0),
        MutedColor, SystemPanelX + 14.0f, Y, nullptr, 0.82f, false);
    Y += 22.0f;
    DrawText(
        Selected.MinimumOperatingPowerWatts > 0.0
            ? FString::Printf(TEXT("STATUS %s   //   MIN %.0f W"),
                *Selected.StatusReason, Selected.MinimumOperatingPowerWatts)
            : FString::Printf(TEXT("STATUS %s"), *Selected.StatusReason),
        Selected.bOperational && Selected.bAvailable ? MutedColor : AlertColor,
        SystemPanelX + 14.0f, Y, nullptr, 0.74f, false);
    Y += 22.0f;
    DrawText(
        FString::Printf(TEXT("MANUAL CONTROL  %s   //   AUTOMATION API  %s"),
            Selected.bSupportsManualControl ? TEXT("YES") : TEXT("N/A"),
            Selected.bSupportsAutomation ? TEXT("YES") : TEXT("N/A")),
        MutedColor, SystemPanelX + 14.0f, Y, nullptr, 0.82f, false);
    Y += 28.0f;

    if (Selected.CapabilityId == FName(TEXT("sensors")))
    {
        DrawText(TEXT("[ENTER] START SCAN   [BACKSPACE] CANCEL"), TextColor,
            SystemPanelX + 14.0f, Y, nullptr, 0.8f, false);
        Y += 22.0f;
        DrawText(
            Telemetry.bIsScanning
                ? FString::Printf(TEXT("ACTIVE: %s  %.1f s"), *Telemetry.ActiveScanTargetId, Telemetry.ScanRemainingSeconds)
                : TEXT("ACTIVE: NONE"),
            MutedColor, SystemPanelX + 14.0f, Y, nullptr, 0.8f, false);
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
                FString::Printf(TEXT("CONFIDENCE %.0f%%  //  ACQUIRED T+%.1fs"),
                    LastScanConfidence * 100.0, LastScanCompletedAtSeconds),
                MutedColor, SystemPanelX + 14.0f, Y, nullptr, 0.70f, false);
            Y += 24.0f;
        }
    }
    else if (Selected.CapabilityId == FName(TEXT("propulsion")))
    {
        DrawText(TEXT("[W/S] X  [A/D] Y  [Q/E] Z  [SPACE] GLOBAL BRAKE"), TextColor,
            SystemPanelX + 14.0f, Y, nullptr, 0.72f, false);
        Y += 22.0f;
        DrawText(
            FString::Printf(TEXT("WORLD VECTOR [%.2f, %.2f, %.2f] m/s"),
                Telemetry.VelocityMetersPerSecond.X,
                Telemetry.VelocityMetersPerSecond.Y,
                Telemetry.VelocityMetersPerSecond.Z),
            MutedColor, SystemPanelX + 14.0f, Y, nullptr, 0.8f, false);
        Y += 24.0f;
    }
    else if (Selected.CapabilityId == FName(TEXT("computation")))
    {
        DrawText(TEXT("[ENTER] INSTALL BASIC SURVIVAL   [BACKSPACE] CLEAR"), TextColor,
            SystemPanelX + 14.0f, Y, nullptr, 0.76f, false);
        Y += 22.0f;
        DrawText(
            PolicyStatus.bInstalled
                ? FString::Printf(TEXT("POLICY: %s  //  %d RULES"), *PolicyStatus.PolicyId, PolicyStatus.RuleCount)
                : TEXT("POLICY: NONE"),
            MutedColor, SystemPanelX + 14.0f, Y, nullptr, 0.8f, false);
        Y += 22.0f;
        DrawText(
            PolicyStatus.bExecutorAvailable
                ? TEXT("EXECUTOR: RUNNING")
                : FString::Printf(TEXT("EXECUTOR: NEED >= %.0f W COMPUTE"), PolicyStatus.MinimumComputationPowerWatts),
            PolicyStatus.bExecutorAvailable ? TextColor : AlertColor,
            SystemPanelX + 14.0f, Y, nullptr, 0.8f, false);
        Y += 22.0f;
        if (AutomationNotice.Sequence > 0)
        {
            DrawText(TEXT("LAST AUTOMATION"), TextColor, SystemPanelX + 14.0f, Y, nullptr, 0.72f, false);
            Y += 18.0f;
            DrawText(
                AutomationNotice.Detail,
                AutomationNotice.bRejected ? AlertColor : MutedColor,
                SystemPanelX + 14.0f, Y, nullptr, 0.62f, false);
            Y += 22.0f;
        }
    }

    if (LastCommand.Sequence > 0)
    {
        DrawText(
            FString::Printf(TEXT("CMD %s: %s"),
                LastCommand.bAccepted ? TEXT("OK") : TEXT("REJECTED"),
                *LastCommand.Detail),
            LastCommand.bAccepted ? TextColor : AlertColor,
            SystemPanelX + 14.0f, ExpandedY + ExpandedHeight - 52.0f, nullptr, 0.75f, false);
    }

    DrawText(TEXT("[ / ] SELECT SYSTEM   //   MOUSE LOOK   WHEEL ZOOM"), MutedColor,
        SystemPanelX + 14.0f, ExpandedY + ExpandedHeight - 26.0f, nullptr, 0.68f, false);
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

void AEverwardHUD::ToggleManipulatorPanel()
{
    bManipulatorPanelExpanded = !bManipulatorPanelExpanded;
}

void AEverwardHUD::CycleSelectedManipulatorArm()
{
    SelectedManipulatorArmIndex = SelectedManipulatorArmIndex == 0 ? 1 : 0;
}

void AEverwardHUD::SelectManipulatorJointShoulder()
{
    SelectedManipulatorJointIndex = 0;
}

void AEverwardHUD::SelectManipulatorJointElbow()
{
    SelectedManipulatorJointIndex = 1;
}

void AEverwardHUD::SelectManipulatorJointWrist()
{
    SelectedManipulatorJointIndex = 2;
}

bool AEverwardHUD::IsManipulatorPanelExpanded() const
{
    return bManipulatorPanelExpanded;
}

int32 AEverwardHUD::GetSelectedManipulatorArmIndex() const
{
    return SelectedManipulatorArmIndex;
}

int32 AEverwardHUD::GetSelectedManipulatorJointIndex() const
{
    return SelectedManipulatorJointIndex;
}
