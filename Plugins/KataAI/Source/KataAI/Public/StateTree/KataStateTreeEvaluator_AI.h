#pragma once

#include "CoreMinimal.h"
#include "StateTreeEvaluatorBase.h"
#include "KataStateTreeEvaluator_AI.generated.h"

class AAIController;

/** Controller 컨텍스트를 입력받고 Pawn 타게팅의 관측 결과를 행동 트리에 전달한다. */
USTRUCT()
struct KATAAI_API FKataStateTreeEvaluator_AIInstanceData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Context")
    TObjectPtr<AAIController> AIController = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "Output")
    TObjectPtr<AActor> TargetActor = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "Output")
    bool bHasVisibleTarget = false;

    UPROPERTY(VisibleAnywhere, Category = "Output")
    bool bHasLastKnownLocation = false;

    UPROPERTY(VisibleAnywhere, Category = "Output")
    FVector LastKnownLocation = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, Category = "Output")
    FVector HomeLocation = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, Category = "Output")
    float TimeSinceLastSeen = 0.0f;
};

/** 인지·Preset을 실행하지 않고 캐릭터별 AI 상태만 읽는 공용 스키마 Evaluator다. */
USTRUCT(meta = (DisplayName = "Kata AI Context"))
struct KATAAI_API FKataStateTreeEvaluator_AI : public FStateTreeEvaluatorCommonBase
{
    GENERATED_BODY()
    using FInstanceDataType = FKataStateTreeEvaluator_AIInstanceData;

    virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
    virtual void TreeStart(FStateTreeExecutionContext& Context) const override;
    virtual void TreeStop(FStateTreeExecutionContext& Context) const override;
    virtual void Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};
