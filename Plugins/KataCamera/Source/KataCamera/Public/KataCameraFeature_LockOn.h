#pragma once

#include "CoreMinimal.h"
#include "KataCameraFeature.h"
#include "KataCameraFeature_LockOn.generated.h"

/**
 * 락온 중 모든 배치 레이어가 공유할 궤도 회전을 적용한다. 매니저가 기본 인스턴스를 소유한다.
 * 회전 계산과 블렌드 상태는 매니저가 가지며, 이 Feature는 Rotation 단계의 실행 순서를 지키기 위해 결과만 적용한다.
 */
UCLASS(EditInlineNew, DefaultToInstanced, meta = (DisplayName = "Lock On Rotation"))
class KATACAMERA_API UKataCameraFeature_LockOnRotation : public UKataCameraFeature
{
    GENERATED_BODY()

public:
    UKataCameraFeature_LockOnRotation();
    virtual void Evaluate(FKataCameraPipelineContext& Context) override;
};

/**
 * 배치 결과에서 좌우 정렬에 맞춰 카메라를 옆으로 옮기고, 조준점이 화면 위치에 오도록 회전을 보정한다.
 * 장애물 제약보다 먼저 실행한다. 보정 양은 락온 가중치를 따른다.
 * 해제 블렌드 중에는 마지막 보정량을 시점 기준 오프셋으로 줄여 가며 더한다. 고정된 초점을 다시 조준하면 시점 입력과 서로 당겨 화면이 출렁인다.
 */
UCLASS(EditInlineNew, DefaultToInstanced, meta = (DisplayName = "Lock On Framing"))
class KATACAMERA_API UKataCameraFeature_LockOnFraming : public UKataCameraFeature
{
    GENERATED_BODY()

public:
    UKataCameraFeature_LockOnFraming();
    virtual void Evaluate(FKataCameraPipelineContext& Context) override;
    virtual void Deinitialize() override;

private:
    /** 락온 중 옆 이동과 조준 보정을 적용한다. */
    void ApplyFraming(FKataCameraPipelineContext& Context) const;

    /** 마지막 락온 프레임의 보정량. 위치는 시점 Yaw 공간, 회전은 Pitch·Yaw 차이다. */
    FVector ReleaseLocationOffset = FVector::ZeroVector;
    FRotator ReleaseRotationOffset = FRotator::ZeroRotator;
    /** 보정량을 기록한 프레임의 가중치. 해제 중에는 현재 가중치와의 비율로 줄인다. */
    float ReleaseWeight = 0.0f;
    bool bHasReleaseOffset = false;
};
