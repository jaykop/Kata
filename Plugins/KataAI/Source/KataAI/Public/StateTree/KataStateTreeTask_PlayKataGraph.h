#pragma once

#include "CoreMinimal.h"
#include "StateTree/KataStateTreeExecutionTypes.h"
#include "KataStateTreeTask_PlayKataGraph.generated.h"

class UKataGraph;

/** 진입 설정과 실행 결과다. 실행 기록은 트리 인스턴스마다 독립적이다. */
USTRUCT()
struct KATAAI_API FKataStateTreeTask_PlayKataGraphInstanceData : public FKataStateTreeExecutionData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Parameter")
    TObjectPtr<UKataGraph> Graph = nullptr;

    /** 자동 진입이 없을 때 한 번만 보내는 트리거다. */
    UPROPERTY(EditAnywhere, Category = "Parameter", meta = (Categories = "Trigger"))
    FGameplayTag EntryTrigger;
};

/** 실행 중 Running을 반환하고 이탈 시 자신이 시작한 인스턴스만 정리한다. */
USTRUCT(meta = (DisplayName = "Play KataGraph"))
struct KATAAI_API FKataStateTreeTask_PlayKataGraph : public FStateTreeTaskCommonBase
{
    GENERATED_BODY()
    using FInstanceDataType = FKataStateTreeTask_PlayKataGraphInstanceData;

    FKataStateTreeTask_PlayKataGraph();
    virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
    virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
    virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
    virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
#if WITH_EDITOR
    virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup,
        EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};

