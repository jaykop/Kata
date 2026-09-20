#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "KataGraphEdgeBase.h"
#include "KataEdge.generated.h"

class UKataCondition;

/** 전이가 현재 액션을 끊을지, 끝나기를 기다릴지. */
UENUM(BlueprintType)
enum class EKataTransitionTiming : uint8
{
    /** 조건이 맞는 즉시 현재 액션을 끊고 넘어간다. 캔슬. */
    Immediate,
    /** 조건이 맞아도 현재 액션이 끝날 때까지 기다린 뒤 넘어간다. 링크. */
    OnActionEnd
};

/**
 * 두 노드를 잇는 전이.
 *
 * 트리거 이벤트 태그로 발동하므로 입력·AI·애님 노티파이가 같은 통로를 쓴다.
 * 언제 받을 수 있는지는 현재 액션의 창 태스크가 정하고, 이 엣지는 어떤 창을 요구할지만 적는다.
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
     * 계층으로 맞춘다. Input.Attack을 걸어두면 Input.Attack.Light도 받는다.
     * 비우면 트리거 없이 조건만 보는 자동 전이가 된다. 액션이 끝나고 중립으로
     * 돌아오는 경로가 여기에 해당한다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Transition")
    FGameplayTag TriggerTag;

    /**
     * 현재 액션에서 이 태그의 창이 열려 있어야 전이가 성립한다.
     *
     * 창은 액션 타임라인의 Transition Window 태스크가 연다.
     * 비우면 액션이 도는 동안 항상 열린 것으로 본다.
     * 진입 노드에서 나가는 엣지는 기준 액션이 없어 이 값을 무시한다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Transition")
    FGameplayTag RequiredWindowTag;

    /** 추가 조건. 비우면 통과한다. 평가에 부작용이 없어야 한다. */
    UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Kata|Transition")
    TObjectPtr<UKataCondition> Condition;

    /** 지금 끊고 갈지, 현재 액션이 끝난 뒤 이어갈지. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Transition")
    EKataTransitionTiming Timing = EKataTransitionTiming::Immediate;

    /**
     * 같은 프레임에 여러 엣지가 맞을 때의 우선순위. 큰 값이 이긴다.
     * 같으면 선언 순서를 따른다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Transition")
    int32 Priority = 0;

    /** 주어진 트리거가 이 엣지의 트리거 조건에 맞는지. 자동 전이는 트리거를 보지 않는다. */
    UFUNCTION(BlueprintPure, Category = "Kata|Transition")
    bool MatchesTrigger(const FGameplayTag& Trigger) const;

    /** 트리거 없이 조건만으로 성립하는 자동 전이인지. */
    UFUNCTION(BlueprintPure, Category = "Kata|Transition")
    bool IsAutomatic() const { return !TriggerTag.IsValid(); }

#if WITH_EDITOR
    virtual FText GetNodeTitle() const override;
#endif
};
