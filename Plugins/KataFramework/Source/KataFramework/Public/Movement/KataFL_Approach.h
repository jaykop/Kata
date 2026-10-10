#pragma once

#include "CoreMinimal.h"

class AActor;
class UCapsuleComponent;
class UKataHurtBoxComponent;
class UTargetingPreset;

/**
 * 대상에게 다가가는 이동(오토 대시, 전진 제한)이 거리를 재는 기준.
 * 거리는 실행 주체 캡슐의 축 선분에서 대상 도형 표면까지 잰다. 높이가 다른 부위도 수평 간격이 맞게 나온다.
 */
namespace KataFL
{
    /**
     * 캡슐의 축 선분(양 끝 반구 중심 사이)과 반지름을 구한다. 회전을 따르므로 기울어진 캡슐도 맞게 잰다.
     */
    KATAFRAMEWORK_API void GetCapsuleAxis(const UCapsuleComponent& Capsule, FVector& OutStart, FVector& OutEnd, float& OutRadius);

    /** 액터 루트가 캡슐이면 그 축과 반지름을 구한다. 루트가 캡슐이 아니면 false다. */
    KATAFRAMEWORK_API bool GetActorCapsuleAxis(const AActor& Actor, FVector& OutStart, FVector& OutEnd, float& OutRadius);

    /**
     * HurtBox 도형 표면에서 선분 [SegmentStart, SegmentEnd]에 가장 가까운 점을 구한다. 크기에는 컴포넌트 스케일이 적용된다.
     *
     * @param OutSurfacePoint 표면의 최근접점. 선분이 도형과 겹치면 표면 대신 도형 중심을 돌려줘 방향을 잴 수 있게 한다.
     * @param OutSegmentPoint 거리를 잰 선분 위의 점.
     * @return 선분과 도형 사이 거리(cm). 겹치면 0이다.
     */
    KATAFRAMEWORK_API float GetClosestHurtBoxPointToSegment(const UKataHurtBoxComponent& HurtBox, const FVector& SegmentStart, const FVector& SegmentEnd,
        FVector& OutSurfacePoint, FVector& OutSegmentPoint);

    /**
     * 액터 몸통 표면에서 선분에 가장 가까운 점을 구한다. 루트가 캡슐이면 캡슐 표면, 아니면 액터 위치를 점으로 본다.
     * HurtBox가 없는 대상의 대체 기준이다. 반환값과 출력은 GetClosestHurtBoxPointToSegment와 같다.
     */
    KATAFRAMEWORK_API float GetClosestBodyPointToSegment(const AActor& Actor, const FVector& SegmentStart, const FVector& SegmentEnd,
        FVector& OutSurfacePoint, FVector& OutSegmentPoint);

    /**
     * 다가갈 기준이 될 HurtBox 후보를 모은다.
     *
     * 실행 주체가 PC 타게팅 컴포넌트를 갖고 락온 중이 아니면(소프트락) SoftLockPreset을 즉시 실행해 결과에서 고른다.
     * 결과가 HurtBox이면 그대로, 액터이면 그 액터의 HurtBox 전부를 넣는다. 그 밖(락온 중 PC, AI)이거나 Preset이 후보를 내지 못하면
     * Target의 HurtBox 전부를 넣는다. 실행 주체 자신의 HurtBox는 넣지 않는다. 후보가 없으면 OutHurtBoxes가 빈다.
     */
    KATAFRAMEWORK_API void GatherApproachHurtBoxes(AActor& Avatar, AActor* Target, const UTargetingPreset* SoftLockPreset,
        TArray<TWeakObjectPtr<const UKataHurtBoxComponent>>& OutHurtBoxes);

    /**
     * 후보 HurtBox 중 표면이 선분에 가장 가까운 점을 찾는다. 파괴됐거나 콜리전이 꺼진 후보는 건너뛴다.
     * 출력의 의미는 GetClosestHurtBoxPointToSegment와 같다. 쓸 수 있는 후보가 없으면 false다.
     */
    KATAFRAMEWORK_API bool FindClosestHurtBoxPoint(TConstArrayView<TWeakObjectPtr<const UKataHurtBoxComponent>> HurtBoxes,
        const FVector& SegmentStart, const FVector& SegmentEnd, FVector& OutSurfacePoint, FVector& OutSegmentPoint, float& OutDistance);
}
