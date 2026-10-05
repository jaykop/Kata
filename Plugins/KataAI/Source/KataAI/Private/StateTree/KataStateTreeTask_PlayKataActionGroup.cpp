#include "StateTree/KataStateTreeTask_PlayKataActionGroup.h"

#include "StateTreeExecutionContext.h"
#include "StateTree/KataStateTreeExecutionUtils.h"
#include "FunctionLibraries/KataFL_ActionGroup.h"
#include "GameFramework/Pawn.h"
#include "Runtime/KataActionComponent.h"

FKataStateTreeTask_PlayKataActionGroup::FKataStateTreeTask_PlayKataActionGroup()
{
    // 진입 시 입력과 반환된 실행을 고정해 대상 변경이나 바인딩 복사가 정리 대상을 바꾸지 않게 한다.
    bShouldCopyBoundPropertiesOnTick = false;
    bShouldCopyBoundPropertiesOnExitState = false;
#if WITH_EDITORONLY_DATA
    bConsideredForCompletion = true;
    bCanEditConsideredForCompletion = false;
#endif
}

EStateTreeRunStatus FKataStateTreeTask_PlayKataActionGroup::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
    FInstanceDataType& Data = Context.GetInstanceData(*this);
    Data.Selection = FKataActionGroupSelection();
    UKataActionComponent* Component = nullptr;
    if (!KataStateTreeExecution::Prepare(Data, Component))
    {
        return EStateTreeRunStatus::Failed;
    }
    if (!IsValid(Data.Group))
    {
        Data.Result = EKataStateTreeExecutionResult::InvalidSetup;
        return EStateTreeRunStatus::Failed;
    }
    FKataContext ActionContext;
    ActionContext.OwnerActor = Data.ExecutionPawn;
    ActionContext.AvatarActor = Data.ExecutionPawn;
    ActionContext.TargetActor = Data.TargetActor.Get();
    TArray<int32> Candidates;
    for (int32 Index = 0; Index < Data.Group->Entries.Num(); ++Index)
    {
        const FKataActionGroupEntry& Entry = Data.Group->Entries[Index];
        if (!Entry.HasValidAsset() || !Entry.HasValidPayload() || !FMath::IsFinite(Entry.Weight) || Entry.Weight <= 0.0f)
        {
            continue;
        }
        if (Entry.Type == EKataActionGroupEntryType::Action
            && Component->CanPlayKataAction(Entry.Action, ActionContext) != EKataStartResult::Started)
        {
            continue;
        }
        // Graph에는 같은 사전 판정 API가 없으므로 실제 시작 결과를 확인한다.
        Candidates.Add(Index);
    }
    // 난수 생성기의 상한 반올림이 순수 선택 함수의 [0, 1) 계약을 벗어나지 않게 한다.
    const double RandomValue = FMath::Min(static_cast<double>(FMath::FRand()), 1.0 - 1.0 / 16777216.0);
    if (!UKataFL_ActionGroup::TrySelectEntry(Data.Group, Candidates, RandomValue, Data.Selection))
    {
        Data.Result = EKataStateTreeExecutionResult::NoEligibleEntry;
        return EStateTreeRunStatus::Failed;
    }
    const FKataActionGroupEntry& Selected = Data.Selection.Entry;
    return Selected.Type == EKataActionGroupEntryType::Action
        ? KataStateTreeExecution::PlayAction(Data, Selected.Action)
        : KataStateTreeExecution::PlayGraph(Data, Selected.Graph, Data.EntryTrigger);
}

EStateTreeRunStatus FKataStateTreeTask_PlayKataActionGroup::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
    return KataStateTreeExecution::Poll(Context.GetInstanceData(*this));
}

void FKataStateTreeTask_PlayKataActionGroup::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
    KataStateTreeExecution::Cleanup(Context.GetInstanceData(*this));
}

