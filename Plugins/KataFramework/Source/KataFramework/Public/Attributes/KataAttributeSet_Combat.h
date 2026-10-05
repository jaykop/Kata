#pragma once

#include "Attributes/KataAttributeSet.h"
#include "CoreMinimal.h"
#include "KataAttributeSet_Combat.generated.h"

/**
 * 전투에 참여하는 캐릭터의 공격·방어 스탯.
 *
 * UKataDamageExecution이 공격한 쪽의 AttackPower와 맞은 쪽의 Defense를 캡처한다. 버프가 반영된 최종값을 쓴다.
 * 싸우지 않는 대상(부서지는 오브젝트 등)은 이 세트 없이 Base 세트만 가져도 된다.
 */
UCLASS(meta = (DisplayName = "Kata Attribute Set: Combat"))
class KATAFRAMEWORK_API UKataAttributeSet_Combat : public UKataAttributeSet
{
    GENERATED_BODY()

public:
    ATTRIBUTE_ACCESSORS_BASIC(UKataAttributeSet_Combat, AttackPower)
    ATTRIBUTE_ACCESSORS_BASIC(UKataAttributeSet_Combat, Defense)

protected:
    virtual void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const override;

private:
    /** 공격력. 피해 계산에서 공격 계수에 곱한다. */
    UPROPERTY(BlueprintReadOnly, Category = "Kata|Attributes|Combat", meta = (AllowPrivateAccess = true))
    FGameplayAttributeData AttackPower;

    /** 방어력. 피해 계산에서 K / (K + Defense) 비율로 피해를 줄인다. */
    UPROPERTY(BlueprintReadOnly, Category = "Kata|Attributes|Combat", meta = (AllowPrivateAccess = true))
    FGameplayAttributeData Defense;
};
