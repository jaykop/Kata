#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "KataGraphEdgeBase.h"
#include "KataEdge.generated.h"

class UKataCondition;

/** 전이가 현재 액션을 즉시 중단할지, 정상 종료까지 기다릴지 정한다. */
UENUM(BlueprintType)
enum class EKataTransitionTiming : uint8
{
    /** 조건이 맞으면 현재 액션을 즉시 중단하고 다음 노드로 전이한다. */
    Immediate,
    /** 조건이 맞으면 현재 액션의 정상 종료를 기다린 뒤 다음 노드로 전이한다. */
    OnActionEnd
};

/**
 * 두 노드를 잇는 전이.
 *
 * 트리거 이벤트 태그로 발동하므로 입력·AI·Anim Notify가 같은 경로를 사용할 수 있다.
 * 현재 액션의 창 태스크가 트리거 수용 시점을 정하고, 이 엣지는 필요한 창만 지정한다.
 */
UCLASS(BlueprintType, meta = (DisplayName = "Kata Edge"))
class KATAGRAPH_API UKataEdge : public UKataGraphEdgeBase
{
    GENERATED_BODY()

public:
    UKataEdge();

    /**
     * 이 전이를 여는 트리거.
     *
     * 태그 계층을 포함해 비교한다. Input.Attack을 지정하면 Input.Attack.Light도 받는다.
     * 비우면 트리거 없이 조건만 보는 자동 전이가 된다. 액션이 끝나고 중립으로
     * 돌아오는 경로가 여기에 해당한다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Transition",
        meta = (DisplayName = "Trigger Event Tag", ToolTip = "이 전이를 요청하는 사건 태그입니다. 예: Input.Attack.Light. 비워 두면 액션 종료 시 조건만 평가하는 자동 전이가 됩니다."))
    FGameplayTag TriggerTag;

    /**
     * 현재 액션에서 이 태그의 창이 열려 있어야 전이가 성립한다.
     *
     * 창은 액션 타임라인의 Transition Window 태스크가 연다.
     * 비우면 액션 실행 중에는 항상 열린 것으로 본다.
     * 진입 노드에서 나가는 엣지는 기준 액션이 없어 이 값을 무시한다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Transition",
        meta = (DisplayName = "Required Action Window Tag", ToolTip = "현재 액션에서 열려 있어야 하는 Transition Window 태그입니다. 비워 두면 액션이 실행되는 동안 언제든 트리거를 받을 수 있습니다."))
    FGameplayTag RequiredWindowTag;

    /** 추가 조건. 비우면 통과한다. 평가에 부작용이 없어야 한다. */
    UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Kata|Transition")
    TObjectPtr<UKataCondition> Condition;

    /** 지금 끊고 갈지, 현재 액션이 끝난 뒤 이어갈지. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Transition")
    EKataTransitionTiming Timing = EKataTransitionTiming::Immediate;

    /**
     * 같은 프레임에 여러 엣지가 성립할 때의 우선순위. 값이 클수록 먼저 선택한다.
     * 같으면 선언 순서를 따른다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Transition")
    int32 Priority = 0;

    /**
     * 이 전이로 다음 액션에 들어갈 때 현재 대상을 넘길지 여부.
     *
     * 끄면 다음 액션은 대상 없이 시작하며, 대상은 그 액션의 PreCommands가 정할 수 있다.
     * 경유 노드를 거치는 전이는 현재 노드에서 나가는 첫 엣지의 값을 따른다.
     * 진입 노드에서 나가는 엣지는 그래프 시작 때 받은 대상을 그대로 쓰므로 이 값을 무시한다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Transition")
    bool bKeepTarget = true;

    /** 주어진 트리거가 이 엣지의 트리거 조건에 맞는지. 자동 전이는 트리거를 보지 않는다. */
    UFUNCTION(BlueprintPure, Category = "Kata|Transition")
    bool MatchesTrigger(const FGameplayTag& Trigger) const;

    /** 트리거 없이 조건만으로 성립하는 자동 전이면 true를 반환한다. */
    UFUNCTION(BlueprintPure, Category = "Kata|Transition")
    bool IsAutomatic() const { return !TriggerTag.IsValid(); }

#if WITH_EDITOR
    virtual FText GetNodeTitle() const override;
#endif
};
