#include "GAS/AbilityTask_PlayKata.h"

#include "AbilitySystemComponent.h"
#include "Definition/KataAsset.h"
#include "Abilities/GameplayAbility.h"
#include "GameFramework/Actor.h"
#include "KataRuntimeLog.h"
#include "Runtime/KataComponent.h"
#include "Runtime/KataInstance.h"

UAbilityTask_PlayKata* UAbilityTask_PlayKata::PlayKataAsset(UGameplayAbility* OwningAbility, UKataAsset* Asset, AActor* TargetActor)
{
    UAbilityTask_PlayKata* Task = NewAbilityTask<UAbilityTask_PlayKata>(OwningAbility);
    Task->Asset = Asset;
    Task->bUseAsset = true;
    Task->TargetActor = TargetActor;
    return Task;
}

UAbilityTask_PlayKata* UAbilityTask_PlayKata::PlayKata(UGameplayAbility* OwningAbility, TSubclassOf<UKataDefinition> DefinitionClass, AActor* TargetActor)
{
    UAbilityTask_PlayKata* Task = NewAbilityTask<UAbilityTask_PlayKata>(OwningAbility);
    Task->DefinitionClass = DefinitionClass;
    Task->TargetActor = TargetActor;
    return Task;
}

void UAbilityTask_PlayKata::Activate()
{
    Super::Activate();

    AActor* Avatar = GetAvatarActor();
    UKataComponent* KataComponent = Avatar != nullptr ? Avatar->FindComponentByClass<UKataComponent>() : nullptr;
    if (KataComponent == nullptr)
    {
        UE_LOG(LogKata, Warning, TEXT("PlayKata ability task requires a UKataComponent on the avatar actor"));
        if (ShouldBroadcastAbilityTaskDelegates())
        {
            OnFailed.Broadcast(EKataStartResult::InvalidContext);
        }
        EndTask();
        return;
    }

    UGameplayAbility* OwningAbility = Ability;

    FKataContext Context;
    Context.OwnerActor = Avatar;
    Context.AvatarActor = Avatar;
    Context.TargetActor = TargetActor.Get();
    Context.AbilitySystem = AbilitySystemComponent.Get();
    Context.OwningAbility = OwningAbility;

    UKataInstance* Instance = nullptr;
    const EKataStartResult Result = bUseAsset
        ? KataComponent->PlayKataAsset(Asset, Context, Instance)
        : KataComponent->PlayKata(DefinitionClass, Context, Instance);
    if (Result != EKataStartResult::Started || Instance == nullptr)
    {
        if (ShouldBroadcastAbilityTaskDelegates())
        {
            OnFailed.Broadcast(Result);
        }
        EndTask();
        return;
    }

    if (!Instance->IsRunning())
    {
        // 순간 타임라인은 시작과 같은 프레임에 끝난다. 종료 알림을 바로 보낸다.
        if (ShouldBroadcastAbilityTaskDelegates())
        {
            OnCompleted.Broadcast(EKataEndReason::Completed);
        }
        EndTask();
        return;
    }

    KataInstance = Instance;
    Instance->OnKataEnded.AddDynamic(this, &UAbilityTask_PlayKata::HandleKataEnded);
}

void UAbilityTask_PlayKata::HandleKataEnded(UKataInstance* Instance, EKataEndReason EndReason)
{
    if (ShouldBroadcastAbilityTaskDelegates())
    {
        if (EndReason == EKataEndReason::Completed)
        {
            OnCompleted.Broadcast(EndReason);
        }
        else
        {
            OnInterrupted.Broadcast(EndReason);
        }
    }

    EndTask();
}

void UAbilityTask_PlayKata::ExternalCancel()
{
    if (IsValid(KataInstance))
    {
        // Ability 취소는 Kata 중단으로 연결한다. 실제 정리는 인스턴스가 한 번만 수행한다.
        KataInstance->RequestEnd(EKataEndReason::Cancelled);
    }

    Super::ExternalCancel();
}

void UAbilityTask_PlayKata::OnDestroy(bool bInOwnerFinished)
{
    if (IsValid(KataInstance))
    {
        KataInstance->OnKataEnded.RemoveDynamic(this, &UAbilityTask_PlayKata::HandleKataEnded);

        if (bInOwnerFinished && KataInstance->IsRunning())
        {
            // Ability가 먼저 끝나면 Kata도 함께 정리한다.
            KataInstance->RequestEnd(EKataEndReason::Interrupted);
        }
    }
    KataInstance = nullptr;

    Super::OnDestroy(bInOwnerFinished);
}
