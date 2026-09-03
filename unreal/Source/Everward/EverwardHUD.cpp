#include "EverwardHUD.h"

#include "CanvasItem.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EverwardProbePawn.h"
#include "ProbeSimulationAdapter.h"

namespace
{
constexpr float HudReferenceWidth = 1920.0f;
constexpr float HudReferenceHeight = 1080.0f;
constexpr float MinimumHudTextScale = 1.05f;

struct FControlReferenceLine
{
    const TCHAR* Key;
    const TCHAR* Action;
};

float ResolveHudScale(const UCanvas* Canvas)
{
    if (Canvas == nullptr)
    {
        return 1.0f;
    }

    const float WidthScale = Canvas->ClipX / HudReferenceWidth;
    const float HeightScale = Canvas->ClipY / HudReferenceHeight;
    return FMath::Clamp(FMath::Min(WidthScale, HeightScale), 0.80f, 1.75f);
}

float ReadableTextScale(float HudScale, float RelativeScale = 1.0f)
{
    // The previous HUD combined the engine small font with scales as low as
    // 0.56, producing approximately 8-11 px text in the 2048x1200 Product
    // Reality capture. The medium font plus this floor keeps every live label
    // human-readable while still allowing stronger heading hierarchy.
    return FMath::Max(MinimumHudTextScale, HudScale * RelativeScale * 1.15f);
}

FString CompactStatusReason(const FString& StatusReason)
{
    int32 SeparatorIndex = INDEX_NONE;
    if (StatusReason.FindChar(TEXT('/'), SeparatorIndex))
    {
        return StatusReason.Left(SeparatorIndex).TrimStartAndEnd();
    }
    return StatusReason;
}

FString TruncatedPanelLine(const FString& Text, int32 MaximumCharacters = 54)
{
    if (Text.Len() <= MaximumCharacters)
    {
        return Text;
    }
    return Text.Left(FMath::Max(0, MaximumCharacters - 3)) + TEXT("...");
}

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

    FString GraspSuffix;
    if (Arm.bTargetGrasped)
    {
        GraspSuffix = FString::Printf(TEXT("  //  HOLDING %s"), *Arm.GraspedTargetId);
    }

    return FString::Printf(
        TEXT("%s ARM  %s%s%s"),
        *Label,
        *StateText,
        Arm.bToolAttached ? TEXT("  //  TOOL") : TEXT(""),
        *GraspSuffix);
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

// Slice 7 "align a manipulator" minimum interaction: a plain read-only view
// over UProbeSimulationAdapter::GetManipulatorReachStatus() for whichever arm
// the manipulator page currently has selected. bHasResult is false whenever
// the authoritative accessor fails closed (arm not fully deployed, or no
// longer has a valid pose to report) -- that reads as a muted explanation
// rather than a fabricated distance.
FString ManipulatorReachLine(const FEverwardManipulatorReachStatus& Reach)
{
    if (!Reach.bHasResult)
    {
        return TEXT("REACH    ARM NOT DEPLOYED / NO VALID POSE");
    }
    return Reach.bInReach
        ? FString::Printf(TEXT("REACH    IN REACH // %.2f M TO SURFACE"), Reach.WristRangeToSurfaceMeters)
        : FString::Printf(TEXT("REACH    OUT OF REACH // %.2f M REMAINING"), Reach.RemainingDistanceMeters);
}
}

void AEverwardHUD::DrawControlsReference()
{
    if (Canvas == nullptr)
    {
        return;
    }

    const float HudScale = ResolveHudScale(Canvas);
    const auto S = [HudScale](float Value) { return Value * HudScale; };
    UFont* const HudFont = GEngine != nullptr ? GEngine->GetMediumFont() : nullptr;

    const FLinearColor BackdropColor(0.003f, 0.008f, 0.012f, 0.94f);
    const FLinearColor PanelColor(0.015f, 0.035f, 0.045f, 0.96f);
    const FLinearColor ColumnColor(0.025f, 0.060f, 0.075f, 0.90f);
    const FLinearColor TextColor(0.90f, 0.96f, 0.98f, 1.0f);
    const FLinearColor MutedColor(0.64f, 0.75f, 0.80f, 1.0f);
    const FLinearColor KeyColor(0.34f, 0.90f, 1.0f, 1.0f);

    DrawRect(BackdropColor, 0.0f, 0.0f, Canvas->ClipX, Canvas->ClipY);

    const float OuterMargin = S(34.0f);
    const float PanelX = OuterMargin;
    const float PanelY = OuterMargin;
    const float PanelWidth = Canvas->ClipX - OuterMargin * 2.0f;
    const float PanelHeight = Canvas->ClipY - OuterMargin * 2.0f;
    DrawRect(PanelColor, PanelX, PanelY, PanelWidth, PanelHeight);

    DrawText(TEXT("CONTROLS // PRIME GENERATION 1"), TextColor,
        PanelX + S(28.0f), PanelY + S(20.0f), HudFont, ReadableTextScale(HudScale, 1.28f), false);
    DrawText(TEXT("Operate the probe by task. Immediate state stays on the live HUD; this page teaches every current binding."),
        MutedColor, PanelX + S(28.0f), PanelY + S(55.0f), HudFont, ReadableTextScale(HudScale, 0.90f), false);

    const FControlReferenceLine FlightControls[] = {
        {TEXT("W / S"), TEXT("FORWARD / REVERSE VELOCITY")},
        {TEXT("A / D"), TEXT("LEFT / RIGHT VELOCITY")},
        {TEXT("Q / E"), TEXT("DOWN / UP VELOCITY")},
        {TEXT("I / K"), TEXT("PITCH UP / DOWN")},
        {TEXT("J / L"), TEXT("YAW LEFT / RIGHT")},
        {TEXT("U / O"), TEXT("ROLL LEFT / RIGHT")},
        {TEXT("SPACE"), TEXT("FULL STOP")},
        {TEXT("R"), TEXT("CAMERA-ALIGNED RIGHTING")},
        {TEXT("MOUSE"), TEXT("ORBIT CAMERA // WHEEL ZOOM")},
    };
    const FControlReferenceLine SystemControls[] = {
        {TEXT("TAB"), TEXT("OPEN / CLOSE SYSTEMS")},
        {TEXT("[ / ]"), TEXT("SELECT INSTALLED SYSTEM")},
        {TEXT("PGUP / PGDN"), TEXT("ADJUST SYSTEM POWER")},
        {TEXT("ENTER"), TEXT("PRIMARY SYSTEM ACTION")},
        {TEXT("BACKSPACE"), TEXT("CANCEL / CLEAR ACTION")},
        {TEXT("SENSORS"), TEXT("ENTER STARTS SCAN")},
        {TEXT("COMPUTE"), TEXT("ENTER INSTALLS POLICY")},
        {TEXT("F1"), TEXT("OPEN / CLOSE THIS REFERENCE")},
        {TEXT("F5"), TEXT("SAVE PROBE STATE")},
        {TEXT("F6"), TEXT("LOAD PROBE STATE")},
    };
    const FControlReferenceLine ManipulatorControls[] = {
        {TEXT("M"), TEXT("OPEN / CLOSE MANIPULATOR")},
        {TEXT("1"), TEXT("DEPLOY / STOW PORT ARM")},
        {TEXT("2"), TEXT("DEPLOY / STOW STARBOARD ARM")},
        {TEXT("3"), TEXT("ATTACH / DETACH TOOL")},
        {TEXT("N"), TEXT("SELECT ARM")},
        {TEXT("4"), TEXT("SELECT SHOULDER")},
        {TEXT("5"), TEXT("SELECT ELBOW")},
        {TEXT("6"), TEXT("SELECT WRIST / TOOL")},
        {TEXT(", / ."), TEXT("ADJUST JOINT TARGET")},
        {TEXT("T"), TEXT("SELECT NEAREST PHYSICAL TARGET")},
        {TEXT("F"), TEXT("GRASP / RELEASE SELECTED TARGET")},
        {TEXT("G"), TEXT("MINE SURVEYED TARGET")},
    };

    const float ColumnGap = S(18.0f);
    const float ColumnY = PanelY + S(92.0f);
    const float ColumnWidth = (PanelWidth - S(56.0f) - ColumnGap * 2.0f) / 3.0f;
    const float ColumnHeight = PanelHeight - S(148.0f);
    const float FirstColumnX = PanelX + S(28.0f);

    const auto DrawControlColumn = [this, HudScale, HudFont, S, ColumnColor, TextColor, MutedColor, KeyColor,
                                    ColumnY, ColumnWidth, ColumnHeight](
        float ColumnX,
        const TCHAR* Heading,
        const FControlReferenceLine* Lines,
        int32 LineCount)
    {
        DrawRect(ColumnColor, ColumnX, ColumnY, ColumnWidth, ColumnHeight);
        DrawText(Heading, TextColor, ColumnX + S(20.0f), ColumnY + S(16.0f),
            HudFont, ReadableTextScale(HudScale, 1.10f), false);

        const float RowHeight = FMath::Min(S(47.0f), (ColumnHeight - S(70.0f)) / FMath::Max(LineCount, 1));
        float RowY = ColumnY + S(62.0f);
        for (int32 Index = 0; Index < LineCount; ++Index)
        {
            DrawText(Lines[Index].Key, KeyColor, ColumnX + S(20.0f), RowY,
                HudFont, ReadableTextScale(HudScale, 0.95f), false);
            DrawText(Lines[Index].Action, Index % 2 == 0 ? TextColor : MutedColor,
                ColumnX + S(132.0f), RowY, HudFont, ReadableTextScale(HudScale, 0.88f), false);
            RowY += RowHeight;
        }
    };

    DrawControlColumn(FirstColumnX, TEXT("FLIGHT + CAMERA"), FlightControls, UE_ARRAY_COUNT(FlightControls));
    DrawControlColumn(FirstColumnX + ColumnWidth + ColumnGap, TEXT("SYSTEMS"),
        SystemControls, UE_ARRAY_COUNT(SystemControls));
    DrawControlColumn(FirstColumnX + (ColumnWidth + ColumnGap) * 2.0f, TEXT("MANIPULATOR + MINING"),
        ManipulatorControls, UE_ARRAY_COUNT(ManipulatorControls));

    DrawText(TEXT("[F1] RETURN TO LIVE HUD"), KeyColor,
        PanelX + S(28.0f), PanelY + PanelHeight - S(42.0f),
        HudFont, ReadableTextScale(HudScale, 1.05f), false);
}

void AEverwardHUD::DrawHUD()
{
    Super::DrawHUD();

    if (Canvas == nullptr)
    {
        return;
    }

    if (bControlsReferenceVisible)
    {
        DrawControlsReference();
        return;
    }

    if (PlayerOwner == nullptr)
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
    const FEverwardTargetSelectionStatus TargetSelection = Adapter->GetSelectedTargetStatus();
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

    const float HudScale = ResolveHudScale(Canvas);
    const auto S = [HudScale](float Value) { return Value * HudScale; };
    UFont* const HudFont = GEngine != nullptr ? GEngine->GetMediumFont() : nullptr;
    const float Margin = S(24.0f);
    const float LineHeight = S(32.0f);
    const float PanelWidth = S(500.0f);
    const FLinearColor PanelColor(0.015f, 0.025f, 0.035f, 0.88f);
    const FLinearColor TextColor(0.90f, 0.96f, 0.98f, 1.0f);
    const FLinearColor MutedColor(0.64f, 0.75f, 0.80f, 1.0f);
    const FLinearColor AlertColor(1.0f, 0.36f, 0.18f, 1.0f);

    const float TelemetryHeight = S(62.0f) + LineHeight * 9.0f;
    const float TelemetryY = Canvas->ClipY - Margin - TelemetryHeight;
    DrawRect(PanelColor, Margin, TelemetryY, PanelWidth, TelemetryHeight);
    DrawText(
        FString::Printf(TEXT("%s  //  GEN %d"), *Telemetry.ProbeId, Telemetry.Generation),
        TextColor, Margin + S(16.0f), TelemetryY + S(12.0f), HudFont, ReadableTextScale(HudScale, 1.08f), false);
    DrawText(
        FString::Printf(TEXT("ENERGY  %s"), *PercentString(Telemetry.StoredEnergyJoules, Telemetry.EnergyCapacityJoules)),
        TextColor, Margin + S(16.0f), TelemetryY + S(48.0f), HudFont, ReadableTextScale(HudScale), false);
    DrawText(
        FString::Printf(TEXT("POWER   %.0f / %.0f W"), Telemetry.TotalPowerAllocatedWatts, Telemetry.PowerCapacityWatts),
        TextColor, Margin + S(16.0f), TelemetryY + S(48.0f) + LineHeight, HudFont, ReadableTextScale(HudScale), false);
    DrawText(
        FString::Printf(TEXT("THERMAL %.1f K"), Telemetry.TemperatureKelvin),
        TextColor, Margin + S(16.0f), TelemetryY + S(48.0f) + LineHeight * 2.0f, HudFont, ReadableTextScale(HudScale), false);
    DrawText(
        FString::Printf(TEXT("STORAGE %.1f / %.1f KG  (%s)"),
            Telemetry.StorageUsedKilograms,
            Telemetry.StorageCapacityKilograms,
            *PercentString(Telemetry.StorageUsedKilograms, Telemetry.StorageCapacityKilograms)),
        TextColor, Margin + S(16.0f), TelemetryY + S(48.0f) + LineHeight * 3.0f,
        HudFont, ReadableTextScale(HudScale), false);
    DrawText(
        FString::Printf(TEXT("VELOCITY %.2f m/s"), Telemetry.VelocityMetersPerSecond.Size()),
        TextColor, Margin + S(16.0f), TelemetryY + S(48.0f) + LineHeight * 4.0f,
        HudFont, ReadableTextScale(HudScale), false);
    // Slice 7 foundation: authoritative range/closing speed for whatever the
    // player has selected with T, over the same registered-body list the
    // swept contact solver already consumes. No selection reads as a plain
    // muted prompt rather than fabricated zeros.
    DrawText(
        TargetSelection.bHasSelection
            ? FString::Printf(TEXT("TARGET  %s // %.1f M // CLOSING %.2f M/S"),
                *TargetSelection.TargetId, TargetSelection.SurfaceRangeMeters, TargetSelection.ClosingSpeedMetersPerSecond)
            : FString(TEXT("TARGET  NONE SELECTED // [T] SELECT NEAREST")),
        TargetSelection.bHasSelection ? TextColor : MutedColor,
        Margin + S(16.0f), TelemetryY + S(48.0f) + LineHeight * 5.0f,
        HudFont, ReadableTextScale(HudScale), false);
    DrawText(
        FString::Printf(TEXT("SIM %.1f s"), Telemetry.SimulationTimeSeconds),
        MutedColor, Margin + S(16.0f), TelemetryY + S(48.0f) + LineHeight * 6.0f,
        HudFont, ReadableTextScale(HudScale, 0.92f), false);
    for (int32 ArmIndex = 0; ArmIndex < ManipulatorArms.Num(); ++ArmIndex)
    {
        const FEverwardManipulatorArmState& Arm = ManipulatorArms[ArmIndex];
        const FString Label = Arm.ArmId == EEverwardManipulatorArmId::Port ? TEXT("PORT") : TEXT("STBD");
        DrawText(
            ManipulatorArmLine(Label, Arm),
            Arm.bIsDeployed || Arm.bIsDeploying || Arm.bIsStowing ? TextColor : MutedColor,
            Margin + S(16.0f),
            TelemetryY + S(48.0f) + LineHeight * (7.0f + ArmIndex),
            HudFont,
            ReadableTextScale(HudScale, 0.94f),
            false);
    }

    // Dedicated manipulator HUD page (Slice 6 joint-articulation follow-up).
    // Rendered above the compact telemetry panel so it never overlaps the
    // always-visible PORT/STBD status lines it elaborates on.
    if (bManipulatorPanelExpanded && !ManipulatorArms.IsEmpty())
    {
        SelectedManipulatorArmIndex = FMath::Clamp(SelectedManipulatorArmIndex, 0, ManipulatorArms.Num() - 1);
        SelectedManipulatorJointIndex = FMath::Clamp(SelectedManipulatorJointIndex, 0, 2);

        // Slice 7 "align a manipulator" minimum interaction: a single reach
        // row for whichever arm the manipulator page has selected, shown
        // only once a physical target is actually selected -- with no
        // selection there is nothing authoritative to report range against.
        const bool bShowReachRow = TargetSelection.bHasSelection;
        const FEverwardManipulatorReachStatus ReachStatus = bShowReachRow
            ? Adapter->GetManipulatorReachStatus(ManipulatorArms[SelectedManipulatorArmIndex].ArmId)
            : FEverwardManipulatorReachStatus();

        int32 ManipulatorRows = 0;
        for (const FEverwardManipulatorArmState& Arm : ManipulatorArms)
        {
            ManipulatorRows += Arm.bIsDeployed ? 4 : 2;
        }
        if (bShowReachRow)
        {
            ManipulatorRows += 1;
        }
        constexpr int32 ManipulatorFooterRows = 6;
        const float PanelHeight = S(54.0f) + LineHeight * (ManipulatorRows + ManipulatorFooterRows);
        const float PanelY = TelemetryY - S(12.0f) - PanelHeight;
        DrawRect(PanelColor, Margin, PanelY, PanelWidth, PanelHeight);
        DrawText(TEXT("MANIPULATOR CONTROL   [M CLOSE]"), TextColor,
            Margin + S(16.0f), PanelY + S(12.0f), HudFont, ReadableTextScale(HudScale, 1.04f), false);

        float ArmY = PanelY + S(50.0f);
        for (int32 ArmIndex = 0; ArmIndex < ManipulatorArms.Num(); ++ArmIndex)
        {
            const FEverwardManipulatorArmState& Arm = ManipulatorArms[ArmIndex];
            const bool bArmSelected = ArmIndex == SelectedManipulatorArmIndex;
            const FString Label = Arm.ArmId == EEverwardManipulatorArmId::Port ? TEXT("PORT") : TEXT("STBD");
            DrawText(
                FString::Printf(TEXT("%s%s"), bArmSelected ? TEXT("> ") : TEXT("  "), *ManipulatorArmLine(Label, Arm)),
                bArmSelected ? TextColor : MutedColor,
                Margin + S(16.0f), ArmY, HudFont, ReadableTextScale(HudScale, 0.96f), false);
            ArmY += LineHeight;

            if (!Arm.bIsDeployed)
            {
                DrawText(TEXT("    DEPLOY ARM TO COMMAND JOINTS"), MutedColor,
                    Margin + S(16.0f), ArmY, HudFont, ReadableTextScale(HudScale, 0.90f), false);
                ArmY += LineHeight;
                continue;
            }

            for (int32 JointIndex = 0; JointIndex < 3; ++JointIndex)
            {
                const bool bJointSelected = bArmSelected && JointIndex == SelectedManipulatorJointIndex;
                DrawText(
                    FString::Printf(TEXT("  %s%s"), bJointSelected ? TEXT("> ") : TEXT("  "), *ManipulatorJointLine(JointIndex, Arm)),
                    bJointSelected ? TextColor : MutedColor,
                    Margin + S(16.0f), ArmY, HudFont, ReadableTextScale(HudScale, 0.90f), false);
                ArmY += LineHeight;
            }
        }

        if (bShowReachRow)
        {
            DrawText(
                ManipulatorReachLine(ReachStatus),
                ReachStatus.bHasResult && ReachStatus.bInReach ? TextColor : MutedColor,
                Margin + S(16.0f), ArmY, HudFont, ReadableTextScale(HudScale, 0.90f), false);
            ArmY += LineHeight;
        }

        const float FooterY = PanelY + PanelHeight - LineHeight * ManipulatorFooterRows;
        DrawText(TEXT("[1] PORT ARM // DEPLOY / STOW"), TextColor,
            Margin + S(16.0f), FooterY, HudFont, ReadableTextScale(HudScale, 0.90f), false);
        DrawText(TEXT("[2] STARBOARD ARM // DEPLOY / STOW"), TextColor,
            Margin + S(16.0f), FooterY + LineHeight, HudFont, ReadableTextScale(HudScale, 0.86f), false);
        DrawText(TEXT("[3] TOOL // ATTACH / DETACH   [N] SELECT ARM"), MutedColor,
            Margin + S(16.0f), FooterY + LineHeight * 2.0f, HudFont, ReadableTextScale(HudScale, 0.86f), false);
        DrawText(TEXT("[4/5/6] SELECT JOINT   [,] / [.] ADJUST TARGET"), MutedColor,
            Margin + S(16.0f), FooterY + LineHeight * 3.0f, HudFont, ReadableTextScale(HudScale, 0.86f), false);
        DrawText(TEXT("[F] GRASP / RELEASE TARGET"), MutedColor,
            Margin + S(16.0f), FooterY + LineHeight * 4.0f, HudFont, ReadableTextScale(HudScale, 0.86f), false);
        DrawText(TEXT("[G] MINE SURVEYED TARGET   [F1] ALL CONTROLS"), MutedColor,
            Margin + S(16.0f), FooterY + LineHeight * 5.0f, HudFont, ReadableTextScale(HudScale, 0.86f), false);
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
        DrawText(AlertText, AlertColor, Margin, AlertY, HudFont, ReadableTextScale(HudScale, 1.25f), false);
        AlertY += S(36.0f);
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
            HudFont,
            ReadableTextScale(HudScale, 1.0f),
            false);
        AlertY += LineHeight;

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
                HudFont,
                ReadableTextScale(HudScale, 0.92f),
                false);
            AlertY += LineHeight;
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
            HudFont,
            ReadableTextScale(HudScale, 0.90f),
            false);
        AlertY += LineHeight;
    }

    if (LastCommand.Sequence > 0 && !LastCommand.bAccepted && WorldNow <= CommandBannerExpiresAtWorldSeconds)
    {
        DrawText(
            FString::Printf(TEXT("COMMAND REJECTED // %s"), *LastCommand.Detail),
            AlertColor, Margin, AlertY, HudFont, ReadableTextScale(HudScale, 1.0f), false);
        AlertY += LineHeight;
    }

    const bool bRejectedCommandAlreadyVisible =
        LastCommand.Sequence > 0 &&
        !LastCommand.bAccepted &&
        AutomationNotice.bRejected &&
        LastCommand.Detail.Equals(AutomationNotice.Detail) &&
        WorldNow <= CommandBannerExpiresAtWorldSeconds;
    if (AutomationNotice.Sequence > 0 &&
        WorldNow <= AutomationBannerExpiresAtWorldSeconds &&
        !bRejectedCommandAlreadyVisible)
    {
        DrawText(
            AutomationNotice.Detail,
            AutomationNotice.bRejected ? AlertColor : TextColor,
            Margin, AlertY, HudFont, ReadableTextScale(HudScale, 0.98f), false);
        AlertY += LineHeight;
    }

    if (Telemetry.bIsScanning)
    {
        DrawText(
            FString::Printf(
                TEXT("SCAN  %s  //  %.1f s REMAINING"),
                *Telemetry.ActiveScanTargetId,
                Telemetry.ScanRemainingSeconds),
            TextColor, Margin, AlertY, HudFont, ReadableTextScale(HudScale, 1.0f), false);
    }
    else if (bHasScanDiscovery)
    {
        DrawText(
            FString::Printf(TEXT("SCAN COMPLETE  //  %s  //  DISCOVERY STORED"), *LastScanTargetId),
            TextColor, Margin, AlertY, HudFont, ReadableTextScale(HudScale, 1.0f), false);
    }

    const double SpeedMetersPerSecond = Telemetry.VelocityMetersPerSecond.Size();
    if (SpeedMetersPerSecond > 0.01)
    {
        const FVector LocalVelocity = Probe->GetActorTransform().InverseTransformVectorNoScale(
            Telemetry.VelocityMetersPerSecond);
        const float FlightWidth = S(560.0f);
        const float FlightHeight = S(96.0f);
        const float FlightX = (Canvas->ClipX - FlightWidth) * 0.5f;
        const float FlightY = Margin;
        DrawRect(PanelColor, FlightX, FlightY, FlightWidth, FlightHeight);
        DrawText(
            FString::Printf(TEXT("FLIGHT  %.2f m/s   [SPACE] FULL STOP"), SpeedMetersPerSecond),
            TextColor, FlightX + S(16.0f), FlightY + S(12.0f),
            HudFont, ReadableTextScale(HudScale, 1.04f), false);
        DrawText(
            FString::Printf(
                TEXT("FWD %+0.2f   RIGHT %+0.2f   UP %+0.2f m/s"),
                LocalVelocity.X,
                LocalVelocity.Y,
                LocalVelocity.Z),
            MutedColor, FlightX + S(16.0f), FlightY + S(52.0f),
            HudFont, ReadableTextScale(HudScale, 0.92f), false);
    }

    const float HelpWidth = S(650.0f);
    const float HelpHeight = S(44.0f);
    const float HelpX = (Canvas->ClipX - HelpWidth) * 0.5f;
    const float HelpY = Canvas->ClipY - Margin - HelpHeight;
    DrawRect(PanelColor, HelpX, HelpY, HelpWidth, HelpHeight);
    DrawText(TEXT("[F1] CONTROLS   [TAB] SYSTEMS   [M] MANIPULATOR"), TextColor,
        HelpX + S(18.0f), HelpY + S(10.0f), HudFont, ReadableTextScale(HudScale, 0.92f), false);

    const float SystemPanelX = Canvas->ClipX - Margin - PanelWidth;
    const float CompactHeaderHeight = S(54.0f);
    const float CompactRowHeight = S(64.0f);
    const float CompactHeight = CompactHeaderHeight + CompactRowHeight * FMath::Max(Capabilities.Num(), 1);
    const float CompactY = Canvas->ClipY - Margin - CompactHeight;

    if (!bSystemsExpanded)
    {
        DrawRect(PanelColor, SystemPanelX, CompactY, PanelWidth, CompactHeight);
        DrawText(
            FString::Printf(TEXT("SYSTEMS  %d INSTALLED   [TAB DETAILS]"), Capabilities.Num()),
            TextColor, SystemPanelX + S(16.0f), CompactY + S(12.0f),
            HudFont, ReadableTextScale(HudScale, 1.02f), false);

        if (Capabilities.IsEmpty())
        {
            DrawText(TEXT("NO INSTALLED CAPABILITIES"), MutedColor,
                SystemPanelX + S(16.0f), CompactY + CompactHeaderHeight,
                HudFont, ReadableTextScale(HudScale, 0.92f), false);
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
                    SystemPanelX + S(16.0f), CompactRowY,
                    HudFont, ReadableTextScale(HudScale, 0.94f), false);
                DrawText(
                    CompactStatusReason(Capability.StatusReason),
                    bHealthy ? MutedColor : AlertColor,
                    SystemPanelX + S(24.0f), CompactRowY + S(30.0f),
                    HudFont, ReadableTextScale(HudScale, 0.88f), false);
                CompactRowY += CompactRowHeight;
            }
        }
        return;
    }

    const float ExpandedHeight = S(720.0f);
    const float ExpandedY = Canvas->ClipY - Margin - ExpandedHeight;
    DrawRect(PanelColor, SystemPanelX, ExpandedY, PanelWidth, ExpandedHeight);
    DrawText(TEXT("SYSTEMS / CONTROL   [TAB CLOSE]"), TextColor,
        SystemPanelX + S(16.0f), ExpandedY + S(14.0f),
        HudFont, ReadableTextScale(HudScale, 1.06f), false);

    if (Capabilities.IsEmpty())
    {
        DrawText(TEXT("NO INSTALLED CAPABILITIES"), MutedColor,
            SystemPanelX + S(16.0f), ExpandedY + S(58.0f),
            HudFont, ReadableTextScale(HudScale, 0.94f), false);
        return;
    }

    SelectedCapabilityIndex = FMath::Clamp(SelectedCapabilityIndex, 0, Capabilities.Num() - 1);

    float Y = ExpandedY + S(56.0f);
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
            SystemPanelX + S(16.0f), Y, HudFont, ReadableTextScale(HudScale, 0.96f), false);
        Y += LineHeight;
    }

    const FEverwardProbeCapability& Selected = Capabilities[SelectedCapabilityIndex];
    Y += S(10.0f);
    DrawText(TruncatedPanelLine(Selected.Description, 50), TextColor,
        SystemPanelX + S(16.0f), Y, HudFont, ReadableTextScale(HudScale, 0.92f), false);
    Y += LineHeight;
    DrawText(
        FString::Printf(TEXT("POWER %.0f W   [PGUP/PGDN ADJUST]"), Selected.AllocatedPowerWatts),
        TextColor, SystemPanelX + S(16.0f), Y, HudFont, ReadableTextScale(HudScale, 0.92f), false);
    Y += LineHeight;
    DrawText(
        Selected.MinimumOperatingPowerWatts > 0.0
            ? FString::Printf(TEXT("INTEGRITY %.0f%%   //   MINIMUM %.0f W"),
                Selected.IntegrityFraction * 100.0, Selected.MinimumOperatingPowerWatts)
            : FString::Printf(TEXT("INTEGRITY %.0f%%"), Selected.IntegrityFraction * 100.0),
        Selected.bOperational && Selected.bAvailable ? MutedColor : AlertColor,
        SystemPanelX + S(16.0f), Y, HudFont, ReadableTextScale(HudScale, 0.90f), false);
    Y += LineHeight;
    DrawText(
        FString::Printf(TEXT("STATUS %s"), *TruncatedPanelLine(Selected.StatusReason, 48)),
        Selected.bOperational && Selected.bAvailable ? MutedColor : AlertColor,
        SystemPanelX + S(16.0f), Y, HudFont, ReadableTextScale(HudScale, 0.88f), false);
    Y += LineHeight;
    DrawText(
        FString::Printf(TEXT("MANUAL CONTROL %s   //   AUTOMATION API %s"),
            Selected.bSupportsManualControl ? TEXT("YES") : TEXT("N/A"),
            Selected.bSupportsAutomation ? TEXT("YES") : TEXT("N/A")),
        MutedColor, SystemPanelX + S(16.0f), Y, HudFont, ReadableTextScale(HudScale, 0.88f), false);
    Y += LineHeight + S(6.0f);

    if (Selected.CapabilityId == FName(TEXT("sensors")))
    {
        DrawText(TEXT("[ENTER] START SCAN   [BACKSPACE] CANCEL"), TextColor,
            SystemPanelX + S(16.0f), Y, HudFont, ReadableTextScale(HudScale, 0.92f), false);
        Y += LineHeight;
        DrawText(
            Telemetry.bIsScanning
                ? FString::Printf(TEXT("ACTIVE: %s  %.1f s"), *Telemetry.ActiveScanTargetId, Telemetry.ScanRemainingSeconds)
                : TEXT("ACTIVE: NONE"),
            MutedColor, SystemPanelX + S(16.0f), Y, HudFont, ReadableTextScale(HudScale, 0.90f), false);
        Y += LineHeight;

        if (bHasScanDiscovery)
        {
            DrawText(TEXT("LAST DISCOVERY"), TextColor, SystemPanelX + S(16.0f), Y,
                HudFont, ReadableTextScale(HudScale, 0.94f), false);
            Y += LineHeight;
            DrawText(LastScanTargetId, MutedColor, SystemPanelX + S(16.0f), Y,
                HudFont, ReadableTextScale(HudScale, 0.88f), false);
            Y += LineHeight;
            DrawText(LastScanObjectClass, TextColor, SystemPanelX + S(16.0f), Y,
                HudFont, ReadableTextScale(HudScale, 0.88f), false);
            Y += LineHeight;
            DrawText(LastScanComposition, MutedColor, SystemPanelX + S(16.0f), Y,
                HudFont, ReadableTextScale(HudScale, 0.86f), false);
            Y += LineHeight;
            DrawText(
                FString::Printf(TEXT("CONFIDENCE %.0f%%  //  ACQUIRED T+%.1fs"),
                    LastScanConfidence * 100.0, LastScanCompletedAtSeconds),
                MutedColor, SystemPanelX + S(16.0f), Y,
                HudFont, ReadableTextScale(HudScale, 0.86f), false);
            Y += LineHeight;
        }
    }
    else if (Selected.CapabilityId == FName(TEXT("propulsion")))
    {
        DrawText(TEXT("[W/S] FORWARD / REVERSE   [A/D] LEFT / RIGHT"), TextColor,
            SystemPanelX + S(16.0f), Y, HudFont, ReadableTextScale(HudScale, 0.88f), false);
        Y += LineHeight;
        DrawText(TEXT("[Q/E] DOWN / UP   [SPACE] FULL STOP"), TextColor,
            SystemPanelX + S(16.0f), Y, HudFont, ReadableTextScale(HudScale, 0.88f), false);
        Y += LineHeight;
        DrawText(TEXT("[I/K] PITCH   [J/L] YAW   [U/O] ROLL   [R] RIGHT"), MutedColor,
            SystemPanelX + S(16.0f), Y, HudFont, ReadableTextScale(HudScale, 0.86f), false);
        Y += LineHeight;
        DrawText(
            FString::Printf(TEXT("WORLD VECTOR [%.2f, %.2f, %.2f] m/s"),
                Telemetry.VelocityMetersPerSecond.X,
                Telemetry.VelocityMetersPerSecond.Y,
                Telemetry.VelocityMetersPerSecond.Z),
            MutedColor, SystemPanelX + S(16.0f), Y, HudFont, ReadableTextScale(HudScale, 0.88f), false);
        Y += LineHeight;
    }
    else if (Selected.CapabilityId == FName(TEXT("computation")))
    {
        DrawText(TEXT("[ENTER] INSTALL BASIC SURVIVAL"), TextColor,
            SystemPanelX + S(16.0f), Y, HudFont, ReadableTextScale(HudScale, 0.90f), false);
        Y += LineHeight;
        DrawText(TEXT("[BACKSPACE] CLEAR POLICY"), TextColor,
            SystemPanelX + S(16.0f), Y, HudFont, ReadableTextScale(HudScale, 0.90f), false);
        Y += LineHeight;
        DrawText(
            PolicyStatus.bInstalled
                ? FString::Printf(TEXT("POLICY: %s  //  %d RULES"), *PolicyStatus.PolicyId, PolicyStatus.RuleCount)
                : TEXT("POLICY: NONE"),
            MutedColor, SystemPanelX + S(16.0f), Y, HudFont, ReadableTextScale(HudScale, 0.88f), false);
        Y += LineHeight;
        DrawText(
            PolicyStatus.bExecutorAvailable
                ? TEXT("EXECUTOR: RUNNING")
                : FString::Printf(TEXT("EXECUTOR: NEED >= %.0f W COMPUTE"), PolicyStatus.MinimumComputationPowerWatts),
            PolicyStatus.bExecutorAvailable ? TextColor : AlertColor,
            SystemPanelX + S(16.0f), Y, HudFont, ReadableTextScale(HudScale, 0.88f), false);
        Y += LineHeight;
        if (AutomationNotice.Sequence > 0)
        {
            DrawText(TEXT("LAST AUTOMATION"), TextColor, SystemPanelX + S(16.0f), Y,
                HudFont, ReadableTextScale(HudScale, 0.90f), false);
            Y += LineHeight;
            DrawText(
                TruncatedPanelLine(AutomationNotice.Detail, 48),
                AutomationNotice.bRejected ? AlertColor : MutedColor,
                SystemPanelX + S(16.0f), Y, HudFont, ReadableTextScale(HudScale, 0.86f), false);
            Y += LineHeight;
        }
    }

    if (LastCommand.Sequence > 0)
    {
        DrawText(
            TruncatedPanelLine(FString::Printf(TEXT("CMD %s: %s"),
                LastCommand.bAccepted ? TEXT("OK") : TEXT("REJECTED"),
                *LastCommand.Detail), 50),
            LastCommand.bAccepted ? TextColor : AlertColor,
            SystemPanelX + S(16.0f), ExpandedY + ExpandedHeight - S(72.0f),
            HudFont, ReadableTextScale(HudScale, 0.88f), false);
    }

    DrawText(TEXT("[ / ] SELECT SYSTEM   [F1] ALL CONTROLS"), MutedColor,
        SystemPanelX + S(16.0f), ExpandedY + ExpandedHeight - S(36.0f),
        HudFont, ReadableTextScale(HudScale, 0.86f), false);
}

void AEverwardHUD::ToggleControlsReference()
{
    bControlsReferenceVisible = !bControlsReferenceVisible;
}

bool AEverwardHUD::IsControlsReferenceVisible() const
{
    return bControlsReferenceVisible;
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
