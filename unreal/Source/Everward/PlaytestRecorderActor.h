#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PlaytestRecorderActor.generated.h"

/**
 * Lightweight non-blocking playtest evidence recorder for the production
 * Everward Unreal project. Evidence capture must never determine gameplay
 * truth or prevent the game from running.
 */
UCLASS()
class EVERWARD_API APlaytestRecorderActor : public AActor
{
    GENERATED_BODY()

public:
    APlaytestRecorderActor();
    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintCallable, Category = "Everward|Playtest")
    void RecordPlaytestEvent(const FString& EventName, const FString& Details = TEXT(""));

    UFUNCTION(BlueprintCallable, Category = "Everward|Playtest")
    void RecordIssueMarker(const FString& Note = TEXT(""));

    UFUNCTION(BlueprintPure, Category = "Everward|Playtest")
    FString GetPlaytestSessionDirectory() const { return SessionDirectory; }

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    void InitializeSession();
    void WriteSessionMetadata(const FString& Status, const FString& EndedUtc = TEXT(""));
    void AppendTelemetry(float DeltaSeconds);
    void CaptureIssueScreenshot();
    FString UtcNowIso() const;
    static FString CsvEscape(const FString& Value);

    FString SessionId;
    FString SessionDirectory;
    FString EventsPath;
    FString TelemetryPath;
    FString StartedUtc;

    double SessionStartSeconds = 0.0;
    float TelemetryAccumulator = 0.0f;
    int32 MarkerCount = 0;

    static constexpr float TelemetryIntervalSeconds = 0.5f;
};
