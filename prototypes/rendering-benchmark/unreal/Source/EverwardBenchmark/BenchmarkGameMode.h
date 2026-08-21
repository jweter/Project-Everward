#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "BenchmarkGameMode.generated.h"

UCLASS()
class EVERWARDBENCHMARK_API ABenchmarkGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    virtual void StartPlay() override;
};
