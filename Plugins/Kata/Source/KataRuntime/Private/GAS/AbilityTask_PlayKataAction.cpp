#include "GAS/AbilityTask_PlayKataAction.h"

#include "AbilitySystemComponent.h"
#include "Action/KataAction.h"
#include "Abilities/GameplayAbility.h"
#include "GameFramework/Actor.h"
#include "KataRuntimeLog.h"
#include "Runtime/KataActionComponent.h"
#include "Runtime/KataActionInstance.h"

UAbilityTask_PlayKataAction* UAbilityTask_PlayKataAction::PlayKataAction(UGameplayAbility* OwningAbility, UKataAction* Action, AActor* TargetActor)
{
    UAbilityTask_PlayKataAction* Task = NewAbilityTask<UAbilityTask_PlayKataAction>(OwningAbility);
    Task->Action = Action;
    Task->TargetActor = TargetActor;
    return Task;
}

void UAbilityTask_PlayKataAction::Activate()
{
    Super::Activate();

    AActor* Avatar = GetAvatarActor();
    UKataActionComponent* ActionComponent = Avatar != nullptr ? Avatar->FindComponentByClass<UKataActionComponent>() : nullptr;
    if (ActionComponent == nullptr)
    {
        UE_LOG(LogKata, Warning, TEXT("PlayKata ability task requires a UKataActionComponent on the avatar actor"));
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

    UKataActionInstance* Instance = nullptr;
    const EKataStartResult Result = ActionComponent->PlayKataAction(Action, Context, Instance);
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
        // 순간 타임라인뿐 아니라 PreCommands나 시각 0 태스크가 중단시킨 경우도 반환 전에 끝난다.
        // 구독 전에 끝났으므로 인스턴스에 기록된 실제 종료 사유로 알린다.
        FinishWithEndReason(Instance->GetEndReason());
        return;
    }

    ActionInstance = Instance;
    Instance->OnKataEnded.AddDynamic(this, &UAbilityTask_PlayKataAction::HandleKataEnded);
}

void UAbilityTask_PlayKataAction::HandleKataEnded(UKataActionInstance* Instance, EKataEndReason EndReason)
{
    FinishWithEndReason(EndReason);
}

void UAbilityTask_PlayKataAction::FinishWithEndReason(EKataEndReason EndReason)
{
    if (ShouldBroadcastAbilityTaskDelegates())
    {
        if (EndReason == EKataEndReason::Completed)
        {
            OnCompleted.Broadcast(EndReason);
        }
        else if (EndReason == EKataEndReason::Branched)
        {
            OnBranched.Broadcast(EndReason);
        }
        else
        {
            OnInterrupted.Broadcast(EndReason);
        }
    }

    EndTask();
}

void UAbilityTask_PlayKataAction::ExternalCancel()
{
    if (IsValid(ActionInstance))
    {
        // Ability 취소는 Kata 중단으로 연결한다. 실제 정리는 인스턴스가 한 번만 수행한다.
        ActionInstance->RequestEnd(EKataEndReason::Cancelled);
    }

    Super::ExternalCancel();
}

void UAbilityTask_PlayKataAction::OnDestroy(bool bInOwnerFinished)
{
    if (IsValid(ActionInstance))
    {
        ActionInstance->OnKataEnded.RemoveDynamic(this, &UAbilityTask_PlayKataAction::HandleKataEnded);

        if (bInOwnerFinished && ActionInstance->IsRunning())
        {
            // Ability가 먼저 끝나면 Kata도 함께 정리한다.
            ActionInstance->RequestEnd(EKataEndReason::Interrupted);
        }
    }
    ActionInstance = nullptr;

    Super::OnDestroy(bInOwnerFinished);
}
