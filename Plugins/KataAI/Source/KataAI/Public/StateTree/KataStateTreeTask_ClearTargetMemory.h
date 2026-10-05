#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "KataStateTreeTask_ClearTargetMemory.generated.h"

class APawn;

USTRUCT()
struct KATAAI_API FKataStateTreeTask_ClearTargetMemoryInstanceData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Context")
    TObjectPtr<APawn> Pawn = nullptr;
};

/** 복귀 성공 뒤 실행한다. 재감지한 현재 대상이 있으면 기억을 지우지 않는다. */
USTRUCT(meta = (DisplayName = "Clear Kata AI Target Memory"))
struct KATAAI_API FKataStateTreeTask_ClearTargetMemory : public FStateTreeTaskCommonBase
{
    GENERATED_BODY()
    using FInstanceDataType = FKataStateTreeTask_ClearTargetMemoryInstanceData;

    virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
    virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};