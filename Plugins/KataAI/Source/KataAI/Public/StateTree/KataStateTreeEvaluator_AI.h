#pragma once

#include "CoreMinimal.h"
#include "StateTreeEvaluatorBase.h"
#include "KataStateTreeEvaluator_AI.generated.h"

class AAIController;

/** AIController를 입력받아 소유 Pawn의 타게팅 상태와 AI Data 설정을 StateTree 바인딩 출력으로 제공한다. */
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

/** 캐릭터별 AI 상태와 AI Data 설정을 읽어 StateTree에 제공한다. Perception이나 Targeting Preset을 직접 실행하지 않는다. */
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
