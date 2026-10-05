#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "StateTreeReference.h"
#include "KataAIData.generated.h"

class UAISense;
class UAISenseConfig;
class UTargetingPreset;

/** NPC가 공유하는 AI 설정이다. 감지 기록·대상·타이머 같은 실행 상태는 저장하지 않는다. */
UCLASS(BlueprintType, meta = (DisplayName = "Kata AI Data"))
class KATAAI_API UKataAIData : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    /** AI Component 스키마의 트리와 파라미터 오버라이드다. 비우면 행동 로직을 실행하지 않는다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|AI")
    FStateTreeReference StateTree;

    /** 공유 설정 원본이다. Controller는 복제한 설정만 Perception에 등록한다. 같은 감각은 한 번만 지정한다. */
    UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Kata|Perception")
    TArray<TObjectPtr<UAISenseConfig>> Senses;

    /** 감지 위치 판정에 우선할 감각이다. 비우면 첫 유효 Sense를 사용하며 목록에 포함된 감각이어야 한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Perception")
    TSubclassOf<UAISense> DominantSense;

    /** 시각 인지 후보를 필터·정렬한다. 첫 Selection에 Kata Select Perceived Actors를 지정한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Targeting")
    TObjectPtr<UTargetingPreset> TargetingPreset;

    /** 시각 후보와 Preset을 다시 평가하는 간격이다. 감지 변경 이벤트는 즉시 반영한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Targeting", meta = (ClampMin = "0.01", Units = "s"))
    float TargetRefreshInterval = 0.2f;

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
