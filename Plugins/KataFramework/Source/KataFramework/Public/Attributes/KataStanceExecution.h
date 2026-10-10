#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "KataStanceExecution.generated.h"

/**
 * 공격의 Poise·Groggy 피해를 UKataAttributeSet_Stance의 메타 Attribute로 내보내는 Execution.
 *
 * 피해 GE의 Executions에 UKataDamageExecution과 함께 넣어 쓴다. 계산식은 다음과 같다.
 *   Poise 피해  = Poise SetByCaller × 맞은 쪽 PoiseDamageTakenMultiplier
 *   Groggy 피해 = Groggy SetByCaller
 * SetByCaller 태그는 UKataCombatSettings에서 정한다. 값이 없는 공격은 해당 피해가 0이다.
 * 맞은 쪽에 Stance 세트가 없으면 아무것도 내보내지 않는다. 경직하지 않는 대상은 피해와 사망만 처리한다.
 * 배율은 적용 시점의 값을 쓰므로 하이퍼아머 구간에 맞으면 줄어든 배율이 적용된다.
 */
UCLASS(meta = (DisplayName = "Kata Stance Execution"))
class KATAFRAMEWORK_API UKataStanceExecution : public UGameplayEffectExecutionCalculation
{
    GENERATED_BODY()

public:
    UKataStanceExecution();

    virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
        FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
