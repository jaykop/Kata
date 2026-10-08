#pragma once

#include "CoreMinimal.h"
#include "StateTreeConditionBase.h"
#include "KataStateTreeCondition_MoveRetryLimit.generated.h"

class APawn;

USTRUCT()
struct KATAAI_API FKataStateTreeCondition_MoveRetryLimitInstanceData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Context")
    TObjectPtr<APawn> Pawn = nullptr;

    /** 허용할 재시도 횟수다. 0이면 무제한이며 조건은 항상 false다. 몬스터별 StateTree 파라미터에 바인딩한다. */
    UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0"))
    int32 MaxMoveRetries = 0;
};

/**
 * 현재 재시도 횟수가 MaxMoveRetries에 도달했는지 확인한다. 횟수를 바꾸지 않는 순수 조회다.
 * 이동 실패 전이에 두어, 도달했으면 포기 상태로, 아니면 재시도 상태로 보낸다.
 * AI 타게팅 컴포넌트가 없으면 false를 반환해 무제한 재시도와 같은 경로를 따른다.
 */
USTRUCT(meta = (DisplayName = "Kata AI Move Retry Limit Reached"))
struct KATAAI_API FKataStateTreeCondition_MoveRetryLimit : public FStateTreeConditionCommonBase
{
    GENERATED_BODY()
    using FInstanceDataType = FKataStateTreeCondition_MoveRetryLimitInstanceData;

    virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
    virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
#if WITH_EDITOR
    virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup,
        EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif

    UPROPERTY(EditAnywhere, Category = "Parameter")
    bool bInvert = false;
};
