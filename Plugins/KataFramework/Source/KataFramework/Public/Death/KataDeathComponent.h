#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "Engine/HitResult.h"
#include "Engine/TimerHandle.h"
#include "KataDeathComponent.generated.h"

class AController;
class APawn;
class UAbilitySystemComponent;
class UKataAttributeSet_Base;
class UKataDeathComponent;
struct FGameplayEffectSpec;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FKataDeathSignature, UKataDeathComponent*, DeathComponent, AActor*, DeathInstigator);
DECLARE_MULTICAST_DELEGATE_OneParam(FKataDeathNativeSignature, UKataDeathComponent* /*DeathComponent*/);

/**
 * 시체 제거를 대신 처리하는 함수. 처리했으면 true를 돌려준다.
 * false를 돌려주거나 바인딩된 객체가 사라졌으면 UKataDeathComponent가 소유자를 Destroy한다.
 */
DECLARE_DELEGATE_RetVal_OneParam(bool, FKataDeathRemovalHandler, UKataDeathComponent* /*DeathComponent*/);

/**
 * Health가 0이 된 Actor를 죽은 상태로 확정하고, 정리와 사망 이벤트, 시체 제거를 맡는 공용 컴포넌트.
 *
 * 소유자의 ASC와 UKataAttributeSet_Base만 있으면 동작하므로 캐릭터가 아닌 피해 대상(파괴 가능한 오브젝트 등)에도 붙일 수 있다.
 * BeginPlay에서 Base 세트의 OnOutOfHealth를 구독하며, 그 전에 세트가 ASC에 추가되어 있어야 한다. ASC나 세트가 없으면 아무것도 하지 않는다.
 *
 * 처리 순서는 다음과 같다.
 *   1. Health 0 또는 Kill: 죽은 상태로 표시하고 Kata Combat 설정의 Dead Status Tag를 Loose Tag로 붙인다.
 *   2. 다음 틱: 모든 Ability를 취소하고 소유자의 Hurt Box 충돌을 끈 뒤 OnDeathCleanup, OnDeathNative, OnDeath 순서로 알린다.
 *      이어서 Death Event Tag로 Gameplay Event를 보낸다. 활성화된 Ability가 없으면 연출 없이 바로 제거를 요청한다.
 *   3. 사망 Ability(UKataDeathAbility)가 연출을 마치면 RequestRemoval을 호출한다.
 * 제거는 RemovalHandler가 있으면 맡기고, 없으면 다음 틱에 소유자를 Destroy한다. 플레이어가 조종 중인 Pawn은 빙의가 풀릴 때까지 미룬다.
 * 같은 Actor를 되살리는 부활은 지원하지 않는다.
 */
UCLASS(ClassGroup = (Kata), meta = (BlueprintSpawnableComponent, DisplayName = "Kata Death"))
class KATAFRAMEWORK_API UKataDeathComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UKataDeathComponent();

    /** 죽은 상태면 true. Health 0이나 Kill 직후부터 true이며, 정리와 연출은 그 뒤에 진행된다. */
    UFUNCTION(BlueprintPure, Category = "Kata|Death")
    bool IsDead() const { return bDead; }

    /** 마지막 피해를 준 Instigator. Kill로 죽었으면 Kill에 넘긴 값이다. 없거나 이미 사라졌으면 nullptr. */
    UFUNCTION(BlueprintPure, Category = "Kata|Death")
    AActor* GetDeathInstigator() const { return DeathInstigator.Get(); }

    /** 마지막 피해 GE의 HitResult. 피해 GE에 HitResult가 없었거나 Kill로 죽었으면 비어 있다. */
    UFUNCTION(BlueprintPure, Category = "Kata|Death")
    const FHitResult& GetDeathHitResult() const { return DeathHitResult; }

    /**
     * GE 없이 즉시 죽인다. Health를 0으로 맞추고 Health 0과 같은 사망 처리로 들어간다.
     * @return 사망 처리를 시작했으면 true. 이미 죽었거나 구독 전(BeginPlay 이전·ASC 없음)이면 false.
     */
    UFUNCTION(BlueprintCallable, Category = "Kata|Death")
    bool Kill(AActor* InInstigator);

    /**
     * 시체 제거를 요청한다. 사망 Ability가 연출을 마친 뒤 호출하며, 같은 사망에서 두 번째 요청부터는 무시한다.
     * 죽지 않은 상태에서는 아무것도 하지 않는다. 실제 제거는 다음 틱 이후에 일어나므로 호출 중 소유자가 사라지지 않는다.
     */
    UFUNCTION(BlueprintCallable, Category = "Kata|Death")
    void RequestRemoval();

    /** 제거를 요청했으면 true. 플레이어 Pawn처럼 제거가 미뤄진 동안에도 true다. */
    UFUNCTION(BlueprintPure, Category = "Kata|Death")
    bool IsRemovalRequested() const { return bRemovalRequested; }

    /**
     * 사망 이벤트를 보내기 전에 소유자 전용 정리를 하라는 신호. 모든 Ability 취소와 Hurt Box 충돌 해제 뒤에 온다.
     * 캐릭터는 여기서 Kata·그래프 중단, 이동 정리, AI 정지를 한다.
     */
    FKataDeathNativeSignature OnDeathCleanup;

    /** OnDeathCleanup 다음에 오는 사망 알림의 C++ 버전. OnDeath보다 먼저 호출된다. */
    FKataDeathNativeSignature OnDeathNative;

    /** 사망 정리가 끝났다는 알림. 사망 이벤트를 보내기 직전에 한 번 온다. HUD나 게임 규칙을 여기에 연결한다. */
    UPROPERTY(BlueprintAssignable, Category = "Kata|Death")
    FKataDeathSignature OnDeath;

    /** 시체 제거를 대신 처리할 함수. 스포너가 자기가 만든 NPC의 Controller까지 정리하기 위해 바인딩한다. 한 번에 하나만 둔다. */
    FKataDeathRemovalHandler RemovalHandler;

protected:
    //~ Begin UActorComponent Interface
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    //~ End UActorComponent Interface

private:
    void HandleOutOfHealth(AActor* EffectInstigator, const FGameplayEffectSpec& EffectSpec);

    /** 죽은 상태 표시와 사망 태그 부여. GE 실행 도중에도 불리므로 정리는 다음 틱으로 미룬다. */
    void BeginDeath(AActor* InInstigator, const FHitResult* HitResult);

    /** 다음 틱에 실행하는 정리, 알림, 사망 이벤트 발송. */
    void ProcessDeath();

    /** 플레이어 빙의가 풀릴 때까지 기다린 뒤 제거를 진행한다. */
    void TryRemove();

    /** RemovalHandler 또는 Destroy로 실제 제거를 한다. */
    void PerformRemoval();

    UFUNCTION()
    void HandleOwnerControllerChanged(APawn* Pawn, AController* OldController, AController* NewController);

    /** 소유자가 플레이어가 조종 중인 Pawn이면 그 Pawn을 돌려준다. */
    APawn* GetPlayerControlledOwnerPawn() const;

    TWeakObjectPtr<UAbilitySystemComponent> AbilitySystem;
    TWeakObjectPtr<const UKataAttributeSet_Base> BaseSet;
    FDelegateHandle OutOfHealthHandle;

    TWeakObjectPtr<AActor> DeathInstigator;
    FHitResult DeathHitResult;

    FTimerHandle ProcessDeathTimer;
    FTimerHandle RemovalTimer;

    bool bDead = false;
    bool bHasDeathHitResult = false;
    bool bDeathProcessed = false;
    bool bRemovalRequested = false;
    bool bRemovalPerformed = false;
    bool bWaitingForPlayerRelease = false;
};
