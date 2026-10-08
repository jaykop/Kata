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

    /** AI Data의 재시도 상한이다. 0이면 무제한이다. 마스터와 슬롯 하위 트리가 같은 값을 읽도록 Evaluator로 제공한다. */
    UPROPERTY(VisibleAnywhere, Category = "Output")
    int32 MaxMoveRetries = 0;

    /** AI Data의 재시도 간격이다. AI Data가 없을 때도 프레임 단위 재시도를 막도록 1초를 유지한다. */
    UPROPERTY(VisibleAnywhere, Category = "Output")
    float MoveRetryInterval = 1.0f;

    /** AI Data의 추격 한계 거리다. 0이면 무제한이다. */
    UPROPERTY(VisibleAnywhere, Category = "Output")
    float LeashDistance = 0.0f;
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
