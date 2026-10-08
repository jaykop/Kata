#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "StateTreeReference.h"
#include "KataAIData.generated.h"

class UAISense;
class UAISenseConfig;
class UTargetingPreset;

/**
 * 마스터 StateTree의 Linked Asset 상태를 교체할 하위 트리 지정이다.
 * Slot은 대상 Linked 상태의 Tag와 일치해야 한다. 엔진은 Parameters에 바인딩이 있는 Linked 상태를 교체하지 않으므로
 * 하위 트리에 필요한 값은 이 참조의 파라미터 오버라이드로 지정한다.
 */
USTRUCT(BlueprintType)
struct KATAAI_API FKataAIStateTreeSlot
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|AI", meta = (Categories = "StateTree.Slot"))
    FGameplayTag Slot;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|AI",
        meta = (Schema = "/Script/GameplayStateTreeModule.StateTreeAIComponentSchema"))
    FStateTreeReference StateTree;
};

/** NPC가 공유하는 AI 설정이다. 감지 기록·대상·타이머 같은 실행 상태는 저장하지 않는다. */
UCLASS(BlueprintType, meta = (DisplayName = "Kata AI Data"))
class KATAAI_API UKataAIData : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    /** AI Component 스키마의 트리와 파라미터 오버라이드다. 비우면 행동 로직을 실행하지 않는다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|AI")
    FStateTreeReference StateTree;

    /** StateTree의 태그가 있는 Linked Asset 상태를 교체한다. 비우면 마스터 트리에 지정된 기본 하위 트리를 쓴다. 트리 시작 전에만 적용한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|AI")
    TArray<FKataAIStateTreeSlot> LinkedStateTreeSlots;

    /**
     * 이동 실패 후 허용할 재시도 횟수다. 0이면 무제한이다.
     * 마스터 트리와 슬롯 하위 트리가 함께 쓰므로 StateTree 파라미터 대신 Kata AI Context 출력으로 제공한다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|AI|Movement Failure", meta = (ClampMin = "0"))
    int32 MaxMoveRetries = 3;

    /** 이동 재시도 사이의 대기 시간이다. 프레임 단위 재시도를 막기 위해 0보다 커야 한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|AI|Movement Failure", meta = (ClampMin = "0.01", Units = "s"))
    float MoveRetryInterval = 1.0f;

    /** Home에서 허용할 추격 거리다. 0이면 무제한이다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|AI|Movement Failure", meta = (ClampMin = "0", Units = "cm"))
    float LeashDistance = 0.0f;

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
