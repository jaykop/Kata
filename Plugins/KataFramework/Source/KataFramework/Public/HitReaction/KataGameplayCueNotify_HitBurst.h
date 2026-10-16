#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Burst.h"
#include "GameplayCueNotifyTypes.h"
#include "GameplayTagContainer.h"
#include "KataGameplayCueNotify_HitBurst.generated.h"

/** 대상 태그 하나와, 그 태그가 있을 때 추가로 실행할 Burst 효과 묶음. */
USTRUCT(BlueprintType)
struct KATAFRAMEWORK_API FKataHitBurstTagEffects
{
    GENERATED_BODY()

    /**
     * Cue 파라미터의 대상 태그(Aggregated Target Tags)에 이 태그가 있으면 Effects를 실행한다.
     * 부모 태그를 지정하면 하위 태그도 맞는다. 히트 처리기는 맞은 HurtBox의 Hurt Box Tags를 대상 태그로 넘긴다.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit Burst", meta = (Categories = "HurtBox"))
    FGameplayTag Tag;

    /**
     * 이 항목이 처음 일치한 항목이면 기본 Burst Effects 뒤에 실행할 파티클·사운드 등.
     * Cue의 기본 Spawn Condition을 통과해야 실행한다. 각 효과는 기본 Spawn Condition과 Placement를 쓰며,
     * 효과별 Override를 켜면 해당 설정을 대신 쓴다.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit Burst")
    FGameplayCueNotify_BurstEffects Effects;
};

/**
 * 맞은 부위의 태그에 따라 이펙트를 고르는 Burst Gameplay Cue.
 *
 * 기본 Burst Effects(흔들림·진동 같은 공통 효과)를 엔진 Burst와 같이 실행한 뒤,
 * Tag Burst Effects에서 대상 태그와 맞는 첫 항목 하나를 더 실행한다. 목록 순서가 우선순위이며,
 * 맞는 항목이 없으면 기본 효과만 처리한다. 기본 효과와 태그별 효과 모두 Cue의 기본 Spawn Condition을 통과해야 실행한다.
 * Kata Hit Trace 결과에는 물리 재질이 없어 엔진 Surface Type이 Default로 판정되므로, 재질 구분은 HurtBox 태그로 한다.
 *
 * 대상 태그는 Apply Gameplay Effect 히트 처리기가 채운다. 다른 경로에서도 Cue 파라미터의 AggregatedTargetTags에
 * 태그를 넣으면 같은 규칙으로 선택한다. 대상 태그가 비어 있으면 기본 효과만 처리한다.
 */
UCLASS(Blueprintable, Category = "GameplayCueNotify", meta = (DisplayName = "Kata GCN Hit Burst"))
class KATAFRAMEWORK_API UKataGameplayCueNotify_HitBurst : public UGameplayCueNotify_Burst
{
    GENERATED_BODY()

protected:
    virtual bool OnExecute_Implementation(AActor* Target, const FGameplayCueParameters& Parameters) const override;

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif

    /** 대상 태그별 추가 효과. 위에서부터 검사해 처음 맞는 항목 하나만 실행한다. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GCN Effects")
    TArray<FKataHitBurstTagEffects> TagBurstEffects;
};
