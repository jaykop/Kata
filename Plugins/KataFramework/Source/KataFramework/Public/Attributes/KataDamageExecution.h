#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "KataDamageExecution.generated.h"

/**
 * 공격 피해를 계산해 UKataAttributeSet_Base의 메타 Attribute Damage로 내보내는 Execution.
 *
 * Instant GE의 Executions에 넣어 쓴다. 계산식은 다음과 같다.
 *   Raw   = 공격 계수(SetByCaller) × 공격한 쪽 AttackPower
 *   Final = Raw × K / (K + 맞은 쪽 Defense)
 * 공격 계수의 태그와 K는 UKataCombatSettings에서 정한다.
 * 공격한 쪽에 Combat 세트가 없으면(함정 등) 공격 계수를 그대로 Raw로 쓰고, 맞은 쪽에 Combat 세트가 없으면 Defense를 0으로 본다.
 * AttackPower는 스펙을 만들 때 스냅샷하고 Defense는 적용 시점의 값을 쓴다. 둘 다 버프가 반영된 최종값이다.
 */
UCLASS(meta = (DisplayName = "Kata Damage Execution"))
class KATAFRAMEWORK_API UKataDamageExecution : public UGameplayEffectExecutionCalculation
{
    GENERATED_BODY()

public:
    UKataDamageExecution();

    virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
        FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
