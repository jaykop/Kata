#include "Death/KataDeathComponent.h"

#include "Abilities/GameplayAbilityTypes.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Attributes/KataAttributeSet_Base.h"
#include "Attributes/KataCombatSettings.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameplayEffect.h"
#include "HitTrace/KataHurtBoxComponent.h"
#include "KataFrameworkLog.h"
#include "TimerManager.h"

UKataDeathComponent::UKataDeathComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UKataDeathComponent::BeginPlay()
{
    Super::BeginPlay();

    // AKataCharacter는 PostInitializeComponents에서 행의 Gameplay Data로 Base 세트를 추가하므로 BeginPlay에는 세트가 있다.
    UAbilitySystemComponent* FoundAbilitySystem = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
    const UKataAttributeSet_Base* FoundBaseSet = FoundAbilitySystem != nullptr ? FoundAbilitySystem->GetSet<UKataAttributeSet_Base>() : nullptr;
    if (FoundBaseSet == nullptr)
    {
        // 프리뷰 월드처럼 Gameplay Data를 적용하지 않은 액터에서도 정상 경로이므로 경고하지 않는다.
        UE_LOG(LogKataFramework, Verbose, TEXT("Kata death component on '%s' is inactive: no ability system or Base attribute set."),
            *GetNameSafe(GetOwner()));
        return;
    }

    AbilitySystem = FoundAbilitySystem;
    BaseSet = FoundBaseSet;
    OutOfHealthHandle = FoundBaseSet->OnOutOfHealth.AddUObject(this, &UKataDeathComponent::HandleOutOfHealth);
}

void UKataDeathComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (const UKataAttributeSet_Base* Set = BaseSet.Get())
    {
        Set->OnOutOfHealth.Remove(OutOfHealthHandle);
    }
    OutOfHealthHandle.Reset();
    BaseSet.Reset();
    AbilitySystem.Reset();

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(ProcessDeathTimer);
        World->GetTimerManager().ClearTimer(RemovalTimer);
    }
    if (bWaitingForPlayerRelease)
    {
        if (APawn* Pawn = Cast<APawn>(GetOwner()))
        {
            Pawn->ReceiveControllerChangedDelegate.RemoveDynamic(this, &UKataDeathComponent::HandleOwnerControllerChanged);
        }
        bWaitingForPlayerRelease = false;
    }
    RemovalHandler.Unbind();

    Super::EndPlay(EndPlayReason);
}

bool UKataDeathComponent::Kill(AActor* InInstigator)
{
    UAbilitySystemComponent* OwnerAbilitySystem = AbilitySystem.Get();
    if (bDead || OwnerAbilitySystem == nullptr || !BaseSet.IsValid())
    {
        return false;
    }

    // 기본값을 0으로 맞추므로 Health를 올리는 지속 효과가 걸려 있으면 현재값은 0보다 클 수 있다. 사망 판정은 이 함수가 직접 시작한다.
    OwnerAbilitySystem->SetNumericAttributeBase(UKataAttributeSet_Base::GetHealthAttribute(), 0.0f);
    BeginDeath(InInstigator, nullptr);
    return true;
}

void UKataDeathComponent::HandleOutOfHealth(AActor* EffectInstigator, const FGameplayEffectSpec& EffectSpec)
{
    BeginDeath(EffectInstigator, EffectSpec.GetEffectContext().GetHitResult());
}

void UKataDeathComponent::BeginDeath(AActor* InInstigator, const FHitResult* HitResult)
{
    if (bDead)
    {
        return;
    }
    bDead = true;
    DeathInstigator = InInstigator;
    bHasDeathHitResult = HitResult != nullptr;
    DeathHitResult = HitResult != nullptr ? *HitResult : FHitResult();

    // 같은 프레임에 이어지는 피해 GE가 태그 요구 조건으로 막히도록 태그만은 즉시 붙인다.
    const FGameplayTag& DeadTag = UKataCombatSettings::Get()->DeadStatusTag;
    if (UAbilitySystemComponent* OwnerAbilitySystem = AbilitySystem.Get(); OwnerAbilitySystem != nullptr && DeadTag.IsValid())
    {
        OwnerAbilitySystem->AddLooseGameplayTag(DeadTag);
    }
    else if (!DeadTag.IsValid())
    {
        UE_LOG(LogKataFramework, Warning, TEXT("Kata death on '%s' has no Dead Status Tag in the Kata Combat settings. Damage and targeting cannot exclude it."),
            *GetNameSafe(GetOwner()));
    }

    // GE 실행 도중일 수 있으므로 Ability 취소와 이벤트 발송은 다음 틱에 한다.
    if (UWorld* World = GetWorld())
    {
        ProcessDeathTimer = World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &UKataDeathComponent::ProcessDeath));
    }
}

void UKataDeathComponent::ProcessDeath()
{
    ProcessDeathTimer.Invalidate();
    AActor* Owner = GetOwner();
    if (bDeathProcessed || !IsValid(Owner) || Owner->IsActorBeingDestroyed())
    {
        return;
    }
    bDeathProcessed = true;

    UAbilitySystemComponent* OwnerAbilitySystem = AbilitySystem.Get();
    if (OwnerAbilitySystem != nullptr)
    {
        OwnerAbilitySystem->CancelAllAbilities();
    }

    // 시체가 공격 판정, 접근 제한 같은 Hurt Box 질의에 걸리지 않게 한다.
    TInlineComponentArray<UKataHurtBoxComponent*> HurtBoxes(Owner);
    for (UKataHurtBoxComponent* HurtBox : HurtBoxes)
    {
        HurtBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    OnDeathCleanup.Broadcast(this);
    OnDeathNative.Broadcast(this);
    OnDeath.Broadcast(this, DeathInstigator.Get());

    // 알림을 받은 쪽이 소유자를 제거했거나 제거를 요청했을 수 있다.
    if (!IsValid(Owner) || Owner->IsActorBeingDestroyed() || bRemovalRequested)
    {
        return;
    }

    const FGameplayTag& EventTag = UKataCombatSettings::Get()->DeathEventTag;
    int32 ActivatedCount = 0;
    if (OwnerAbilitySystem != nullptr && EventTag.IsValid())
    {
        AActor* InstigatorActor = DeathInstigator.Get();
        FGameplayEffectContextHandle Context = OwnerAbilitySystem->MakeEffectContext();
        Context.AddInstigator(InstigatorActor, InstigatorActor);
        if (bHasDeathHitResult)
        {
            Context.AddHitResult(DeathHitResult);
        }

        FGameplayEventData Payload;
        Payload.EventTag = EventTag;
        Payload.Instigator = InstigatorActor;
        Payload.Target = Owner;
        Payload.ContextHandle = Context;
        ActivatedCount = OwnerAbilitySystem->HandleGameplayEvent(EventTag, &Payload);
    }

    if (ActivatedCount == 0)
    {
        // 사망 Ability가 없어도 죽은 대상이 영구히 남지 않게 한다.
        UE_LOG(LogKataFramework, Warning, TEXT("Kata death on '%s' activated no death ability for '%s'. Removing without presentation."),
            *GetNameSafe(Owner), *EventTag.ToString());
        RequestRemoval();
    }
}

void UKataDeathComponent::RequestRemoval()
{
    if (!bDead || bRemovalRequested)
    {
        return;
    }
    bRemovalRequested = true;
    TryRemove();
}

APawn* UKataDeathComponent::GetPlayerControlledOwnerPawn() const
{
    APawn* Pawn = Cast<APawn>(GetOwner());
    const AController* Controller = Pawn != nullptr ? Pawn->GetController() : nullptr;
    // Controller가 이미 다른 Pawn으로 넘어갔는데 이전 참조만 남은 경우는 조종 중으로 보지 않는다.
    return Controller != nullptr && Controller->IsPlayerController() && Controller->GetPawn() == Pawn ? Pawn : nullptr;
}

void UKataDeathComponent::TryRemove()
{
    if (bRemovalPerformed)
    {
        return;
    }

    // 플레이어가 조종 중인 Pawn을 지우면 PlayerController가 Pawn 없이 남는다. 재시작으로 빙의가 넘어간 뒤 제거한다.
    if (APawn* Pawn = GetPlayerControlledOwnerPawn())
    {
        if (!bWaitingForPlayerRelease)
        {
            bWaitingForPlayerRelease = true;
            Pawn->ReceiveControllerChangedDelegate.AddDynamic(this, &UKataDeathComponent::HandleOwnerControllerChanged);
        }
        return;
    }

    if (bWaitingForPlayerRelease)
    {
        if (APawn* Pawn = Cast<APawn>(GetOwner()))
        {
            Pawn->ReceiveControllerChangedDelegate.RemoveDynamic(this, &UKataDeathComponent::HandleOwnerControllerChanged);
        }
        bWaitingForPlayerRelease = false;
    }

    // Ability 종료나 델리게이트 안에서 호출되어도 호출자보다 소유자가 먼저 사라지지 않도록 다음 틱에 제거한다.
    if (UWorld* World = GetWorld(); World != nullptr && !RemovalTimer.IsValid())
    {
        RemovalTimer = World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &UKataDeathComponent::PerformRemoval));
    }
}

void UKataDeathComponent::HandleOwnerControllerChanged(APawn* Pawn, AController* OldController, AController* NewController)
{
    TryRemove();
}

void UKataDeathComponent::PerformRemoval()
{
    RemovalTimer.Invalidate();
    AActor* Owner = GetOwner();
    if (bRemovalPerformed || !IsValid(Owner) || Owner->IsActorBeingDestroyed())
    {
        return;
    }
    // 대기하는 한 틱 사이에 플레이어가 다시 빙의했으면 다시 기다린다.
    if (GetPlayerControlledOwnerPawn() != nullptr)
    {
        TryRemove();
        return;
    }
    bRemovalPerformed = true;

    if (RemovalHandler.IsBound() && RemovalHandler.Execute(this))
    {
        return;
    }
    if (!Owner->Destroy())
    {
        UE_LOG(LogKataFramework, Warning, TEXT("Kata death removal failed: '%s' refused Destroy."), *GetNameSafe(Owner));
    }
}
