#include "StateTree/KataStateTreeTask_PlayKataActionGroup.h"

#include "Action/KataAction.h"
#include "StateTreeExecutionContext.h"
#include "StateTree/KataStateTreeExecutionUtils.h"
#include "FunctionLibraries/KataFL_ActionGroup.h"
#include "GameFramework/Pawn.h"
#include "KataAILog.h"
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
        UE_LOG(LogKataAI, Verbose, TEXT("Play KataActionGroup rejected before selection: Pawn=%s Result=%s"),
            *GetNameSafe(Data.Pawn), *UEnum::GetValueAsString(Data.Result));
        return EStateTreeRunStatus::Failed;
    }
    if (!IsValid(Data.Group))
    {
        Data.Result = EKataStateTreeExecutionResult::InvalidSetup;
        UE_LOG(LogKataAI, Verbose, TEXT("Play KataActionGroup has no Group: Pawn=%s"), *GetNameSafe(Data.Pawn));
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
        if (Entry.Type == EKataActionGroupEntryType::Action)
        {
            const EKataStartResult CanPlayResult = Component->CanPlayKataAction(Entry.Action, ActionContext);
            if (CanPlayResult != EKataStartResult::Started)
            {
                // 후보가 없을 때 어떤 실행 조건에 막혔는지 진단할 수 있게 항목별 사유를 남긴다.
                UE_LOG(LogKataAI, Verbose, TEXT("Play KataActionGroup entry %d '%s' not eligible: %s (Target=%s)"),
                    Index, *GetNameSafe(Entry.Action), *UEnum::GetValueAsString(CanPlayResult), *GetNameSafe(ActionContext.TargetActor.Get()));
                continue;
            }
        }
        // Graph에는 같은 사전 판정 API가 없으므로 실제 시작 결과를 확인한다.
        Candidates.Add(Index);
    }
    // 난수 생성기의 상한 반올림이 순수 선택 함수의 [0, 1) 계약을 벗어나지 않게 한다.
    const double RandomValue = FMath::Min(static_cast<double>(FMath::FRand()), 1.0 - 1.0 / 16777216.0);
    if (!UKataFL_ActionGroup::TrySelectEntry(Data.Group, Candidates, RandomValue, Data.Selection))
    {
        Data.Result = EKataStateTreeExecutionResult::NoEligibleEntry;
        UE_LOG(LogKataAI, Verbose, TEXT("Play KataActionGroup has no eligible entry: Pawn=%s Group=%s"),
            *GetNameSafe(Data.Pawn), *GetNameSafe(Data.Group));
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

#if WITH_EDITOR
FText FKataStateTreeTask_PlayKataActionGroup::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup,
    EStateTreeNodeFormatting Formatting) const
{
    const FInstanceDataType* Data = InstanceDataView.GetPtr<FInstanceDataType>();
    check(Data);
    // 바인딩된 입력은 원본 이름으로 보여 에디터에서 연결 여부를 확인할 수 있게 한다.
    const FText Value = KataStateTreeExecution::DescribeInput(ID, GET_MEMBER_NAME_CHECKED(FInstanceDataType, Group),
        KataStateTreeExecution::DescribeAsset(Data->Group), BindingLookup, Formatting);
    return FText::Format(NSLOCTEXT("KataAI", "PlayKataActionGroupDescription", "Play KataActionGroup {0}"), Value);
}
#endif
