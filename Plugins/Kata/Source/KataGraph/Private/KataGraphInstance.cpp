#include "KataGraphInstance.h"

#include "Action/KataAction.h"
#include "KataActionNode.h"
#include "KataCondition.h"
#include "KataEdge.h"
#include "KataEntryNode.h"
#include "KataGraph.h"
#include "KataGraphNodeBase.h"
#include "KataRuntimeLog.h"
#include "Runtime/KataActionInstance.h"
#include "Runtime/KataComponent.h"
#include "Engine/World.h"

namespace
{
    constexpr int32 MaxSynchronousTransitionDepth = 32;
}

UWorld* UKataGraphInstance::GetWorld() const
{
    return IsValid(KataComponent) ? KataComponent->GetWorld() : nullptr;
}

bool UKataGraphInstance::InitializeInstance(
    UKataGraph* InGraph, UKataComponent* InKataComponent, const FKataContext& InContext)
{
    if (State != EKataGraphInstanceState::Created || !IsValid(InGraph) || !IsValid(InKataComponent))
    {
        return false;
    }

    Graph = InGraph;
    KataComponent = InKataComponent;
    Context = InContext;
    State = EKataGraphInstanceState::WaitingForEntry;

    // 트리거가 비어 있는 진입 엣지는 그래프 시작과 동시에 한 번 평가한다.
    TryAutomaticTransition();
    return true;
}

bool UKataGraphInstance::SendTrigger(FGameplayTag TriggerTag)
{
    if (!IsRunning() || !TriggerTag.IsValid())
    {
        return false;
    }

    UKataActionNode* TargetNode = nullptr;
    UKataEdge* Edge = SelectTransition(TriggerTag, false, TargetNode);
    if (!IsValid(Edge) || !IsValid(TargetNode))
    {
        return false;
    }

    // 진입 엣지는 기준 액션이 없으므로 Timing을 적용하지 않는다.
    if (State == EKataGraphInstanceState::WaitingForEntry || Edge->Timing == EKataTransitionTiming::Immediate)
    {
        PendingEdge = nullptr;
        PendingTargetNode = nullptr;
        return StartNode(TargetNode);
    }

    // 같은 액션 중 새로 들어온 유효 트리거는 이전 예약을 교체한다.
    PendingEdge = Edge;
    PendingTargetNode = TargetNode;
    return true;
}

void UKataGraphInstance::RequestEnd(EKataEndReason Reason)
{
    EndGraph(Reason);
}

UKataEdge* UKataGraphInstance::SelectTransition(
    const FGameplayTag& TriggerTag, bool bAutomatic, UKataActionNode*& OutTargetNode) const
{
    OutTargetNode = nullptr;
    if (!IsValid(Graph))
    {
        return nullptr;
    }

    UKataEdge* BestEdge = nullptr;
    int32 BestPriority = MIN_int32;
    int32 BestOrder = MAX_int32;
    int32 Order = 0;

    if (State == EKataGraphInstanceState::WaitingForEntry)
    {
        // AllNodes 저장 순서로 진입점을 훑어 동률 전이의 결과를 결정한다.
        for (const TObjectPtr<UKataGraphNodeBase>& Node : Graph->AllNodes)
        {
            const UKataEntryNode* EntryNode = Cast<UKataEntryNode>(Node);
            if (EntryNode == nullptr)
            {
                continue;
            }
            if (EntryNode->EntryCondition != nullptr
                && !EntryNode->EntryCondition->IsSatisfied(Context.ToConditionContext()))
            {
                continue;
            }
            ConsiderNodeTransitions(EntryNode, TriggerTag, bAutomatic, true, Order,
                BestEdge, OutTargetNode, BestPriority, BestOrder);
        }
    }
    else if (IsValid(CurrentNode))
    {
        ConsiderNodeTransitions(CurrentNode, TriggerTag, bAutomatic, false, Order,
            BestEdge, OutTargetNode, BestPriority, BestOrder);
    }

    return BestEdge;
}

void UKataGraphInstance::ConsiderNodeTransitions(const UKataGraphNodeBase* SourceNode,
    const FGameplayTag& TriggerTag, bool bAutomatic, bool bIgnoreWindow, int32& InOutOrder,
    UKataEdge*& InOutBestEdge, UKataActionNode*& InOutBestTarget, int32& InOutBestPriority,
    int32& InOutBestOrder) const
{
    if (SourceNode == nullptr)
    {
        return;
    }

    // TMap 순회 순서는 안정적이지 않으므로 저장된 자식 순서와 자식별 엣지 배열을 사용한다.
    for (const TObjectPtr<UKataGraphNodeBase>& Child : SourceNode->ChildrenNodes)
    {
        UKataActionNode* TargetNode = Cast<UKataActionNode>(Child);
        if (TargetNode == nullptr)
        {
            continue;
        }

        TArray<UKataGraphEdgeBase*> Edges;
        SourceNode->GetEdgesTo(Child, Edges);
        for (UKataGraphEdgeBase* EdgeBase : Edges)
        {
            const int32 CandidateOrder = InOutOrder++;
            UKataEdge* Edge = Cast<UKataEdge>(EdgeBase);
            if (Edge == nullptr || Edge->IsAutomatic() != bAutomatic)
            {
                continue;
            }
            if (!bAutomatic && !Edge->MatchesTrigger(TriggerTag))
            {
                continue;
            }
            if (!bIgnoreWindow && Edge->RequiredWindowTag.IsValid())
            {
                UWorld* World = GetWorld();
                const float TriggerWorldSeconds = World != nullptr ? World->GetTimeSeconds() : 0.0f;
                if (!IsValid(CurrentActionInstance)
                    || !CurrentActionInstance->AcceptsTriggerAt(Edge->RequiredWindowTag, TriggerWorldSeconds))
                {
                    continue;
                }
            }
            if (!PassesTransitionConditions(Edge, TargetNode))
            {
                continue;
            }

            if (Edge->Priority > InOutBestPriority
                || (Edge->Priority == InOutBestPriority && CandidateOrder < InOutBestOrder))
            {
                InOutBestEdge = Edge;
                InOutBestTarget = TargetNode;
                InOutBestPriority = Edge->Priority;
                InOutBestOrder = CandidateOrder;
            }
        }
    }
}

bool UKataGraphInstance::PassesTransitionConditions(const UKataEdge* Edge, const UKataActionNode* TargetNode) const
{
    if (Edge == nullptr || TargetNode == nullptr || TargetNode->Action == nullptr)
    {
        return false;
    }

    const FKataConditionContext ConditionContext = Context.ToConditionContext();
    if (Edge->Condition != nullptr && !Edge->Condition->IsSatisfied(ConditionContext))
    {
        return false;
    }
    return TargetNode->EntryCondition == nullptr || TargetNode->EntryCondition->IsSatisfied(ConditionContext);
}

bool UKataGraphInstance::StartNode(UKataActionNode* TargetNode)
{
    if (!IsRunning() || !IsValid(TargetNode) || !IsValid(TargetNode->Action.Get()) || !IsValid(KataComponent))
    {
        return false;
    }

    ++SynchronousTransitionDepth;
    if (SynchronousTransitionDepth > MaxSynchronousTransitionDepth)
    {
        --SynchronousTransitionDepth;
        UE_LOG(LogKata, Error, TEXT("Kata graph exceeded the synchronous transition limit (%d)."),
            MaxSynchronousTransitionDepth);
        EndGraph(EKataEndReason::ContractError);
        return false;
    }

    bChangingAction = true;
    if (IsValid(CurrentActionInstance))
    {
        CurrentActionInstance->OnKataEnded.RemoveDynamic(this, &UKataGraphInstance::HandleActionEnded);
        if (CurrentActionInstance->IsRunning())
        {
            CurrentActionInstance->RequestEnd(EKataEndReason::Interrupted);
        }
    }

    CurrentNode = TargetNode;
    CurrentActionInstance = nullptr;
    PendingEdge = nullptr;
    PendingTargetNode = nullptr;

    UKataActionInstance* NewActionInstance = nullptr;
    const EKataStartResult StartResult = KataComponent->PlayKataAction(TargetNode->Action.Get(), Context, NewActionInstance);
    bChangingAction = false;

    if (StartResult != EKataStartResult::Started || !IsValid(NewActionInstance))
    {
        UE_LOG(LogKata, Warning, TEXT("Kata graph failed to start action '%s' (result %d)."),
            *GetNameSafe(TargetNode->Action.Get()), static_cast<int32>(StartResult));
        --SynchronousTransitionDepth;
        EndGraph(EKataEndReason::ContractError);
        return false;
    }

    CurrentActionInstance = NewActionInstance;
    State = EKataGraphInstanceState::RunningAction;
    if (NewActionInstance->IsRunning())
    {
        NewActionInstance->OnKataEnded.AddDynamic(this, &UKataGraphInstance::HandleActionEnded);
    }
    else
    {
        // 순간 타임라인은 PlayKataAction 안에서 끝날 수 있으므로 완료 처리를 직접 잇는다.
        CurrentActionInstance = nullptr;
        if (!TryAutomaticTransition())
        {
            EndGraph(EKataEndReason::Completed);
        }
    }

    --SynchronousTransitionDepth;
    return true;
}

bool UKataGraphInstance::TryAutomaticTransition()
{
    UKataActionNode* TargetNode = nullptr;
    UKataEdge* Edge = SelectTransition(FGameplayTag(), true, TargetNode);
    if (!IsValid(Edge) || !IsValid(TargetNode))
    {
        return false;
    }

    // 자동 전이는 진입 시점 또는 현재 액션의 정상 완료 시점에만 평가한다.
    return StartNode(TargetNode);
}

void UKataGraphInstance::HandleActionEnded(UKataActionInstance* Instance, EKataEndReason EndReason)
{
    if (bChangingAction || State == EKataGraphInstanceState::Ended || Instance != CurrentActionInstance)
    {
        return;
    }

    Instance->OnKataEnded.RemoveDynamic(this, &UKataGraphInstance::HandleActionEnded);
    CurrentActionInstance = nullptr;

    if (EndReason != EKataEndReason::Completed)
    {
        EndGraph(EndReason);
        return;
    }

    if (IsValid(PendingTargetNode))
    {
        UKataActionNode* TargetNode = PendingTargetNode;
        PendingEdge = nullptr;
        PendingTargetNode = nullptr;
        StartNode(TargetNode);
        return;
    }

    if (!TryAutomaticTransition())
    {
        EndGraph(EKataEndReason::Completed);
    }
}

void UKataGraphInstance::EndGraph(EKataEndReason Reason)
{
    if (State == EKataGraphInstanceState::Ended)
    {
        return;
    }

    State = EKataGraphInstanceState::Ended;
    PendingEdge = nullptr;
    PendingTargetNode = nullptr;

    UKataActionInstance* EndingAction = CurrentActionInstance;
    CurrentActionInstance = nullptr;
    if (IsValid(EndingAction))
    {
        EndingAction->OnKataEnded.RemoveDynamic(this, &UKataGraphInstance::HandleActionEnded);
        if (EndingAction->IsRunning())
        {
            EndingAction->RequestEnd(Reason);
        }
    }

    OnGraphEnded.Broadcast(this, Reason);
}
