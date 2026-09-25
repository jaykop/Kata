#pragma once

#include "CoreMinimal.h"
#include "Tasks/TargetingTask.h"
#include "KataTargetingSortTask_Weighted.generated.h"

struct FTargetingDefaultResultData;

/**
 * 가중치를 가진 Targeting 정렬 기반 클래스.
 *
 * 엔진 UTargetingSortTask_Base는 원점수를 태스크 안의 최고점으로 나눠 0~1로 만든 뒤 더하므로, 원점수에 배율을 곱해도 상쇄된다.
 * 이 클래스는 정규화한 점수에 Weight를 곱해 더한다. 누적 점수가 작은 후보가 앞에 오도록 오름차순 정렬하는 엔진 규칙을 따르므로
 * 엔진 정렬 태스크와 같은 Preset에 섞어 쓸 수 있다. 정렬 결과의 첫 항목이 가장 우선하는 후보다.
 */
UCLASS(Abstract)
class KATATARGETING_API UKataTargetingSortTask_Weighted : public UTargetingTask
{
    GENERATED_BODY()

public:
    UKataTargetingSortTask_Weighted(const FObjectInitializer& ObjectInitializer);

    virtual void Execute(const FTargetingRequestHandle& TargetingHandle) const override;

protected:
    /** 후보의 원점수. 음수는 0으로 본다. 파생 클래스가 구현한다. */
    virtual float GetRawScore(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const;

    /** 다른 정렬 태스크와 합칠 때의 비중. 0이면 이 태스크는 순서에 영향을 주지 않는다. */
    UPROPERTY(EditAnywhere, Category = "Kata|Sort", meta = (ClampMin = "0.0"))
    float Weight = 1.0f;

    /** 켜면 원점수가 높을수록 앞선다. 끄면 낮을수록 앞선다. */
    UPROPERTY(EditAnywhere, Category = "Kata|Sort")
    bool bHigherIsBetter = false;

    /** 누적 점수가 같은 후보의 기존 순서를 유지한다. */
    UPROPERTY(EditAnywhere, Category = "Kata|Sort")
    bool bStableSort = true;
};
