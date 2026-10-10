#pragma once

#include "Abilities/GameplayAbility.h"
#include "CoreMinimal.h"
#include "KataRuntimeTypes.h"
#include "KataHitReactionAbility.generated.h"

class UAnimMontage;
class UKataAction;

/**
 * 맞은 쪽 기준으로 공격이 들어온 방향. 휘청이는 방향이 아니다.
 * 예를 들어 Left는 맞은 캐릭터의 왼쪽에서 공격이 들어왔다는 뜻이며, 반응 애니메이션은 보통 오른쪽으로 휘청인다.
 */
UENUM(BlueprintType)
enum class EKataHitDirection : uint8
{
    Front,
    Back,
    Left,
    Right
};

/** 반응을 재생하는 방식. */
UENUM(BlueprintType)
enum class EKataHitReactionMode : uint8
{
    /** 반응 Kata를 재생한다. 캐릭터의 액션 슬롯을 쓰므로 현재 액션은 교체 규칙에 따라 끊긴다. */
    KataAction,
    /** 가산 몽타주를 재생한다. 액션 슬롯을 쓰지 않으므로 현재 액션이 이어진다. 공격 몽타주와 다른 Slot Group에 둔다. */
    AdditiveMontage
};

/**
 * 반응 이벤트로 활성화되어 피격 반응을 재생하는 Gameplay Ability의 기반.
 *
 * 반응 종류마다 Blueprint 자식을 만들어 다음을 채운다.
 *   - Ability Triggers: Kata Combat 설정의 반응 이벤트 태그(Gameplay Event).
 *   - Activation Owned Tags: 해당 Status.HitReaction 태그. 반응이 재생되는 동안 붙는다.
 *   - Activation Blocked Tags: 이 반응을 막을 상태(예: 다운·그로기 중의 경직).
 *   - Mode와 방향별 반응 데이터.
 * 같은 반응이 다시 오면 진행 중인 반응을 끝내고 다시 시작한다. 반응이 끝나거나 끊기거나 시작이 거절되면 Ability도 끝난다.
 * 이벤트 Payload는 UKataHitReactionGameplayEffectComponent가 채운다. ContextHandle의 HitResult와 Instigator로 방향을 고른다.
 */
UCLASS(Abstract, Blueprintable, meta = (DisplayName = "Kata Hit Reaction Ability"))
class KATAFRAMEWORK_API UKataHitReactionAbility : public UGameplayAbility
{
    GENERATED_BODY()

public:
    UKataHitReactionAbility();

    /**
     * 맞은 쪽 기준으로 공격이 들어온 방향을 고른다.
     * HitResult의 TraceStart→TraceEnd(접촉한 서브스텝의 무기 이동)에서 수평 성분이 MinHorizontalRatio 이상이면 이동의 반대쪽을 피격 방향으로 본다.
     * 좌우도 맞은 쪽 기준이다. 무기가 맞은 쪽의 왼쪽에서 오른쪽으로 지나가면 왼쪽 피격이며, 마주 선 공격자 시점으로는 왼쪽에서 오른쪽으로 벤 공격이 오른쪽 피격이다. 수평 성분이 부족하거나(내려찍기) 이동이 없으면 공격자 위치로 고르고, 공격자도 없으면 Front다.
     */
    UFUNCTION(BlueprintPure, Category = "Kata|Hit Reaction")
    static EKataHitDirection ResolveHitDirection(const AActor* Victim, const FHitResult& HitResult, const AActor* Attacker, float MinHorizontalRatio);

protected:
    //~ Begin UGameplayAbility Interface
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
    //~ End UGameplayAbility Interface

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit Reaction")
    EKataHitReactionMode Mode = EKataHitReactionMode::KataAction;

    /** 방향별 반응 Kata. 반응 Kata는 현재 액션을 끊을 수 있도록 Blocking Policy의 Can Interrupt Active Kata를 켠다. 고른 방향이 비어 있으면 Front를 쓴다. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit Reaction",
        meta = (EditCondition = "Mode == EKataHitReactionMode::KataAction", EditConditionHides))
    TMap<EKataHitDirection, TObjectPtr<UKataAction>> DirectionalActions;

    /** 방향별 가산 몽타주. 고른 방향이 비어 있으면 Front를 쓴다. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit Reaction",
        meta = (EditCondition = "Mode == EKataHitReactionMode::AdditiveMontage", EditConditionHides))
    TMap<EKataHitDirection, TObjectPtr<UAnimMontage>> DirectionalMontages;

    /** 켜면 반응이 끝날 때 Groggy를 0으로 되돌린다. 그로기 반응에 쓴다. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit Reaction")
    bool bResetGroggyOnEnd = false;

private:
    UFUNCTION()
    void HandleKataEnded(EKataEndReason EndReason);

    UFUNCTION()
    void HandleKataFailed(EKataStartResult FailureReason);

    UFUNCTION()
    void HandleMontageEnded();

    /** 현재 활성화를 정상 종료로 끝낸다. 태스크 콜백에서 쓴다. */
    void FinishReaction();
};
