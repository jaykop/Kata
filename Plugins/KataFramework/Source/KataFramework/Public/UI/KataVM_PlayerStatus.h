#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "KataVM_PlayerStatus.generated.h"

/**
 * 로컬 플레이어가 조종하는 폰의 HP·스태미나·마나를 위젯에 전달하는 ViewModel.
 * 값은 UKataHUDSubsystem이 폰 ASC의 UKataAttributeSet_Base를 구독해 채운다. 위젯은 Resolver(UKataHUDViewModelResolver)로 받는다.
 * 비율 함수는 FieldNotify 함수라 현재값이나 최대값이 바뀔 때 함께 알린다. 최대값이 0 이하면 비율은 0이다.
 */
UCLASS(BlueprintType, meta = (DisplayName = "Kata Player Status Viewmodel"))
class KATAFRAMEWORK_API UKataVM_PlayerStatus : public UMVVMViewModelBase
{
    GENERATED_BODY()

public:
    float GetHealth() const { return Health; }
    float GetMaxHealth() const { return MaxHealth; }
    float GetStamina() const { return Stamina; }
    float GetMaxStamina() const { return MaxStamina; }
    float GetMana() const { return Mana; }
    float GetMaxMana() const { return MaxMana; }
    bool HasStatus() const { return bHasStatus; }

    UFUNCTION(BlueprintPure, FieldNotify, Category = "Kata|HUD")
    float GetHealthRatio() const;

    UFUNCTION(BlueprintPure, FieldNotify, Category = "Kata|HUD")
    float GetStaminaRatio() const;

    UFUNCTION(BlueprintPure, FieldNotify, Category = "Kata|HUD")
    float GetManaRatio() const;

    void SetHealth(float NewValue);
    void SetMaxHealth(float NewValue);
    void SetStamina(float NewValue);
    void SetMaxStamina(float NewValue);
    void SetMana(float NewValue);
    void SetMaxMana(float NewValue);
    void SetHasStatus(bool bNewValue);

private:
    UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, Category = "Kata|HUD", meta = (AllowPrivateAccess = true))
    float Health = 0.0f;

    UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, Category = "Kata|HUD", meta = (AllowPrivateAccess = true))
    float MaxHealth = 0.0f;

    UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, Category = "Kata|HUD", meta = (AllowPrivateAccess = true))
    float Stamina = 0.0f;

    UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, Category = "Kata|HUD", meta = (AllowPrivateAccess = true))
    float MaxStamina = 0.0f;

    UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, Category = "Kata|HUD", meta = (AllowPrivateAccess = true))
    float Mana = 0.0f;

    UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, Category = "Kata|HUD", meta = (AllowPrivateAccess = true))
    float MaxMana = 0.0f;

    /** 관찰 중인 폰에 ASC와 Base 세트가 있으면 true다. false면 모든 값이 0이다. */
    UPROPERTY(BlueprintReadOnly, FieldNotify, Getter = "HasStatus", Category = "Kata|HUD", meta = (AllowPrivateAccess = true))
    bool bHasStatus = false;
};
