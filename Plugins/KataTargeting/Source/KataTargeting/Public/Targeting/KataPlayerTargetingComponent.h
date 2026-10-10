#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "GameplayTagContainer.h"
#include "Targeting/KataTargetingComponent.h"
#include "KataPlayerTargetingComponent.generated.h"

class UKataTargetPointComponent;
class UTargetingPreset;

/** 락온 지점을 잃었을 때의 동작. */
UENUM(BlueprintType)
enum class EKataLockLostBehavior : uint8
{
    /** 락온을 해제한다. */
    Release,
    /** Lock On Preset으로 다음 지점을 찾아 옮긴다. 후보가 없으면 해제한다. */
    SwitchToNext
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FKataLockTargetChangedSignature, UKataTargetPointComponent*, OldPoint, UKataTargetPointComponent*, NewPoint);

/**
 * PC용 타게팅 컴포넌트. 소프트 타겟과 락온을 관리한다.
 *
 * 소프트 타겟은 공격 방향을 정하는 마지막 기준이며 액터 단위다. 액션을 시작할 때 락온도 이동 입력도 없을 때만
 * ResolveActionTarget()이 Soft Target Preset으로 한 번 갱신하고, 둘 중 하나가 있으면 비운다.
 * 락온은 액터가 아니라 대상 부위에 붙인 UKataTargetPointComponent를 잡는다. 같은 액터의 다른 부위도 전환 후보다.
 * Lock On·전환 Preset은 Kata Expand Target Points로 지점을 펼쳐야 한다.
 * 공격 방향의 우선순위는 락온 지점 → 이동 입력 방향 → 소프트 타겟이며, 모두 없으면 돌지 않는다.
 * 락온 중에만 컴포넌트 Tick이 켜지며, Tick 간격마다 거리, 대상 태그와 시야를 확인한다. 대상 파괴와 지점 비활성화는 알림으로 즉시 처리한다.
 * 시야는 락온을 유지하는 동안에만 이 컴포넌트가 본다. 락온·전환 후보에서 가려진 지점을 빼려면 Preset에 Kata Filter Line Of Sight를 둔다.
 * 락온 중에는 자기 ASC에 LockingStatusTag를, 대상 액터의 ASC에 TargetedStatusTag를 Loose 태그로 붙인다.
 * 대상은 약한 참조로 보관하므로 이 컴포넌트가 대상의 수명을 늘리지 않는다.
 */
UCLASS(Blueprintable, ClassGroup = (Kata), PrioritizeCategories = ("Kata|Targeting", "Kata|Targeting|Status", "Kata|Faction"),
    meta = (BlueprintSpawnableComponent))
class KATATARGETING_API UKataPlayerTargetingComponent : public UKataTargetingComponent
{
    GENERATED_BODY()

public:
    UKataPlayerTargetingComponent();

    /** 액션 시작 때 락온이 없으면 소프트 타겟을 고르는 Preset. 액터 단위로 고른다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Targeting")
    TObjectPtr<UTargetingPreset> SoftTargetPreset;

    /** AcquireLock()과 다음 지점 전환에 쓰는 Preset. Kata Expand Target Points로 락온 지점을 펼쳐야 한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Targeting")
    TObjectPtr<UTargetingPreset> LockOnPreset;

    /** SwitchLockLeft()에 쓰는 Preset. 보통 Kata Expand Target Points 뒤에 Kata Filter Lock Side(Left)를 둔다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Targeting")
    TObjectPtr<UTargetingPreset> SwitchLeftPreset;

    /** SwitchLockRight()에 쓰는 Preset. 보통 Kata Expand Target Points 뒤에 Kata Filter Lock Side(Right)를 둔다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Targeting")
    TObjectPtr<UTargetingPreset> SwitchRightPreset;

    /** 락온 지점과의 거리가 이 값을 넘으면 락온을 잃는다. 0이면 거리로는 풀리지 않는다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Targeting", meta = (ClampMin = "0.0", Units = "cm"))
    float MaxLockDistance = 0.0f;

    /** 대상 액터의 ASC에 이 태그 중 하나라도 있으면 락온을 잃는다. 예: 사망 상태 태그. 대상에 ASC가 없으면 보지 않는다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Targeting")
    FGameplayTagContainer LockBreakTags;

    /**
     * 켜면 카메라 시점에서 락온 지점까지 LineOfSightChannel로 막는 물체가 LineOfSightGraceTime 동안 이어질 때 락온을 잃는다.
     * 실행 주체와 대상 액터, 두 액터에 붙은 액터는 가림으로 보지 않는다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Targeting")
    bool bBreakLockOnLineOfSightLoss = true;

    /** 락온 유지 중 가림을 판정할 트레이스 채널. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Targeting", meta = (EditCondition = "bBreakLockOnLineOfSightLoss"))
    TEnumAsByte<ECollisionChannel> LineOfSightChannel = ECC_Visibility;

    /**
     * 가려진 상태가 이 시간 동안 이어지면 락온을 잃는다. 기둥 뒤를 잠깐 지날 때 풀리지 않게 한다.
     * 판정은 Tick 간격마다 하므로 실제 해제 시점은 Tick 간격만큼 늦을 수 있다. 0이면 가려진 첫 판정에서 잃는다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Targeting",
        meta = (EditCondition = "bBreakLockOnLineOfSightLoss", ClampMin = "0.0", Units = "s"))
    float LineOfSightGraceTime = 0.5f;

    /** 대상 파괴, 거리 초과, 해제 태그, 시야 상실로 락온을 잃었을 때의 동작. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Targeting")
    EKataLockLostBehavior LockLostBehavior = EKataLockLostBehavior::Release;

    /** 락온 중인 지점이 꺼졌을 때(부위 파괴 등)의 동작. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Targeting")
    EKataLockLostBehavior LockPointDisabledBehavior = EKataLockLostBehavior::Release;

    /** 락온 중 이 컴포넌트 소유자의 ASC에 붙이는 Status 태그. 비어 있으면 붙이지 않는다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Targeting|Status", meta = (Categories = "Status"))
    FGameplayTag LockingStatusTag;

    /** 락온 중 대상 액터의 ASC에 붙이는 Status 태그. 비어 있거나 대상에 ASC가 없으면 붙이지 않는다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Targeting|Status", meta = (Categories = "Status"))
    FGameplayTag TargetedStatusTag;

    /** 락온 지점이 바뀔 때 알린다. 해제하면 NewPoint가 nullptr이다. 카메라와 HUD가 구독한다. */
    UPROPERTY(BlueprintAssignable, Category = "Kata|Targeting")
    FKataLockTargetChangedSignature OnLockTargetChanged;

    UFUNCTION(BlueprintPure, Category = "Kata|Targeting")
    AActor* GetSoftTarget() const { return SoftTarget.Get(); }

    /** 락온 지점을 가진 액터. 액션 대상으로 쓰인다. 락온이 없으면 nullptr이다. */
    UFUNCTION(BlueprintPure, Category = "Kata|Targeting")
    AActor* GetLockTarget() const;

    UFUNCTION(BlueprintPure, Category = "Kata|Targeting")
    UKataTargetPointComponent* GetLockPoint() const { return LockPoint.Get(); }

    UFUNCTION(BlueprintPure, Category = "Kata|Targeting")
    bool IsLocked() const { return LockPoint.IsValid(); }

    /** Soft Target Preset을 즉시 실행해 소프트 타겟을 갱신하고 돌려준다. 후보가 없으면 소프트 타겟을 비운다. */
    UFUNCTION(BlueprintCallable, Category = "Kata|Targeting")
    AActor* UpdateSoftTarget();

    /** Lock On Preset의 가장 우선하는 지점으로 락온한다. 후보가 없으면 현재 상태를 바꾸지 않고 false를 반환한다. */
    UFUNCTION(BlueprintCallable, Category = "Kata|Targeting")
    bool AcquireLock();

    /** 락온 중일 때 왼쪽 전환 Preset의 가장 우선하는 지점으로 바꾼다. 후보가 없으면 현재 지점을 유지하고 false를 반환한다. */
    UFUNCTION(BlueprintCallable, Category = "Kata|Targeting")
    bool SwitchLockLeft();

    /** 락온 중일 때 오른쪽 전환 Preset의 가장 우선하는 지점으로 바꾼다. 후보가 없으면 현재 지점을 유지하고 false를 반환한다. */
    UFUNCTION(BlueprintCallable, Category = "Kata|Targeting")
    bool SwitchLockRight();

    UFUNCTION(BlueprintCallable, Category = "Kata|Targeting")
    void ReleaseLock();

    /**
     * Lock On Preset을 즉시 실행해 지금 락온할 수 있는 지점을 우선순위 순서로 채운다. 락온 상태는 바꾸지 않는다.
     * 디버그 표시와 HUD 같은 조회용이며, 호출할 때마다 Preset을 실행하므로 매 프레임 부르지 않는다.
     */
    void GetLockOnCandidates(TArray<UKataTargetPointComponent*>& OutPoints) const;

    /**
     * Soft Target Preset을 즉시 실행해 소프트 타겟 후보 액터를 우선순위 순서로 채운다. 소프트 타겟은 바꾸지 않는다.
     * 첫 항목이 지금 UpdateSoftTarget()을 부르면 고를 대상이다. GetLockOnCandidates()처럼 조회용이며 매 프레임 부르지 않는다.
     */
    void GetSoftTargetCandidates(TArray<AActor*>& OutTargets) const;

    virtual AActor* GetCurrentTarget_Implementation() const override;

    /**
     * 락온 중이면 락온 지점의 액터를 돌려준다. 락온이 없고 이동 입력이 있으면 소프트 타겟을 비우고 nullptr을 돌려준다.
     * 둘 다 없으면 소프트 타겟을 갱신해 돌려준다.
     */
    virtual AActor* ResolveActionTarget_Implementation() override;

    /** 락온 중이면 이어받은 대상이 락온 지점의 액터일 때만, 락온이 없으면 이동 입력이 없을 때만 true다. */
    virtual bool CanKeepActionTarget_Implementation(AActor* CurrentTarget) const override;

    /** 락온 지점 위치 → 이동 입력 방향 → ActionTarget 순서로 방향을 정한다. */
    virtual bool ResolveFacingDirection_Implementation(AActor* ActionTarget, FVector& OutDirection) const override;

    /** 소유 폰의 수평 이동 입력 방향을 돌려준다. 락온 여부와 관계없이 입력 그대로다. */
    virtual bool ResolveMoveDirection_Implementation(FVector& OutDirection) const override;

    /** 락온 지점이 있으면 true다. */
    virtual bool IsLockOnActive_Implementation() const override;

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    /** 락온 지점을 바꾸는 단일 경로. 구독, Tick 켜고 끄기, Status 태그, 변경 알림을 함께 처리한다. */
    void SetLockPoint(UKataTargetPointComponent* NewPoint);

    bool SwitchLock(const UTargetingPreset* Preset);

    /** 지점이 켜져 있고 거리와 대상 태그 조건을 만족하는지. 지점과 소유 액터의 파괴 여부도 함께 본다. */
    bool IsLockPointValid(const UKataTargetPointComponent* Point) const;

    /**
     * 락온 지점의 시야를 판정하고 가려진 시작 시각을 갱신한다.
     * @return 시야 조건을 쓰지 않거나, 보이거나, 가려진 시간이 아직 유예 시간보다 짧으면 true.
     */
    bool UpdateLockLineOfSight(const UKataTargetPointComponent* Point);

    /** 락온 지점을 잃었을 때 Behavior를 적용한다. */
    void HandleLockLost(EKataLockLostBehavior Behavior);

    /** 새 지점에 맞춰 Status 태그를 옮긴다. 같은 액터의 지점끼리 바뀌면 대상 태그를 그대로 둔다. */
    void UpdateStatusTags(AActor* NewTargetActor);

    /** 붙였던 Status 태그를 모두 뗀다. */
    void ClearStatusTags();

    /**
     * 소유 폰의 수평 이동 입력 방향. 이번 프레임에 쌓인 입력을 먼저 보고, 없으면 직전 프레임에 소비된 입력을 본다.
     * 폰이 아니거나 입력이 없으면 false다. 폰이 이동 입력을 무시하는 동안에는 입력이 쌓이지 않으므로 false가 된다.
     */
    bool GetMoveInputDirection(FVector& OutDirection) const;

    UFUNCTION()
    void HandleLockTargetEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason);

    UFUNCTION()
    void HandleLockPointEnabledChanged(UKataTargetPointComponent* Point, bool bEnabled);

    /** 실행 상태. 저장하지 않으며 대상 수명에 관여하지 않도록 약한 참조로 둔다. */
    TWeakObjectPtr<AActor> SoftTarget;

    TWeakObjectPtr<UKataTargetPointComponent> LockPoint;

    /** 구독을 해제하기 위해 기록한 지점의 소유 액터. 지점이 먼저 사라져도 액터 구독을 정리할 수 있게 따로 둔다. */
    TWeakObjectPtr<AActor> LockPointOwner;

    /** 락온 지점이 가려지기 시작한 월드 시각. 보이는 동안과 지점이 바뀔 때는 음수다. */
    double LineOfSightBlockedStartTime = -1.0;

    /** TargetedStatusTag를 붙인 액터와 그때 쓴 태그. 설정이 바뀌어도 붙인 태그를 정확히 뗀다. */
    TWeakObjectPtr<AActor> TaggedTargetActor;
    FGameplayTag AppliedTargetedTag;
    FGameplayTag AppliedLockingTag;
};
