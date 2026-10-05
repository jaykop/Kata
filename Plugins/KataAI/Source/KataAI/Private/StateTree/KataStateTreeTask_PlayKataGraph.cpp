#include "StateTree/KataStateTreeTask_PlayKataGraph.h"

#include "StateTreeExecutionContext.h"
#include "StateTree/KataStateTreeExecutionUtils.h"

FKataStateTreeTask_PlayKataGraph::FKataStateTreeTask_PlayKataGraph()
{
    // 진입 시 입력과 반환된 실행을 고정해 대상 변경이나 바인딩 복사가 정리 대상을 바꾸지 않게 한다.
    bShouldCopyBoundPropertiesOnTick = false;
    bShouldCopyBoundPropertiesOnExitState = false;
#if WITH_EDITORONLY_DATA
    bConsideredForCompletion = true;
    bCanEditConsideredForCompletion = false;
#endif
}

EStateTreeRunStatus FKataStateTreeTask_PlayKataGraph::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
    FInstanceDataType& Data = Context.GetInstanceData(*this);
    UKataActionComponent* Component = nullptr;
    if (!KataStateTreeExecution::Prepare(Data, Component))
    {
        return EStateTreeRunStatus::Failed;
    }
    return KataStateTreeExecution::PlayGraph(Data, Data.Graph, Data.EntryTrigger);
}

EStateTreeRunStatus FKataStateTreeTask_PlayKataGraph::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
    return KataStateTreeExecution::Poll(Context.GetInstanceData(*this));
}

void FKataStateTreeTask_PlayKataGraph::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
    KataStateTreeExecution::Cleanup(Context.GetInstanceData(*this));
}

