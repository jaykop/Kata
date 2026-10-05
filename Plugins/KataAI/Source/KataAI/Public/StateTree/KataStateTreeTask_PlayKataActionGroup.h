#pragma once

#include "CoreMinimal.h"
#include "ActionGroup/KataActionGroup.h"
#include "StateTree/KataStateTreeExecutionTypes.h"
#include "KataStateTreeTask_PlayKataActionGroup.generated.h"


/** 진입 설정과 실행 결과다. 실행 기록은 트리 인스턴스마다 독립적이다. */
USTRUCT()
struct KATAAI_API FKataStateTreeTask_PlayKataActionGroupInstanceData : public FKataStateTreeExecutionData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Parameter")
    TObjectPtr<UKataActionGroup> Group = nullptr;

    UPROPERTY(EditAnywhere, Category = "Parameter", meta = (Categories = "Trigger"))
    FGameplayTag EntryTrigger;

    UPROPERTY(VisibleAnywhere, Category = "Output")
    FKataActionGroupSelection Selection;
};

/** 실행 중 Running을 반환하고 이탈 시 자신이 시작한 인스턴스만 정리한다. */
USTRUCT(meta = (DisplayName = "Play KataActionGroup"))
struct KATAAI_API FKataStateTreeTask_PlayKataActionGroup : public FStateTreeTaskCommonBase
{
    GENERATED_BODY()
    using FInstanceDataType = FKataStateTreeTask_PlayKataActionGroupInstanceData;

    FKataStateTreeTask_PlayKataActionGroup();
    virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
    virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
    virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
    virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

