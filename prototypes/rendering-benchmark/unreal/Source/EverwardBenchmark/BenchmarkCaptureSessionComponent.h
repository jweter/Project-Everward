#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BenchmarkCaptureSessionComponent.generated.h"

class ABenchmarkAdapter;
class FJsonObject;

UCLASS(ClassGroup = (Everward), meta = (BlueprintSpawnableComponent))
class EVERWARDBENCHMARK_API UBenchmarkCaptureSessionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UBenchmarkCaptureSessionComponent();

    virtual void BeginPlay() override;
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

private:
    static constexpr int32 ExpectedHandoffVersion = 2;
    static constexpr double WarmupSeconds = 5.0;

    TWeakObjectPtr<ABenchmarkAdapter> Adapter;
    TSharedPtr<FJsonObject> Handoff;
    double WarmupElapsedSeconds = 0.0;
    double CaptureElapsedSeconds = 0.0;
    bool bCapturing = false;
    bool bCompleted = false;
    TArray<double> CpuGameThreadSamplesMs;
    uint64 PeakProcessPhysicalBytes = 0;

    bool LoadCanonicalHandoff();
    void BeginCanonicalCapture();
    void FinishCanonicalCapture();
    bool WriteObservation() const;
    TSharedPtr<FJsonObject> BuildObservation() const;

    static double Mean(const TArray<double>& Values);
    static double Percentile(const TArray<double>& SortedValues, double Fraction);
};
