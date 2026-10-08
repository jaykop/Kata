#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "KataStateTreeTask_MoveRetry.generated.h"

class APawn;

/** 이동 재시도 횟수에 적용할 동작이다. */
UENUM()
enum class EKataAIMoveRetryOperation : uint8
{
    Increment,
    Reset
};

USTRUCT()
struct KATAAI_API FKataStateTreeTask_MoveRetryInstanceData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Context")
    TObjectPtr<APawn> Pawn = nullptr;

    UPROPERTY(EditAnywhere, Category = "Parameter")
    EKataAIMoveRetryOperation Operation = EKataAIMoveRetryOperation::Increment;

    /** 동작을 적용한 뒤의 재시도 횟수다. */
    UPROPERTY(VisibleAnywhere, Category = "Output")
    int32 RetryCount = 0;
};

/**
 * Pawn의 AI 타게팅 컴포넌트에 보관된 이동 재시도 횟수를 늘리거나 초기화한다.
 * 상한 판정은 하지 않는다. 상한은 Kata AI Move Retry Limit Reached 조건으로 확인한다.
 * 재시도 상태에는 이 Task와 함께 0보다 긴 Delay Task를 두어 프레임 단위 재시도를 막는다.
 */
USTRUCT(meta = (DisplayName = "Update Kata AI Move Retry"))
struct KATAAI_API FKataStateTreeTask_MoveRetry : public FStateTreeTaskCommonBase
{
    GENERATED_BODY()
    using FInstanceDataType = FKataStateTreeTask_MoveRetryInstanceData;

    virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
    virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
