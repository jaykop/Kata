#pragma once

#include "Abilities/GameplayAbility.h"
#include "CoreMinimal.h"
#include "HitReaction/KataHitReactionAbility.h"
#include "KataRuntimeTypes.h"
#include "KataDeathAbility.generated.h"

class ACharacter;
class UKataAction;
class UKataDeathComponent;

/** 사망 Ability의 기본 연출 방식. */
UENUM(BlueprintType)
enum class EKataDeathPresentation : uint8
{
    /** 연출 없이 바로 시체 단계로 넘어간다. 캐릭터가 아닌 대상이나 연출을 직접 재정의할 때 쓴다. */
    None,
    /** 방향별 사망 Kata를 재생하고, 끝나면 이동을 멈춘 채 시체로 남는다. */
    Action,
    /** 사망 순간 Ragdoll로 바꾼다. */
    Ragdoll,
    /** 방향별 사망 Kata를 재생하고, 끝나는 순간 Ragdoll로 바꾼다. */
    ActionThenRagdoll
};

/**
 * 사망 이벤트로 활성화되어 사망 연출, 시체 유지, DimOut을 진행하고 마지막에 시체 제거를 요청하는 Gameplay Ability의 기반.
 *
 * Blueprint 자식에서 다음을 채우고 Gameplay Data의 Ability 목록으로 부여한다.
 *   - Ability Triggers: Kata Combat 설정의 Death Event Tag(Gameplay Event).
 *   - 연출 방식과 방향별 사망 Kata, 시체 유지 시간, DimOut 파라미터와 지속 시간.
 * 사망은 막히면 안 되므로 Activation Blocked Tags는 비워 둔다. 데이터 검증이 비어 있지 않은 자식을 오류로 알린다.
 * 연출 단계는 Start Presentation을 재정의해 바꿀 수 있다. 재정의한 연출은 끝날 때 Finish Presentation을 호출해야 시체 단계로 넘어간다.
 * 어떤 이유로 끝나든 제거 요청은 한 번 나간다. 시체 단계 전에 취소되면 DimOut 없이 바로 제거를 요청한다.
 */
UCLASS(Abstract, Blueprintable, meta = (DisplayName = "Kata Death Ability"))
class KATAFRAMEWORK_API UKataDeathAbility : public UGameplayAbility
{
    GENERATED_BODY()

public:
    UKataDeathAbility();

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

    /**
     * 캐릭터를 Ragdoll로 바꾼다. 캡슐 충돌과 CharacterMovement를 끄고 메시에 Kata Combat 설정의 Ragdoll 충돌 프로필을 적용한 뒤 물리 시뮬레이션을 켠다.
     * 메시에 Physics Asset이 없으면 경고만 남기고 이동만 멈춘다.
     */
    UFUNCTION(BlueprintCallable, Category = "Kata|Death")
    static void StartRagdoll(ACharacter* Character);

    /** 캐릭터의 이동을 즉시 멈추고 CharacterMovement를 끈다. Action 연출이 끝난 시체를 그 자리에 두는 데 쓴다. */
    UFUNCTION(BlueprintCallable, Category = "Kata|Death")
    static void StopCharacterMovement(ACharacter* Character);

protected:
    //~ Begin UGameplayAbility Interface
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
    //~ End UGameplayAbility Interface

    /**
     * 사망 연출을 시작한다. 기본 구현은 Presentation에 따라 Action·Ragdoll을 진행하고 끝나면 FinishPresentation을 호출한다.
     * Action·Ragdoll을 고른 Ability가 캐릭터가 아닌 대상에서 실행되면 경고를 남기고 None처럼 처리한다.
     * @param EventData 사망 이벤트 Payload. Instigator와 ContextHandle의 HitResult로 방향을 고를 수 있다.
     */
    UFUNCTION(BlueprintNativeEvent, Category = "Kata|Death")
    void StartPresentation(const FGameplayEventData& EventData);

    /** 연출이 끝났음을 알리고 시체 유지 단계로 넘어간다. 같은 활성화에서 두 번째 호출부터는 무시한다. */
    UFUNCTION(BlueprintCallable, Category = "Kata|Death")
    void FinishPresentation();

    /** Avatar의 사망 컴포넌트. 없으면 nullptr. */
    UFUNCTION(BlueprintPure, Category = "Kata|Death")
    UKataDeathComponent* GetDeathComponent() const;

    /** 기본 연출 방식. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death")
    EKataDeathPresentation Presentation = EKataDeathPresentation::Ragdoll;

    /**
     * 방향별 사망 Kata. 고른 방향이 비어 있으면 Front를 쓴다. 방향은 마지막 피해의 HitResult와 Instigator로 고른다.
     * 사망 Kata의 Activation Blocked Tags에 사망 상태 태그를 넣으면 재생이 거절되므로 넣지 않는다.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death",
        meta = (EditCondition = "Presentation == EKataDeathPresentation::Action || Presentation == EKataDeathPresentation::ActionThenRagdoll", EditConditionHides))
    TMap<EKataHitDirection, TObjectPtr<UKataAction>> DirectionalActions;

    /** 연출이 끝난 뒤 DimOut을 시작할 때까지 시체를 그대로 두는 시간(초). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death|Corpse", meta = (ClampMin = "0.0", Units = "s"))
    float CorpseLifetime = 5.0f;

    /** DimOut에서 0에서 1로 올릴 머티리얼 스칼라 파라미터. 비우면 DimOut 없이 유지 시간 뒤 바로 제거한다. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death|Corpse")
    FName DimOutParameterName = TEXT("DimOut");

    /** DimOut에 걸리는 시간(초). 0이면 DimOut 없이 바로 제거한다. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death|Corpse", meta = (ClampMin = "0.0", Units = "s"))
    float DimOutDuration = 1.0f;

private:
    UFUNCTION()
    void HandleActionEnded(EKataEndReason EndReason);

    UFUNCTION()
    void HandleActionFailed(EKataStartResult FailureReason);

    UFUNCTION()
    void HandleCorpseLifetimeEnded();

    UFUNCTION()
    void HandleDimOutFinished();

    /** 사망 Kata가 끝나거나 거절된 뒤 연출 방식에 맞게 Ragdoll 또는 이동 정지로 마무리한다. */
    void CompleteActionPresentation();

    void StartDimOut();

    /** 이번 활성화의 제거 요청을 한 번만 보낸다. */
    void RequestRemovalOnce();

    bool bPresentationFinished = false;
    bool bRemovalRequested = false;
};
