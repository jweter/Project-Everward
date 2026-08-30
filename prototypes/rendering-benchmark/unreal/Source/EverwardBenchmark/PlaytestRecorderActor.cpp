#include "PlaytestRecorderActor.h"

#include "Engine/Engine.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "InputCoreTypes.h"
#include "Misc/App.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "UnrealClient.h"

APlaytestRecorderActor::APlaytestRecorderActor()
{
    PrimaryActorTick.bCanEverTick = true;
}

void APlaytestRecorderActor::BeginPlay()
{
    Super::BeginPlay();
    InitializeSession();
}

void APlaytestRecorderActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (!SessionDirectory.IsEmpty())
    {
        RecordPlaytestEvent(TEXT("session_ended"), FString::Printf(TEXT("reason=%d"), static_cast<int32>(EndPlayReason)));
        WriteSessionMetadata(TEXT("complete"), UtcNowIso());
    }

    Super::EndPlay(EndPlayReason);
}

void APlaytestRecorderActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (SessionDirectory.IsEmpty())
    {
        return;
    }

    TelemetryAccumulator += DeltaSeconds;
    if (TelemetryAccumulator >= TelemetryIntervalSeconds)
    {
        AppendTelemetry(DeltaSeconds);
        TelemetryAccumulator = 0.0f;
    }

    if (APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
    {
        if (PlayerController->WasInputKeyJustPressed(EKeys::F9))
        {
            RecordIssueMarker(TEXT("F9 manual playtest marker"));
        }
    }
}

void APlaytestRecorderActor::InitializeSession()
{
    const FDateTime Now = FDateTime::UtcNow();
    SessionId = Now.ToString(TEXT("%Y%m%dT%H%M%SZ"));
    StartedUtc = Now.ToIso8601();
    SessionStartSeconds = FPlatformTime::Seconds();

    SessionDirectory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Playtests"), SessionId);
    EventsPath = FPaths::Combine(SessionDirectory, TEXT("events.jsonl"));
    TelemetryPath = FPaths::Combine(SessionDirectory, TEXT("telemetry.csv"));

    IFileManager::Get().MakeDirectory(*SessionDirectory, true);

    const FString Header = TEXT("timestamp_utc,elapsed_seconds,delta_seconds,fps,frame_ms,pawn_x,pawn_y,pawn_z,pawn_speed\n");
    FFileHelper::SaveStringToFile(Header, *TelemetryPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);

    WriteSessionMetadata(TEXT("running"));
    RecordPlaytestEvent(TEXT("session_started"), TEXT("non_blocking_validation=true"));

    UE_LOG(LogTemp, Display, TEXT("Everward playtest recorder started: %s"), *SessionDirectory);
}

void APlaytestRecorderActor::WriteSessionMetadata(const FString& Status, const FString& EndedUtc)
{
    TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
    Root->SetNumberField(TEXT("schema_version"), 1);
    Root->SetStringField(TEXT("session_id"), SessionId);
    Root->SetStringField(TEXT("status"), Status);
    Root->SetBoolField(TEXT("blocking_gate"), false);
    Root->SetStringField(TEXT("started_utc"), StartedUtc);
    Root->SetStringField(TEXT("ended_utc"), EndedUtc);
    Root->SetStringField(TEXT("project_name"), FApp::GetProjectName());
    Root->SetStringField(TEXT("engine_version"), FEngineVersion::Current().ToString());
    Root->SetStringField(TEXT("build_configuration"), LexToString(FApp::GetBuildConfiguration()));
    Root->SetStringField(TEXT("session_directory"), SessionDirectory);
    Root->SetNumberField(TEXT("issue_marker_count"), MarkerCount);

    FString Json;
    const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Json);
    FJsonSerializer::Serialize(Root, Writer);
    FFileHelper::SaveStringToFile(Json + LINE_TERMINATOR, *FPaths::Combine(SessionDirectory, TEXT("session.json")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}

void APlaytestRecorderActor::RecordPlaytestEvent(const FString& EventName, const FString& Details)
{
    if (EventsPath.IsEmpty())
    {
        return;
    }

    TSharedRef<FJsonObject> Event = MakeShared<FJsonObject>();
    Event->SetStringField(TEXT("timestamp_utc"), UtcNowIso());
    Event->SetNumberField(TEXT("elapsed_seconds"), FPlatformTime::Seconds() - SessionStartSeconds);
    Event->SetStringField(TEXT("event_type"), EventName);
    Event->SetStringField(TEXT("details"), Details);

    FString Json;
    const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Json);
    FJsonSerializer::Serialize(Event, Writer);
    FFileHelper::SaveStringToFile(Json + LINE_TERMINATOR, *EventsPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM, &IFileManager::Get(), FILEWRITE_Append);
}

void APlaytestRecorderActor::RecordIssueMarker(const FString& Note)
{
    ++MarkerCount;
    RecordPlaytestEvent(TEXT("issue_marker"), FString::Printf(TEXT("marker=%d;note=%s"), MarkerCount, *Note));
    CaptureIssueScreenshot();
    WriteSessionMetadata(TEXT("running"));

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Yellow, FString::Printf(TEXT("Playtest marker #%d captured"), MarkerCount));
    }
}

void APlaytestRecorderActor::CaptureIssueScreenshot()
{
    const FString ScreenshotDirectory = FPaths::Combine(SessionDirectory, TEXT("screenshots"));
    IFileManager::Get().MakeDirectory(*ScreenshotDirectory, true);

    const FString Filename = FPaths::Combine(
        ScreenshotDirectory,
        FString::Printf(TEXT("marker_%03d_%s.png"), MarkerCount, *FDateTime::UtcNow().ToString(TEXT("%H%M%S"))));

    FScreenshotRequest::RequestScreenshot(Filename, false, false);
}

void APlaytestRecorderActor::AppendTelemetry(float DeltaSeconds)
{
    const double Elapsed = FPlatformTime::Seconds() - SessionStartSeconds;
    const double FrameMs = DeltaSeconds > 0.0f ? static_cast<double>(DeltaSeconds) * 1000.0 : 0.0;
    const double Fps = DeltaSeconds > 0.0f ? 1.0 / static_cast<double>(DeltaSeconds) : 0.0;

    FVector Location = FVector::ZeroVector;
    double Speed = 0.0;

    if (APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
    {
        if (APawn* Pawn = PlayerController->GetPawn())
        {
            Location = Pawn->GetActorLocation();
            Speed = Pawn->GetVelocity().Size();
        }
    }

    const FString Row = FString::Printf(
        TEXT("%s,%.3f,%.6f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f\n"),
        *CsvEscape(UtcNowIso()),
        Elapsed,
        static_cast<double>(DeltaSeconds),
        Fps,
        FrameMs,
        Location.X,
        Location.Y,
        Location.Z,
        Speed);

    FFileHelper::SaveStringToFile(Row, *TelemetryPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM, &IFileManager::Get(), FILEWRITE_Append);
}

FString APlaytestRecorderActor::UtcNowIso() const
{
    return FDateTime::UtcNow().ToIso8601();
}

FString APlaytestRecorderActor::CsvEscape(const FString& Value)
{
    FString Escaped = Value;
    Escaped.ReplaceInline(TEXT("\""), TEXT("\"\""));
    return FString::Printf(TEXT("\"%s\""), *Escaped);
}
