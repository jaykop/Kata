#include "StateTree/KataStateTreeTask_ClearTargetMemory.h"

#include "GameFramework/Pawn.h"
#include "StateTreeExecutionContext.h"
#include "Targeting/KataAITargetingComponent.h"

EStateTreeRunStatus FKataStateTreeTask_ClearTargetMemory::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
    const FInstanceDataType& Data = Context.GetInstanceData(*this);
    UKataAITargetingComponent* Targeting = IsValid(Data.Pawn)
        ? Data.Pawn->FindComponentByClass<UKataAITargetingComponent>() : nullptr;
    if (Targeting == nullptr)
    {
        return EStateTreeRunStatus::Failed;
    }
    Targeting->ClearTargetMemory();
    return EStateTreeRunStatus::Succeeded;
}