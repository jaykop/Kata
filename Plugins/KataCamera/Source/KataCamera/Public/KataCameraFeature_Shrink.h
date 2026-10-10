#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "KataCameraFeature.h"
#include "KataCameraFeature_Shrink.generated.h"

class AActor;

/**
 * 피벗과 카메라 사이에 장애물이 있으면 카메라를 피벗 쪽으로 당기는 Constraint Feature.
 *
 * 먼저 뷰 타깃 위치에서 피벗까지 구 스윕을 해, 피벗이 장애물 안이나 너머에 있으면 스윕 시작점을 충돌 지점으로 당긴다.
 * 그 시작점에서 배치·구도 보정이 끝난 카메라 위치까지 다시 스윕하고, 막히면 같은 광선 위에서 충돌 지점까지 당긴다.
 * 레일을 따라 이동하지 않으므로 Spline 배치의 각도는 그대로다. 카메라 회전과 FOV는 바꾸지 않는다.
 * 당김 정도는 피벗-카메라 거리에 대한 비율로 유지한다. 막히지 않은 동안 비율이 1이므로 Pitch 변화로 거리가 바뀌어도 지연이 없다.
 * 당길 때는 즉시 또는 빠르게, 복귀할 때는 느리게 보간해 벽을 스칠 때 튀지 않게 한다.
 */
UCLASS(meta = (DisplayName = "Shrink"))
class KATACAMERA_API UKataCameraFeature_Shrink : public UKataCameraFeature
{
    GENERATED_BODY()

public:
    UKataCameraFeature_Shrink();

    virtual void Deinitialize() override;
    virtual void Evaluate(FKataCameraPipelineContext& Context) override;
    virtual FString GetDebugString() const override;

protected:
    /** 스윕에 쓰는 구의 반지름. 근평면이 벽에 걸리지 않을 만큼 둔다. */
    UPROPERTY(EditAnywhere, Category = "Shrink", meta = (ClampMin = "0.0", Units = "Centimeters"))
    float ProbeRadius = 12.0f;

    /** 장애물을 판정하는 충돌 채널. */
    UPROPERTY(EditAnywhere, Category = "Shrink")
    TEnumAsByte<ECollisionChannel> ProbeChannel = ECC_Camera;

    /** 피벗에서 카메라까지 남길 최소 거리. 피벗 바로 앞에서 시야가 캐릭터 안으로 들어가는 것을 막는다. */
    UPROPERTY(EditAnywhere, Category = "Shrink", meta = (ClampMin = "0.0", Units = "Centimeters"))
    float MinDistance = 10.0f;

    /** 당길 때의 보간 속도. 0이면 즉시 당긴다. 장애물을 뚫고 보이는 프레임을 막으려면 0이나 큰 값을 쓴다. */
    UPROPERTY(EditAnywhere, Category = "Shrink", meta = (ClampMin = "0.0"))
    float PullInInterpSpeed = 0.0f;

    /** 장애물이 사라졌을 때 원래 거리로 돌아가는 보간 속도. 0이면 즉시 돌아간다. */
    UPROPERTY(EditAnywhere, Category = "Shrink", meta = (ClampMin = "0.0"))
    float RecoverInterpSpeed = 4.0f;

private:
    /** 현재 적용 중인 거리 비율(0~1). 1이면 당기지 않은 상태다. */
    float CurrentRatio = 1.0f;

    /** 마지막 프레임에 장애물이 있었는지. 디버그 표시용이다. */
    bool bLastBlocked = false;

    /** 마지막 프레임에 뷰 타깃 위치와 피벗 사이가 막혀 스윕 시작점을 당겼는지. 디버그 표시용이다. */
    bool bLastPivotBlocked = false;

    /** 뷰 타깃이 바뀌면 이전 폰의 당김 상태를 이어받지 않고 즉시 새 값으로 맞춘다. */
    TWeakObjectPtr<AActor> LastViewTarget;
};
