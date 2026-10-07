#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "KataRuntimeTypes.h"
#include "UObject/Object.h"
#include "KataGraphInstance.generated.h"

class UKataActionInstance;
class UKataActionNode;
class UKataActionComponent;
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

#if WITH_EDITOR
/** 그래프 디버거에 남기는 실행 기록의 종류. 에디터 빌드에서만 존재한다. */
enum class EKataGraphDebugEvent : uint8
{
    /** 액션 노드로 전이했다. 진입 엣지로 시작한 경우 From은 비어 있다. */
    Transition,
    /** 현재 액션이 끝나면 전이하도록 예약했다. */
    Reserved,
    /** 대상 액션이 시작을 거절했다. */
    Rejected,
    /** 그래프 실행이 끝났다. */
    Ended
};

/**
 * 그래프 디버거에 표시할 실행 기록 하나.
 *
 * 노드·엣지·액션은 약한 참조다. PIE 중 저장으로 버려진 실행 사본을 이 기록이 붙잡지 않는다.
 */
struct FKataGraphDebugRecord
{
    EKataGraphDebugEvent Event = EKataGraphDebugEvent::Transition;
    double WorldSeconds = 0.0;
    TWeakObjectPtr<UKataGraphNodeBase> FromNode;
    TWeakObjectPtr<UKataGraphNodeBase> ToNode;
    TWeakObjectPtr<const UKataEdge> Edge;
    TWeakObjectPtr<UObject> Action;
    /** 엣지에 지정한 트리거. 비어 있으면 자동 전이다. */
    FGameplayTag TriggerTag;
    EKataStartResult StartResult = EKataStartResult::Started;
    EKataEndReason EndReason = EKataEndReason::Completed;
};
#endif

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
    bool InitializeInstance(UKataGraph* InGraph, UKataActionComponent* InActionComponent, const FKataContext& InContext);

    /** 현재 상태에서 트리거와 일치하는 전이를 한 번 평가한다. */
    UFUNCTION(BlueprintCallable, Category = "Kata|Graph")
    bool SendTrigger(UPARAM(meta = (Categories = "Trigger")) FGameplayTag TriggerTag);

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

    /** Ended 상태에서만 의미가 있는 종료 사유다. 순간 완료도 조회할 수 있다. */
    UFUNCTION(BlueprintPure, Category = "Kata|Graph")
    EKataEndReason GetEndReason() const { return GraphEndReason; }

    /** 액션 시작 요청이 한 번이라도 있었는지 반환한다. */
    UFUNCTION(BlueprintPure, Category = "Kata|Graph")
    bool HasActionStartResult() const { return bHasActionStartResult; }

    /** HasActionStartResult가 true일 때만 유효한 마지막 액션 시작 결과다. */
    UFUNCTION(BlueprintPure, Category = "Kata|Graph")
    EKataStartResult GetLastActionStartResult() const { return LastActionStartResult; }

    /** 거절 후 진입 대기와 실제로 시작한 순간 콤보를 구분한다. */
    UFUNCTION(BlueprintPure, Category = "Kata|Graph")
    bool HasStartedAction() const { return bHasStartedAction; }

    /** 현재 액션이 정상 완료되면 실행할 예약 대상. 예약이 없으면 nullptr이다. */
    UFUNCTION(BlueprintPure, Category = "Kata|Graph")
    UKataActionNode* GetPendingTargetNode() const { return PendingTargetNode; }

    /** 예약 대상으로 이어지는 엣지. 예약이 없으면 nullptr이다. */
    UFUNCTION(BlueprintPure, Category = "Kata|Graph")
    UKataEdge* GetPendingEdge() const { return PendingEdge; }

    UPROPERTY(BlueprintAssignable, Category = "Kata|Graph")
    FKataGraphInstanceEndedSignature OnGraphEnded;

#if WITH_EDITOR
    /** 보관하는 최근 기록의 최대 개수. 넘치면 가장 오래된 기록부터 버린다. */
    static constexpr int32 MaxDebugRecords = 32;

    /** 오래된 순서의 최근 실행 기록. 에디터 그래프 디버거가 읽는다. */
    const TArray<FKataGraphDebugRecord>& GetDebugRecords() const { return DebugRecords; }

    /** 기록을 추가할 때마다 증가한다. 화면이 변경 여부만 확인하는 데 쓴다. */
    uint32 GetDebugRecordSerial() const { return DebugRecordSerial; }
#endif

private:
#if WITH_EDITOR
    void AddDebugRecord(FKataGraphDebugRecord&& Record);

    TArray<FKataGraphDebugRecord> DebugRecords;
    uint32 DebugRecordSerial = 0;
#endif

    EKataEndReason GraphEndReason = EKataEndReason::Completed;
    EKataStartResult LastActionStartResult = EKataStartResult::InvalidDefinition;
    bool bHasActionStartResult = false;
    bool bHasStartedAction = false;

    /** 현재 진입점 또는 액션 노드에서 가장 우선하는 전이를 찾는다. */
    UKataEdge* SelectTransition(const FGameplayTag& TriggerTag, bool bAutomatic, UKataActionNode*& OutTargetNode) const;

    /**
     * 한 노드의 엣지를 저장된 자식·엣지 순서로 평가한다.
     *
     * bSkipSelfTarget은 별칭에서 온 엣지에만 켠다. 별칭이 현재 노드를 포함하면 그 엣지가
     * 자기 자신을 겨눌 수 있는데, 이는 별칭이 넓어서 생기는 부작용이지 의도한 전이가 아니다.
     * 노드에 직접 그은 자기 엣지는 명시적 의도이므로 끄고 평가한다.
     */
    void ConsiderNodeTransitions(const UKataGraphNodeBase* SourceNode, const FGameplayTag& TriggerTag,
        bool bAutomatic, bool bIgnoreWindow, bool bSkipSelfTarget, int32& InOutOrder,
        UKataEdge*& InOutBestEdge, UKataActionNode*& InOutBestTarget, int32& InOutBestPriority,
        int32& InOutBestOrder) const;

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
    TObjectPtr<UKataActionComponent> ActionComponent;

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
