#include "StateTree/KataStateTreeTask_MoveRetry.h"

#include "GameFramework/Pawn.h"
#include "StateTreeExecutionContext.h"
#include "Targeting/KataAITargetingComponent.h"

EStateTreeRunStatus FKataStateTreeTask_MoveRetry::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
    FInstanceDataType& Data = Context.GetInstanceData(*this);
    UKataAITargetingComponent* Targeting = IsValid(Data.Pawn)
        ? Data.Pawn->FindComponentByClass<UKataAITargetingComponent>() : nullptr;
    if (Targeting == nullptr)
    {
        Data.RetryCount = 0;
        return EStateTreeRunStatus::Failed;
    }
    if (Data.Operation == EKataAIMoveRetryOperation::Increment)
    {
        Data.RetryCount = Targeting->IncrementMoveRetryCount();
    }
    else
    {
        Targeting->ResetMoveRetryCount();
        Data.RetryCount = 0;
    }
    return EStateTreeRunStatus::Succeeded;
}
