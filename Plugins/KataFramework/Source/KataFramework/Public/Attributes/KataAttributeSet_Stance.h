#pragma once

#include "Attributes/KataAttributeSet.h"
#include "CoreMinimal.h"
#include "KataAttributeSet_Stance.generated.h"

/**
 * 피격 반응을 정하는 자세 스탯. Poise(버티는 힘)와 Groggy(쌓이면 무방비가 되는 게이지)를 가진다.
 *
 * Poise·Groggy 피해는 메타 Attribute PoiseDamage·GroggyDamage로 받는다. UKataStanceExecution만 이 값을 쓰도록 모디파이어 선택기에서 숨긴다.
 * Poise 회복과 Groggy 감소는 값만 정의한다. 실제 변화는 이 값을 참조하는 Infinite Periodic GE가 수행하며, 피격 직후의 지연은 GE의 Ongoing Tag Requirements로 처리한다.
 * 경직하지 않는 대상(훈련용 더미, 오브젝트 등)은 이 세트를 넣지 않는다. 세트가 없으면 피해와 사망만 처리하고 반응하지 않는다.
 */
UCLASS(meta = (DisplayName = "Kata Attribute Set: Stance"))
class KATAFRAMEWORK_API UKataAttributeSet_Stance : public UKataAttributeSet
{
    GENERATED_BODY()

public:
    ATTRIBUTE_ACCESSORS_BASIC(UKataAttributeSet_Stance, Poise)
    ATTRIBUTE_ACCESSORS_BASIC(UKataAttributeSet_Stance, MaxPoise)
    ATTRIBUTE_ACCESSORS_BASIC(UKataAttributeSet_Stance, PoiseDamageTakenMultiplier)
    ATTRIBUTE_ACCESSORS_BASIC(UKataAttributeSet_Stance, Groggy)
    ATTRIBUTE_ACCESSORS_BASIC(UKataAttributeSet_Stance, MaxGroggy)
    ATTRIBUTE_ACCESSORS_BASIC(UKataAttributeSet_Stance, GroggyDecayRate)
    ATTRIBUTE_ACCESSORS_BASIC(UKataAttributeSet_Stance, PoiseDamage)
    ATTRIBUTE_ACCESSORS_BASIC(UKataAttributeSet_Stance, GroggyDamage)

    //~ Begin UAttributeSet Interface
    virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
    virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
    //~ End UAttributeSet Interface

protected:
    virtual void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const override;

private:
    /** 현재 Poise. 0이 되면 무너진 것으로 본다. MaxPoise만 지정하면 가득 찬 상태로 시작한다. */
    UPROPERTY(BlueprintReadOnly, Category = "Kata|Attributes|Poise", meta = (AllowPrivateAccess = true))
    FGameplayAttributeData Poise;

    UPROPERTY(BlueprintReadOnly, Category = "Kata|Attributes|Poise", meta = (AllowPrivateAccess = true))
    FGameplayAttributeData MaxPoise;

    /**
     * 받는 Poise 피해에 곱하는 배율. 기본 1이다.
     * 하이퍼아머는 공격 구간 동안 이 값을 낮추는 GE로 표현한다. 현재 Poise를 직접 바꾸지 않으므로 구간이 끝나도 Poise가 튀지 않는다.
     */
    UPROPERTY(BlueprintReadOnly, Category = "Kata|Attributes|Poise", meta = (AllowPrivateAccess = true))
    FGameplayAttributeData PoiseDamageTakenMultiplier = 1.0f;

    /** 현재 Groggy. 0에서 시작해 쌓이며 MaxGroggy에 닿으면 그로기 반응 대상이 된다. */
    UPROPERTY(BlueprintReadOnly, Category = "Kata|Attributes|Groggy", meta = (AllowPrivateAccess = true))
    FGameplayAttributeData Groggy;

    UPROPERTY(BlueprintReadOnly, Category = "Kata|Attributes|Groggy", meta = (AllowPrivateAccess = true))
    FGameplayAttributeData MaxGroggy;

    /** 초당 Groggy 감소량. */
    UPROPERTY(BlueprintReadOnly, Category = "Kata|Attributes|Groggy", meta = (AllowPrivateAccess = true))
    FGameplayAttributeData GroggyDecayRate;

    /** 메타 Attribute. 배율을 적용한 Poise 피해량이며 Poise에서 뺀 뒤 0으로 되돌린다. */
    UPROPERTY(BlueprintReadOnly, Category = "Kata|Attributes|Meta", meta = (AllowPrivateAccess = true, HideFromModifiers))
    FGameplayAttributeData PoiseDamage;

    /** 메타 Attribute. Groggy 증가량이며 Groggy에 더한 뒤 0으로 되돌린다. */
    UPROPERTY(BlueprintReadOnly, Category = "Kata|Attributes|Meta", meta = (AllowPrivateAccess = true, HideFromModifiers))
    FGameplayAttributeData GroggyDamage;
};
