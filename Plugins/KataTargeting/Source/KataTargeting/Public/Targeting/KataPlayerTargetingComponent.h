#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Targeting/KataTargetingComponent.h"
#include "KataPlayerTargetingComponent.generated.h"

class UTargetingPreset;

/** 락온 대상이 유효하지 않게 됐을 때의 동작. */
UENUM(BlueprintType)
enum class EKataLockLostBehavior : uint8
{
    /** 락온을 해제한다. */
    Release,
    /** Lock On Preset으로 다음 대상을 찾아 옮긴다. 후보가 없으면 해제한다. */
    SwitchToNext
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FKataLockTargetChangedSignature, AActor*, OldTarget, AActor*, NewTarget);

/**
 * PC용 타게팅 컴포넌트. 소프트 타겟과 락온을 관리한다.
 *
 * 소프트 타겟은 액션을 시작할 때 ResolveActionTarget()이 Soft Target Preset으로 한 번 갱신한다.
 * 락온은 AcquireLock()으로 걸고 SwitchLockLeft()·SwitchLockRight()로 바꾼다. 입력 연결은 호출하는 쪽이 맡는다.
 * 락온 중에만 컴포넌트 Tick이 켜지며, Tick 간격마다 거리와 대상 태그를 확인한다. 대상 파괴는 OnEndPlay로 즉시 처리한다.
 * 대상은 약한 참조로 보관하므로 이 컴포넌트가 대상의 수명을 늘리지 않는다.
 */
UCLASS(Blueprintable, ClassGroup = (Kata), meta = (BlueprintSpawnableComponent))
class KATATARGETING_API UKataPlayerTargetingComponent : public UKataTargetingComponent
{
    GENERATED_BODY()

public:
    UKataPlayerTargetingComponent();

    /** 액션 시작 때 락온 대상이 없으면 소프트 타겟을 고르는 Preset. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Targeting")
    TObjectPtr<UTargetingPreset> SoftTargetPreset;

    /** AcquireLock()과 다음 대상 전환에 쓰는 Preset. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Targeting")
    TObjectPtr<UTargetingPreset> LockOnPreset;

    /** SwitchLockLeft()에 쓰는 Preset. 보통 Kata Filter Lock Side(Left)를 포함한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Targeting")
    TObjectPtr<UTargetingPreset> SwitchLeftPreset;

    /** SwitchLockRight()에 쓰는 Preset. 보통 Kata Filter Lock Side(Right)를 포함한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Targeting")
    TObjectPtr<UTargetingPreset> SwitchRightPreset;

    /** 락온 대상과의 거리가 이 값을 넘으면 락온을 잃는다. 0이면 거리로는 풀리지 않는다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Targeting", meta = (ClampMin = "0.0", Units = "cm"))
    float MaxLockDistance = 0.0f;

    /** 대상의 ASC에 이 태그 중 하나라도 있으면 락온을 잃는다. 예: 사망 상태 태그. 대상에 ASC가 없으면 보지 않는다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Targeting")
    FGameplayTagContainer LockBreakTags;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Targeting")
    EKataLockLostBehavior LockLostBehavior = EKataLockLostBehavior::Release;

    /** 락온 대상이 바뀔 때 알린다. 해제하면 NewTarget이 nullptr이다. 카메라와 HUD가 구독한다. */
    UPROPERTY(BlueprintAssignable, Category = "Kata|Targeting")
    FKataLockTargetChangedSignature OnLockTargetChanged;

    UFUNCTION(BlueprintPure, Category = "Kata|Targeting")
    AActor* GetSoftTarget() const { return SoftTarget.Get(); }

    UFUNCTION(BlueprintPure, Category = "Kata|Targeting")
    AActor* GetLockTarget() const { return LockTarget.Get(); }

    UFUNCTION(BlueprintPure, Category = "Kata|Targeting")
    bool IsLocked() const { return LockTarget.IsValid(); }

    /** Soft Target Preset을 즉시 실행해 소프트 타겟을 갱신하고 돌려준다. 후보가 없으면 소프트 타겟을 비운다. */
    UFUNCTION(BlueprintCallable, Category = "Kata|Targeting")
    AActor* UpdateSoftTarget();

    /** Lock On Preset의 가장 우선하는 후보로 락온한다. 후보가 없으면 현재 상태를 바꾸지 않고 false를 반환한다. */
    UFUNCTION(BlueprintCallable, Category = "Kata|Targeting")
    bool AcquireLock();

    /** 락온 중일 때 왼쪽 전환 Preset의 가장 우선하는 후보로 바꾼다. 후보가 없으면 현재 대상을 유지하고 false를 반환한다. */
    UFUNCTION(BlueprintCallable, Category = "Kata|Targeting")
    bool SwitchLockLeft();

    /** 락온 중일 때 오른쪽 전환 Preset의 가장 우선하는 후보로 바꾼다. 후보가 없으면 현재 대상을 유지하고 false를 반환한다. */
    UFUNCTION(BlueprintCallable, Category = "Kata|Targeting")
    bool SwitchLockRight();

    UFUNCTION(BlueprintCallable, Category = "Kata|Targeting")
    void ReleaseLock();

    virtual AActor* GetCurrentTarget_Implementation() const override;

    /** 락온 대상이 있으면 그 대상을, 없으면 소프트 타겟을 갱신해 돌려준다. */
    virtual AActor* ResolveActionTarget_Implementation() override;

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    /** 락온 대상을 바꾸는 단일 경로. 파괴 구독, Tick 켜고 끄기, 변경 알림을 함께 처리한다. */
    void SetLockTarget(AActor* NewTarget);

    bool SwitchLock(const UTargetingPreset* Preset);

    /** 거리와 대상 태그 조건을 만족하는지. 파괴 여부도 함께 본다. */
    bool IsLockTargetValid(const AActor* Target) const;

    /** 락온 대상을 잃었을 때 LockLostBehavior를 적용한다. */
    void HandleLockLost();

    UFUNCTION()
    void HandleLockTargetEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason);

    /** 실행 상태. 저장하지 않으며 대상 수명에 관여하지 않도록 약한 참조로 둔다. */
    TWeakObjectPtr<AActor> SoftTarget;

    TWeakObjectPtr<AActor> LockTarget;
};
