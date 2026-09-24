#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "KataRuntimeTypes.h"
#include "UObject/Object.h"
#include "KataGraphInstance.generated.h"

class UKataActionInstance;
class UKataActionNode;
class UKataComponent;
class UKataEdge;
class UKataGraph;
class UKataGraphInstance;
class UKataGraphNodeBase;

/** 그래프 실행 인스턴스의 수명 단계. */
UENUM(BlueprintType)
enum class EKataGraphInstanceState : uint8
{
    Created,
    WaitingForEntry,
    RunningAction,
    Ended
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FKataGraphInstanceEndedSignature, UKataGraphInstance*, Instance, EKataEndReason, EndReason);

/**
 * Kata 그래프 실행 한 번의 상태.
 *
 * 그래프 에셋과 노드에는 실행 상태를 저장하지 않는다. 현재 노드, 액션 종료를 기다리는 전이,
 * 실행 중인 액션 인스턴스는 이 객체가 소유한다. 트리거는 호출된 프레임에만 평가하며
 * 입력 버퍼는 아직 제공하지 않는다.
 */
UCLASS(BlueprintType)
class KATAGRAPH_API UKataGraphInstance : public UObject
{
    GENERATED_BODY()

public:
    virtual UWorld* GetWorld() const override;

    /** 그래프와 실행 컴포넌트를 연결하고 진입 대기 상태로 시작한다. */
    bool InitializeInstance(UKataGraph* InGraph, UKataComponent* InKataComponent, const FKataContext& InContext);

    /** 현재 상태에서 트리거와 일치하는 전이를 한 번 평가한다. */
    UFUNCTION(BlueprintCallable, Category = "Kata|Graph")
    bool SendTrigger(FGameplayTag TriggerTag);

    /** 그래프 실행을 끝내고 현재 액션도 같은 사유로 종료한다. */
    UFUNCTION(BlueprintCallable, Category = "Kata|Graph")
    void RequestEnd(EKataEndReason Reason);

    UFUNCTION(BlueprintPure, Category = "Kata|Graph")
    bool IsRunning() const { return State != EKataGraphInstanceState::Created && State != EKataGraphInstanceState::Ended; }

    UFUNCTION(BlueprintPure, Category = "Kata|Graph")
    EKataGraphInstanceState GetState() const { return State; }

    UFUNCTION(BlueprintPure, Category = "Kata|Graph")
    UKataGraph* GetGraph() const { return Graph; }

    UFUNCTION(BlueprintPure, Category = "Kata|Graph")
    UKataActionNode* GetCurrentNode() const { return CurrentNode; }

    UFUNCTION(BlueprintPure, Category = "Kata|Graph")
    UKataActionInstance* GetCurrentActionInstance() const { return CurrentActionInstance; }

    UPROPERTY(BlueprintAssignable, Category = "Kata|Graph")
    FKataGraphInstanceEndedSignature OnGraphEnded;

private:
    /** 현재 진입점 또는 액션 노드에서 가장 우선하는 전이를 찾는다. */
    UKataEdge* SelectTransition(const FGameplayTag& TriggerTag, bool bAutomatic, UKataActionNode*& OutTargetNode) const;

    /** 한 노드의 엣지를 저장된 자식·엣지 순서로 평가한다. */
    void ConsiderNodeTransitions(const UKataGraphNodeBase* SourceNode, const FGameplayTag& TriggerTag,
        bool bAutomatic, bool bIgnoreWindow, int32& InOutOrder, UKataEdge*& InOutBestEdge,
        UKataActionNode*& InOutBestTarget, int32& InOutBestPriority, int32& InOutBestOrder) const;

    /**
     * 전이가 가리키는 노드에서 출발해 실제로 실행할 액션 노드를 찾는다.
     *
     * 경유 노드처럼 머무를 수 없는 노드는 지나가고, 지나는 모든 노드의 조건과
     * 엣지의 조건을 확인한다. 한 곳이라도 막히면 전이가 성립하지 않으므로 nullptr을 준다.
     * 순환 그래프에서도 끝나도록 이미 지난 노드는 다시 내려가지 않는다.
     */
    UKataActionNode* ResolveExecutableTarget(UKataGraphNodeBase* Node, const FGameplayTag& TriggerTag,
        TSet<const UKataGraphNodeBase*>& Visited) const;
    /**
     * 대상 노드의 액션을 시작한다. ViaEdge는 이 노드로 들어온 전이이며, 현재 노드에서 나가는 첫 엣지다.
     * 그 엣지의 bKeepTarget에 따라 대상을 넘기고, 시작한 액션이 PreCommands에서 정한 대상을 그래프 Context에 다시 기록한다.
     */
    bool StartNode(UKataActionNode* TargetNode, const UKataEdge* ViaEdge);
    bool TryAutomaticTransition();
    void EndGraph(EKataEndReason Reason);

    UFUNCTION()
    void HandleActionEnded(UKataActionInstance* Instance, EKataEndReason EndReason);

    UPROPERTY(Transient)
    TObjectPtr<UKataGraph> Graph;

    UPROPERTY(Transient)
    TObjectPtr<UKataComponent> KataComponent;

    UPROPERTY(Transient)
    FKataContext Context;

    UPROPERTY(Transient)
    TObjectPtr<UKataActionNode> CurrentNode;

    UPROPERTY(Transient)
    TObjectPtr<UKataActionInstance> CurrentActionInstance;

    /** OnActionEnd 전이가 선택된 뒤 현재 액션이 정상 완료되기를 기다리는 대상. */
    UPROPERTY(Transient)
    TObjectPtr<UKataActionNode> PendingTargetNode;

    UPROPERTY(Transient)
    TObjectPtr<UKataEdge> PendingEdge;

    UPROPERTY(Transient)
    EKataGraphInstanceState State = EKataGraphInstanceState::Created;

    /** 즉시 전이로 액션을 끝내는 동안 종료 콜백이 그래프를 중복 종료하지 않게 한다. */
    bool bChangingAction = false;

    /** 순간 액션과 자동 전이로 같은 호출 스택에서 무한 순환하는 것을 막는다. */
    int32 SynchronousTransitionDepth = 0;
};
