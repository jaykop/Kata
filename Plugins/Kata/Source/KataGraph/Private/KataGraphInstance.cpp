#include "KataGraphInstance.h"

#include "Action/KataAction.h"
#include "KataActionNode.h"
#include "KataCondition.h"
#include "KataEdge.h"
#include "KataEntryNode.h"
#include "KataGraph.h"
#include "KataGraphNodeBase.h"
#include "KataNode.h"
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
        return StartNode(TargetNode, Edge);
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

    const FKataConditionContext ConditionContext = Context.ToConditionContext();

    // TMap 순회 순서는 안정적이지 않으므로 저장된 자식 순서와 자식별 엣지 배열을 사용한다.
    for (const TObjectPtr<UKataGraphNodeBase>& Child : SourceNode->ChildrenNodes)
    {
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
            if (Edge->Condition != nullptr && !Edge->Condition->IsSatisfied(ConditionContext))
            {
                continue;
            }

            // 이미 이긴 후보를 넘지 못하면 해석하지 않는다. 조건 평가에 부작용이 없으므로 건너뛰어도 된다.
            if (Edge->Priority < InOutBestPriority
                || (Edge->Priority == InOutBestPriority && CandidateOrder >= InOutBestOrder))
            {
                continue;
            }

            // 자식이 경유 노드일 수 있으므로 실행 가능한 노드까지 해석한다.
            TSet<const UKataGraphNodeBase*> Visited;
            UKataActionNode* TargetNode = ResolveExecutableTarget(Child, TriggerTag, Visited);
            if (TargetNode == nullptr)
            {
                continue;
            }

            InOutBestEdge = Edge;
            InOutBestTarget = TargetNode;
            InOutBestPriority = Edge->Priority;
            InOutBestOrder = CandidateOrder;
        }
    }
}

UKataActionNode* UKataGraphInstance::ResolveExecutableTarget(UKataGraphNodeBase* Node,
    const FGameplayTag& TriggerTag, TSet<const UKataGraphNodeBase*>& Visited) const
{
    UKataNode* KataNode = Cast<UKataNode>(Node);
    if (KataNode == nullptr || Visited.Contains(KataNode))
    {
        return nullptr;
    }
    Visited.Add(KataNode);

    const FKataConditionContext ConditionContext = Context.ToConditionContext();
    if (KataNode->EntryCondition != nullptr && !KataNode->EntryCondition->IsSatisfied(ConditionContext))
    {
        return nullptr;
    }

    if (KataNode->IsExecutableState())
    {
        UKataActionNode* ActionNode = Cast<UKataActionNode>(KataNode);
        return ActionNode != nullptr && ActionNode->Action != nullptr ? ActionNode : nullptr;
    }

    // 머무를 수 없는 노드이므로 여기서 멈추지 않고 나가는 엣지로 계속 내려간다.
    UKataActionNode* BestTarget = nullptr;
    int32 BestPriority = MIN_int32;
    int32 BestOrder = MAX_int32;
    int32 Order = 0;

    for (const TObjectPtr<UKataGraphNodeBase>& Child : KataNode->ChildrenNodes)
    {
        TArray<UKataGraphEdgeBase*> Edges;
        KataNode->GetEdgesTo(Child, Edges);
        for (UKataGraphEdgeBase* EdgeBase : Edges)
        {
            const int32 CandidateOrder = Order++;
            const UKataEdge* Edge = Cast<UKataEdge>(EdgeBase);
            if (Edge == nullptr)
            {
                continue;
            }

            // 트리거는 들어온 엣지에서 이미 받았다. 여기서는 비어 있거나 같은 트리거만 통과시킨다.
            // Required Window Tag와 Timing은 떠나는 액션이 없어 보지 않는다.
            if (Edge->TriggerTag.IsValid() && !Edge->MatchesTrigger(TriggerTag))
            {
                continue;
            }
            if (Edge->Condition != nullptr && !Edge->Condition->IsSatisfied(ConditionContext))
            {
                continue;
            }
            if (Edge->Priority < BestPriority
                || (Edge->Priority == BestPriority && CandidateOrder >= BestOrder))
            {
                continue;
            }

            // 갈래마다 방문 기록을 따로 들고 내려간다. 한 갈래에서 지난 노드가 다른 갈래를 막지 않게 한다.
            TSet<const UKataGraphNodeBase*> BranchVisited = Visited;
            if (UKataActionNode* Resolved = ResolveExecutableTarget(Child, TriggerTag, BranchVisited))
            {
                BestTarget = Resolved;
                BestPriority = Edge->Priority;
                BestOrder = CandidateOrder;
            }
        }
    }

    return BestTarget;
}

bool UKataGraphInstance::StartNode(UKataActionNode* TargetNode, const UKataEdge* ViaEdge)
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
            CurrentActionInstance->RequestEnd(EKataEndReason::Branched);
        }
    }

    // 진입 엣지는 그래프 시작 때 받은 대상을 그대로 쓴다. 이어지는 전이만 엣지 설정을 따른다.
    // 파괴된 대상은 약한 참조가 스스로 비우므로 따로 확인하지 않는다.
    const bool bFromEntry = State == EKataGraphInstanceState::WaitingForEntry;
    if (!bFromEntry && IsValid(ViaEdge) && !ViaEdge->bKeepTarget)
    {
        Context.TargetActor.Reset();
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

    // PreCommands가 바꾼 대상을 다음 전이가 이어받게 한다. 액션이 시작 중에 이미 끝났어도 Context는 남아 있다.
    Context.TargetActor = NewActionInstance->GetContextRef().TargetActor;

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
    return StartNode(TargetNode, Edge);
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
        const UKataEdge* ViaEdge = PendingEdge;
        PendingEdge = nullptr;
        PendingTargetNode = nullptr;
        StartNode(TargetNode, ViaEdge);
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
