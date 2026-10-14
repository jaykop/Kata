#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "KataVM_EnemyHealth.generated.h"

/**
 * 적 체력바 하나의 ViewModel. UKataHUDSubsystem이 대상 ASC의 Health·MaxHealth와 사망·락온 상태를 채운다.
 * 항목 위젯은 Manual 생성 방식으로 두고, 레이어(UKataEnemyHealthBarLayer)가 위젯마다 이 ViewModel을 넣는다.
 * 대상 액터는 약한 참조라 대상이 사라지면 GetBarWorldLocation이 false를 반환한다.
 */
UCLASS(BlueprintType, meta = (DisplayName = "Kata Enemy Health Viewmodel"))
class KATAFRAMEWORK_API UKataVM_EnemyHealth : public UMVVMViewModelBase
{
    GENERATED_BODY()

public:
    float GetHealth() const { return Health; }
    float GetMaxHealth() const { return MaxHealth; }
    bool IsLockedOn() const { return bLockedOn; }
    bool IsDead() const { return bDead; }

    /** 최대값이 0 이하면 0이다. */
    UFUNCTION(BlueprintPure, FieldNotify, Category = "Kata|HUD")
    float GetHealthRatio() const;

    /**
     * 바를 그릴 월드 위치. 대상 캡슐 꼭대기에서 Kata HUD 설정의 Bar Height Offset만큼 위다.
     * 루트가 캡슐이 아니면 액터 바운드 꼭대기를 쓴다. 매 프레임 바뀌므로 FieldNotify로 알리지 않으며, 레이어가 Tick마다 읽는다.
     * 대상이 사라졌으면 false를 반환한다.
     */
    UFUNCTION(BlueprintPure, Category = "Kata|HUD")
    bool GetBarWorldLocation(FVector& OutLocation) const;

    UFUNCTION(BlueprintPure, Category = "Kata|HUD")
    AActor* GetTargetActor() const { return TargetActor.Get(); }

    void SetTargetActor(AActor* NewTarget) { TargetActor = NewTarget; }
    void SetHealth(float NewValue);
    void SetMaxHealth(float NewValue);
    void SetLockedOn(bool bNewValue);
    void SetDead(bool bNewValue);

private:
    UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, Category = "Kata|HUD", meta = (AllowPrivateAccess = true))
    float Health = 0.0f;

    UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, Category = "Kata|HUD", meta = (AllowPrivateAccess = true))
    float MaxHealth = 0.0f;

    /** 로컬 플레이어가 이 대상을 락온하고 있으면 true다. */
    UPROPERTY(BlueprintReadOnly, FieldNotify, Getter = "IsLockedOn", Category = "Kata|HUD", meta = (AllowPrivateAccess = true))
    bool bLockedOn = false;

    /** 대상이 죽었으면 true다. 바는 Kata HUD 설정의 Dead Hide Delay 뒤 사라진다. */
    UPROPERTY(BlueprintReadOnly, FieldNotify, Getter = "IsDead", Category = "Kata|HUD", meta = (AllowPrivateAccess = true))
    bool bDead = false;

    TWeakObjectPtr<AActor> TargetActor;
};
