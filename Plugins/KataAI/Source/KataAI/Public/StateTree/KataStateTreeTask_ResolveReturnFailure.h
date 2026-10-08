#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "KataStateTreeTask_ResolveReturnFailure.generated.h"

class APawn;

/** 복귀 이동을 포기했을 때의 처리 방식이다. */
UENUM()
enum class EKataAIReturnFailurePolicy : uint8
{
    /** 현재 위치를 새 Home으로 기록하고 그 자리에 머문다. */
    Stay,
    /** 복귀 지점으로 순간이동한다. 순간이동이 실패하면 Stay로 처리한다. */
    Teleport
};

USTRUCT()
struct KATAAI_API FKataStateTreeTask_ResolveReturnFailureInstanceData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Context")
    TObjectPtr<APawn> Pawn = nullptr;

    UPROPERTY(EditAnywhere, Category = "Parameter")
    EKataAIReturnFailurePolicy Policy = EKataAIReturnFailurePolicy::Stay;

    /** false면 AI 타게팅 컴포넌트의 Home을 복귀 지점으로 쓴다. true면 ReturnLocation을 쓴다. */
    UPROPERTY(EditAnywhere, Category = "Parameter")
    bool bUseReturnLocation = false;

    /** Teleport의 목적지다. 스포너 지점처럼 Home과 다른 위치를 바인딩할 때 사용한다. */
    UPROPERTY(EditAnywhere, Category = "Parameter", meta = (EditCondition = "bUseReturnLocation"))
    FVector ReturnLocation = FVector::ZeroVector;

    /** 실제로 적용된 처리 방식이다. Teleport가 실패하면 Stay가 된다. */
    UPROPERTY(VisibleAnywhere, Category = "Output")
    EKataAIReturnFailurePolicy AppliedPolicy = EKataAIReturnFailurePolicy::Stay;
};

/**
 * 복귀 재시도 상한에 도달했을 때 실행한다. 이동 요청을 멈추고 정책에 따라 Home을 정리한 뒤
 * 대상 기억과 재시도 횟수를 지우고 즉시 성공한다. 어느 경로든 Pawn이 멈춘 채 남지 않게 한다.
 * Stay는 현재 위치, Teleport는 순간이동한 위치를 새 Home으로 기록해 다음 교전 뒤 같은 실패를 반복하지 않는다.
 * 이전 Evaluator 출력으로 수색을 다시 고르지 않도록 Returned 상태처럼 짧은 Delay와 함께 둔다.
 */
USTRUCT(meta = (DisplayName = "Resolve Kata AI Return Failure"))
struct KATAAI_API FKataStateTreeTask_ResolveReturnFailure : public FStateTreeTaskCommonBase
{
    GENERATED_BODY()
    using FInstanceDataType = FKataStateTreeTask_ResolveReturnFailureInstanceData;

    virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
    virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
