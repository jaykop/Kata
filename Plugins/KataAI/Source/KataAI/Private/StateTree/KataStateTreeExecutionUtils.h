#pragma once

#include "StateTree/KataStateTreeExecutionTypes.h"

class UKataAction;
class UKataGraph;
class UKataActionComponent;

namespace KataStateTreeExecution
{
    bool Prepare(FKataStateTreeExecutionData& Data, UKataActionComponent*& OutComponent);
    EStateTreeRunStatus PlayAction(FKataStateTreeExecutionData& Data, UKataAction* Action);
    EStateTreeRunStatus PlayGraph(FKataStateTreeExecutionData& Data, UKataGraph* Graph, FGameplayTag EntryTrigger);
    EStateTreeRunStatus Poll(FKataStateTreeExecutionData& Data);
    void Cleanup(FKataStateTreeExecutionData& Data, EKataEndReason Reason = EKataEndReason::Cancelled);
}
