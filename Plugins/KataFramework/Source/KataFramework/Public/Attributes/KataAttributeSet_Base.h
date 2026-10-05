#pragma once

#include "Attributes/KataAttributeSet.h"
#include "CoreMinimal.h"
#include "KataAttributeSet_Base.generated.h"

struct FGameplayEffectSpec;

/**
 * Health가 0이 된 순간을 알린다. 사망 처리는 이 신호를 받는 쪽이 맡는다.
 * @param EffectInstigator 마지막 피해를 준 GE의 Instigator. 없으면 nullptr.
 * @param EffectSpec 마지막 피해를 준 GE 스펙. 호출 동안만 유효하다.
 */
DECLARE_MULTICAST_DELEGATE_TwoParams(FKataOutOfHealthSignature, AActor* /*EffectInstigator*/, const FGameplayEffectSpec& /*EffectSpec*/);

/**
 * 모든 캐릭터가 공통으로 쓰는 자원 스탯. Health·Stamina·Mana와 각각의 최대값, 회복 속도를 가진다.
 *
 * 피해와 회복은 메타 Attribute Damage·Healing으로 받는다. GE가 이 값에 들어오면 PostGameplayEffectExecute에서 Health로 옮기고 0으로 되돌린다.
 * Damage는 UKataDamageExecution만 쓰도록 모디파이어 선택기에서 숨긴다. 방어 식을 거치지 않는 피해 경로를 만들지 않기 위해서다.
 * 회복 속도는 값만 정의한다. 실제 회복은 이 값을 참조하는 Infinite Periodic GE가 수행한다.
 */
UCLASS(meta = (DisplayName = "Kata Attribute Set: Base"))
class KATAFRAMEWORK_API UKataAttributeSet_Base : public UKataAttributeSet
{
    GENERATED_BODY()

public:
    ATTRIBUTE_ACCESSORS_BASIC(UKataAttributeSet_Base, Health)
    ATTRIBUTE_ACCESSORS_BASIC(UKataAttributeSet_Base, MaxHealth)
    ATTRIBUTE_ACCESSORS_BASIC(UKataAttributeSet_Base, Stamina)
    ATTRIBUTE_ACCESSORS_BASIC(UKataAttributeSet_Base, MaxStamina)
    ATTRIBUTE_ACCESSORS_BASIC(UKataAttributeSet_Base, StaminaRegenRate)
    ATTRIBUTE_ACCESSORS_BASIC(UKataAttributeSet_Base, Mana)
    ATTRIBUTE_ACCESSORS_BASIC(UKataAttributeSet_Base, MaxMana)
    ATTRIBUTE_ACCESSORS_BASIC(UKataAttributeSet_Base, ManaRegenRate)
    ATTRIBUTE_ACCESSORS_BASIC(UKataAttributeSet_Base, Damage)
    ATTRIBUTE_ACCESSORS_BASIC(UKataAttributeSet_Base, Healing)

    /** Health가 0보다 큰 상태에서 0이 되면 한 번 알린다. Health가 다시 0보다 커지면 다음 0 도달에서 다시 알린다. */
    FKataOutOfHealthSignature OnOutOfHealth;

    /** Health가 0 이하인지 돌려준다. */
    bool IsOutOfHealth() const { return bOutOfHealth; }

    //~ Begin UAttributeSet Interface
    virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
    virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
    //~ End UAttributeSet Interface

protected:
    virtual void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const override;

private:
    UPROPERTY(BlueprintReadOnly, Category = "Kata|Attributes|Health", meta = (AllowPrivateAccess = true))
    FGameplayAttributeData Health;

    UPROPERTY(BlueprintReadOnly, Category = "Kata|Attributes|Health", meta = (AllowPrivateAccess = true))
    FGameplayAttributeData MaxHealth;

    UPROPERTY(BlueprintReadOnly, Category = "Kata|Attributes|Stamina", meta = (AllowPrivateAccess = true))
    FGameplayAttributeData Stamina;

    UPROPERTY(BlueprintReadOnly, Category = "Kata|Attributes|Stamina", meta = (AllowPrivateAccess = true))
    FGameplayAttributeData MaxStamina;

    /** 초당 Stamina 회복량. */
    UPROPERTY(BlueprintReadOnly, Category = "Kata|Attributes|Stamina", meta = (AllowPrivateAccess = true))
    FGameplayAttributeData StaminaRegenRate;

    UPROPERTY(BlueprintReadOnly, Category = "Kata|Attributes|Mana", meta = (AllowPrivateAccess = true))
    FGameplayAttributeData Mana;

    UPROPERTY(BlueprintReadOnly, Category = "Kata|Attributes|Mana", meta = (AllowPrivateAccess = true))
    FGameplayAttributeData MaxMana;

    /** 초당 Mana 회복량. */
    UPROPERTY(BlueprintReadOnly, Category = "Kata|Attributes|Mana", meta = (AllowPrivateAccess = true))
    FGameplayAttributeData ManaRegenRate;

    /** 메타 Attribute. 방어 계산을 마친 피해량이며 Health에서 뺀 뒤 0으로 되돌린다. */
    UPROPERTY(BlueprintReadOnly, Category = "Kata|Attributes|Meta", meta = (AllowPrivateAccess = true, HideFromModifiers))
    FGameplayAttributeData Damage;

    /** 메타 Attribute. 회복량이며 Health에 더한 뒤 0으로 되돌린다. */
    UPROPERTY(BlueprintReadOnly, Category = "Kata|Attributes|Meta", meta = (AllowPrivateAccess = true))
    FGameplayAttributeData Healing;

    /** OnOutOfHealth를 한 번만 보내기 위한 상태. */
    bool bOutOfHealth = false;
};
