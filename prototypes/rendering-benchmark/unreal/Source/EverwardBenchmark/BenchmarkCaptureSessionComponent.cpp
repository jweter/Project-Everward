#include "BenchmarkCaptureSessionComponent.h"

#include "BenchmarkAdapter.h"
#include "Dom/JsonObject.h"
#include "EngineGlobals.h"
#include "HAL/PlatformMemory.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "Misc/DateTime.h"
#include "Misc/EngineVersion.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "RHI.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

UBenchmarkCaptureSessionComponent::UBenchmarkCaptureSessionComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UBenchmarkCaptureSessionComponent::BeginPlay()
{
    Super::BeginPlay();
    Adapter = Cast<ABenchmarkAdapter>(GetOwner());
}

void UBenchmarkCaptureSessionComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (bCompleted || !Adapter.IsValid() || !Adapter->IsCanonicalHandoffReady())
    {
        return;
    }

    if (!bCapturing)
    {
        WarmupElapsedSeconds += static_cast<double>(DeltaTime);
        if (WarmupElapsedSeconds >= WarmupSeconds)
        {
            BeginCanonicalCapture();
        }
        return;
    }

    const double GameThreadMs = FPlatformTime::ToMilliseconds(GGameThreadTime);
    CpuGameThreadSamplesMs.Add(GameThreadMs);

    const FPlatformMemoryStats MemoryStats = FPlatformMemory::GetStats();
    PeakProcessPhysicalBytes = FMath::Max(PeakProcessPhysicalBytes, MemoryStats.PeakUsedPhysical);

    CaptureElapsedSeconds += static_cast<double>(DeltaTime);
    if (CaptureElapsedSeconds >= Adapter->GetCanonicalDurationSeconds())
    {
        FinishCanonicalCapture();
    }
}

void UBenchmarkCaptureSessionComponent::BeginCanonicalCapture()
{
    bCapturing = true;
    CaptureElapsedSeconds = 0.0;
    CpuGameThreadSamplesMs.Reset();
    PeakProcessPhysicalBytes = 0;
    Adapter->RestartCanonicalPlayback();

    UE_LOG(LogTemp, Display, TEXT("Everward Unreal benchmark capture started after %.1f s warmup"), WarmupSeconds);
}

void UBenchmarkCaptureSessionComponent::FinishCanonicalCapture()
{
    bCapturing = false;
    bCompleted = true;

    if (!WriteObservation())
    {
        UE_LOG(LogTemp, Error, TEXT("Unable to write Unreal benchmark capture observation"));
        return;
    }

    UE_LOG(LogTemp, Display, TEXT("Everward Unreal benchmark capture observation written to Saved/unreal_capture_observation.json"));
}

bool UBenchmarkCaptureSessionComponent::WriteObservation() const
{
    const TSharedPtr<FJsonObject> Observation = BuildObservation();
    FString JsonText;
    const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonText);
    if (!FJsonSerializer::Serialize(Observation.ToSharedRef(), Writer))
    {
        return false;
    }

    const FString OutputPath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("unreal_capture_observation.json"));
    return FFileHelper::SaveStringToFile(JsonText, *OutputPath);
}

TSharedPtr<FJsonObject> UBenchmarkCaptureSessionComponent::BuildObservation() const
{
    TArray<double> SortedSamples = CpuGameThreadSamplesMs;
    SortedSamples.Sort();

    const double MeanMs = Mean(SortedSamples);
    const double P50Ms = Percentile(SortedSamples, 0.50);
    const double P95Ms = Percentile(SortedSamples, 0.95);
    const double MaxMs = SortedSamples.Num() > 0 ? SortedSamples.Last() : 0.0;
    const double PeakMemoryMiB = static_cast<double>(PeakProcessPhysicalBytes) / 1048576.0;

    const FPlatformMemoryConstants MemoryConstants = FPlatformMemory::GetConstants();
    const double RamGiB = static_cast<double>(MemoryConstants.TotalPhysical) / 1073741824.0;

    const FString EngineVersion = FEngineVersion::Current().ToString();
    const FString OsVersion = FPlatformMisc::GetOSVersion();
    const FString CpuModel = FPlatformMisc::GetCPUBrand();
    const FString GpuModel = GRHIAdapterName;

    TSharedPtr<FJsonObject> Hardware = MakeShared<FJsonObject>();
    Hardware->SetStringField(TEXT("engine_version"), EngineVersion);
    Hardware->SetStringField(TEXT("os_version"), OsVersion);
    Hardware->SetStringField(TEXT("cpu_model"), CpuModel);
    Hardware->SetStringField(TEXT("gpu_model"), GpuModel);
    Hardware->SetNumberField(TEXT("ram_gib"), RamGiB);

    TSharedPtr<FJsonObject> CpuFrameTime = MakeShared<FJsonObject>();
    CpuFrameTime->SetNumberField(TEXT("mean"), MeanMs);
    CpuFrameTime->SetNumberField(TEXT("p50"), P50Ms);
    CpuFrameTime->SetNumberField(TEXT("p95"), P95Ms);
    CpuFrameTime->SetNumberField(TEXT("max"), MaxMs);

    TSharedPtr<FJsonObject> RunRecordPrefill = MakeShared<FJsonObject>();
    RunRecordPrefill->SetStringField(TEXT("engine_version"), EngineVersion);
    RunRecordPrefill->SetStringField(TEXT("os_version"), OsVersion);
    RunRecordPrefill->SetStringField(TEXT("cpu_model"), CpuModel);
    RunRecordPrefill->SetStringField(TEXT("gpu_model"), GpuModel);
    RunRecordPrefill->SetNumberField(TEXT("ram_gib"), RamGiB);
    RunRecordPrefill->SetNumberField(TEXT("cpu_frame_time_ms"), P50Ms);
    RunRecordPrefill->SetNumberField(TEXT("peak_memory_mib"), PeakMemoryMiB);

    TArray<TSharedPtr<FJsonValue>> ManualEvidence;
    for (const TCHAR* Field : {
        TEXT("project_settings"),
        TEXT("gpu_frame_time_ms"),
        TEXT("implementation_hours"),
        TEXT("build_size_mib"),
        TEXT("screenshots"),
        TEXT("notes")})
    {
        ManualEvidence.Add(MakeShared<FJsonValueString>(Field));
    }

    TArray<TSharedPtr<FJsonValue>> MeasurementNotes;
    MeasurementNotes.Add(MakeShared<FJsonValueString>(
        TEXT("CPU frame timing uses Unreal GGameThreadTime converted with FPlatformTime::ToMilliseconds.")));
    MeasurementNotes.Add(MakeShared<FJsonValueString>(
        TEXT("peak_memory_mib uses FPlatformMemory::GetStats().PeakUsedPhysical during the canonical capture window.")));
    MeasurementNotes.Add(MakeShared<FJsonValueString>(
        TEXT("GPU frame time remains manual and must come from Unreal Insights, stat GPU, or equivalent engine-native profiling.")));

    TSharedPtr<FJsonObject> Observation = MakeShared<FJsonObject>();
    Observation->SetNumberField(TEXT("observation_version"), 1);
    Observation->SetStringField(TEXT("engine"), TEXT("unreal"));
    Observation->SetStringField(TEXT("scenario_name"), Adapter->GetCanonicalScenarioName());
    Observation->SetNumberField(TEXT("scenario_version"), Adapter->GetCanonicalScenarioVersion());
    Observation->SetStringField(TEXT("captured_at_utc"), FDateTime::UtcNow().ToIso8601());
    Observation->SetNumberField(TEXT("warmup_seconds"), WarmupSeconds);
    Observation->SetNumberField(TEXT("capture_duration_seconds"), Adapter->GetCanonicalDurationSeconds());
    Observation->SetNumberField(TEXT("frame_sample_count"), SortedSamples.Num());
    Observation->SetObjectField(TEXT("hardware"), Hardware);
    Observation->SetObjectField(TEXT("cpu_frame_time_ms"), CpuFrameTime);
    Observation->SetNumberField(TEXT("peak_memory_mib"), PeakMemoryMiB);
    Observation->SetObjectField(TEXT("run_record_prefill"), RunRecordPrefill);
    Observation->SetArrayField(TEXT("manual_evidence_still_required"), ManualEvidence);
    Observation->SetArrayField(TEXT("measurement_notes"), MeasurementNotes);
    return Observation;
}

double UBenchmarkCaptureSessionComponent::Mean(const TArray<double>& Values)
{
    if (Values.Num() == 0)
    {
        return 0.0;
    }

    double Total = 0.0;
    for (const double Value : Values)
    {
        Total += Value;
    }
    return Total / static_cast<double>(Values.Num());
}

double UBenchmarkCaptureSessionComponent::Percentile(const TArray<double>& SortedValues, double Fraction)
{
    if (SortedValues.Num() == 0)
    {
        return 0.0;
    }

    const int32 Index = FMath::Clamp(
        FMath::RoundToInt(static_cast<double>(SortedValues.Num() - 1) * Fraction),
        0,
        SortedValues.Num() - 1);
    return SortedValues[Index];
}
