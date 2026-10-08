#include "StateTree/KataStateTreeExecutionUtils.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Pawn.h"
#include "KataGraphComponent.h"
#include "KataGraphInstance.h"
#include "Runtime/KataActionComponent.h"
#include "Runtime/KataActionInstance.h"
#include "StateTreePropertyBindings.h"

namespace KataStateTreeExecution
{
    bool Prepare(FKataStateTreeExecutionData& Data, UKataActionComponent*& OutComponent)
    {
        Cleanup(Data);
        APawn* Pawn = Data.Pawn;
        AActor* Target = Data.TargetActor;
        Data = FKataStateTreeExecutionData();
        Data.Pawn = Pawn;
        Data.TargetActor = Target;
        Data.ExecutionPawn = Pawn;
        OutComponent = nullptr;
        const IAbilitySystemInterface* AbilityOwner = Cast<IAbilitySystemInterface>(Pawn);
        const UAbilitySystemComponent* ASC = AbilityOwner != nullptr ? AbilityOwner->GetAbilitySystemComponent() : nullptr;
        if (!IsValid(Pawn) || !IsValid(ASC) || ASC->GetAvatarActor() != Pawn)
        {
            Data.Result = EKataStateTreeExecutionResult::InvalidSetup;
            return false;
        }
        OutComponent = Pawn->FindComponentByClass<UKataActionComponent>();
        if (OutComponent == nullptr)
        {
            Data.Result = EKataStateTreeExecutionResult::InvalidSetup;
            return false;
        }
        const UKataGraphComponent* GraphComponent = Pawn->FindComponentByClass<UKataGraphComponent>();
        if (OutComponent->IsPlayingKata() || (GraphComponent != nullptr && GraphComponent->IsRunningGraph()))
        {
            Data.Result = EKataStateTreeExecutionResult::Busy;
            return false;
        }
        return true;
    }

    EStateTreeRunStatus PlayAction(FKataStateTreeExecutionData& Data, UKataAction* Action)
    {
        APawn* Pawn = Data.ExecutionPawn.Get();
        UKataActionComponent* Component = Pawn != nullptr ? Pawn->FindComponentByClass<UKataActionComponent>() : nullptr;
        if (Component == nullptr)
        {
            Data.Result = EKataStateTreeExecutionResult::InvalidSetup;
            return EStateTreeRunStatus::Failed;
        }
        UKataActionInstance* Instance = nullptr;
        Data.Details.StartResult = Component->PlayKataActionOnSelf(Action, Data.TargetActor, Instance);
        Data.Details.bHasStartResult = true;
        Data.Details.ActionInstance = Instance;
        if (Data.Details.StartResult != EKataStartResult::Started || !IsValid(Instance))
        {
            Data.Result = EKataStateTreeExecutionResult::StartRejected;
            return EStateTreeRunStatus::Failed;
        }
        return Poll(Data);
    }

    EStateTreeRunStatus PlayGraph(FKataStateTreeExecutionData& Data, UKataGraph* Graph, FGameplayTag EntryTrigger)
    {
        APawn* Pawn = Data.ExecutionPawn.Get();
        UKataGraphComponent* Component = Pawn != nullptr ? Pawn->FindComponentByClass<UKataGraphComponent>() : nullptr;
        UKataGraphInstance* Instance = nullptr;
        if (Component == nullptr || !Component->StartGraphOnSelf(Graph, Data.TargetActor, Instance) || !IsValid(Instance))
        {
            Data.Result = EKataStateTreeExecutionResult::InvalidSetup;
            return EStateTreeRunStatus::Failed;
        }
        Data.Details.GraphInstance = Instance;
        if (Instance->GetState() == EKataGraphInstanceState::WaitingForEntry && EntryTrigger.IsValid())
        {
            Instance->SendTrigger(EntryTrigger);
        }
        Data.Details.bHasStartResult = Instance->HasActionStartResult();
        if (Data.Details.bHasStartResult)
        {
            Data.Details.StartResult = Instance->GetLastActionStartResult();
        }
        if (Instance->GetState() == EKataGraphInstanceState::WaitingForEntry || !Instance->HasStartedAction())
        {
            Data.Result = Data.Details.bHasStartResult
                ? EKataStateTreeExecutionResult::StartRejected : EKataStateTreeExecutionResult::InvalidSetup;
            Cleanup(Data);
            return EStateTreeRunStatus::Failed;
        }
        return Poll(Data);
    }

    EStateTreeRunStatus Poll(FKataStateTreeExecutionData& Data)
    {
        if (!Data.ExecutionPawn.IsValid())
        {
            Cleanup(Data, EKataEndReason::OwnerInvalid);
            Data.Result = EKataStateTreeExecutionResult::Interrupted;
            return EStateTreeRunStatus::Failed;
        }
        if (IsValid(Data.Details.GraphInstance))
        {
            if (Data.Details.GraphInstance->IsRunning())
            {
                Data.Result = EKataStateTreeExecutionResult::Running;
                return EStateTreeRunStatus::Running;
            }
            Data.Details.EndReason = Data.Details.GraphInstance->GetEndReason();
        }
        else if (IsValid(Data.Details.ActionInstance))
        {
            if (Data.Details.ActionInstance->IsRunning())
            {
                Data.Result = EKataStateTreeExecutionResult::Running;
                return EStateTreeRunStatus::Running;
            }
            Data.Details.EndReason = Data.Details.ActionInstance->GetEndReason();
        }
        else
        {
            Data.Result = EKataStateTreeExecutionResult::InvalidSetup;
            return EStateTreeRunStatus::Failed;
        }
        Data.Details.bHasEndReason = true;
        const bool bCompleted = Data.Details.EndReason == EKataEndReason::Completed;
        Data.Result = bCompleted ? EKataStateTreeExecutionResult::Completed : EKataStateTreeExecutionResult::Interrupted;
        return bCompleted ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Failed;
    }

    void Cleanup(FKataStateTreeExecutionData& Data, EKataEndReason Reason)
    {
        // 현재 컴포넌트의 실행을 멈추지 않고 이 Task가 받은 인스턴스만 정리한다.
        if (IsValid(Data.Details.GraphInstance))
        {
            if (Data.Details.GraphInstance->IsRunning())
            {
                Data.Details.GraphInstance->RequestEnd(Reason);
                if (Data.Result == EKataStateTreeExecutionResult::Running)
                {
                    Data.Result = EKataStateTreeExecutionResult::Interrupted;
                }
            }
            Data.Details.bHasEndReason = true;
            Data.Details.EndReason = Data.Details.GraphInstance->GetEndReason();
        }
        else if (IsValid(Data.Details.ActionInstance))
        {
            if (Data.Details.ActionInstance->IsRunning())
            {
                Data.Details.ActionInstance->RequestEnd(Reason);
                if (Data.Result == EKataStateTreeExecutionResult::Running)
                {
                    Data.Result = EKataStateTreeExecutionResult::Interrupted;
                }
            }
            Data.Details.bHasEndReason = true;
            Data.Details.EndReason = Data.Details.ActionInstance->GetEndReason();
        }
        Data.Details.ActionInstance = nullptr;
        Data.Details.GraphInstance = nullptr;
        Data.ExecutionPawn.Reset();
    }

#if WITH_EDITOR
    FText DescribeInput(const FGuid& ID, FName MemberName, const FText& ValueText,
        const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting)
    {
        const FText BoundText = BindingLookup.GetBindingSourceDisplayName(FPropertyBindingPath(ID, MemberName), Formatting);
        return BoundText.IsEmpty() ? ValueText : BoundText;
    }

    FText DescribeAsset(const UObject* Asset)
    {
        return Asset != nullptr ? FText::FromString(Asset->GetName()) : FText::FromString(TEXT("None"));
    }
#endif
}
