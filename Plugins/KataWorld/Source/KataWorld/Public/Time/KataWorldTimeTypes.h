#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "KataWorldTimeTypes.generated.h"

/** 월드 시각이 흐르는 방식. */
UENUM(BlueprintType)
enum class EKataWorldTimeMode : uint8
{
    /** 하루 길이에 맞춰 흐른다. */
    Flowing,

    /** 시각이 멈춰 있다. 시각을 지정하는 호출로만 바뀐다. */
    Fixed
};

/** 시간대 구간 하나. 시작 시각부터 다음 구간의 시작 시각 전까지 이어진다. */
USTRUCT(BlueprintType)
struct KATAWORLD_API FKataTimePhase
{
    GENERATED_BODY()

    /** 구간이 시작하는 시각(시). 24는 0시와 같다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Time", meta = (ClampMin = "0.0", ClampMax = "24.0", UIMax = "24.0", ForceUnits = "Hours"))
    float StartHour = 0.0f;

    /** 현재 시각이 이 구간에 있을 때의 시간대 태그. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Time", meta = (Categories = "TimeOfDay"))
    FGameplayTag PhaseTag;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FKataTimePhaseChangedSignature, FGameplayTag, OldPhase, FGameplayTag, NewPhase);
DECLARE_MULTICAST_DELEGATE_TwoParams(FKataTimePhaseChangedNativeSignature, const FGameplayTag& /*OldPhase*/, const FGameplayTag& /*NewPhase*/);
