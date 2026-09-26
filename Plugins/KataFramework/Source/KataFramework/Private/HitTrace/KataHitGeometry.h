#pragma once

#include "CoreMinimal.h"

class UKataHurtBoxComponent;

/**
 * SocketTrace 면 판정의 정밀 단계에 쓰는 기하 계산.
 *
 * 엔진에는 삼각형으로 하는 충돌 질의가 없어서, 후보 HurtBox의 Sphere·Capsule·Box 도형을 꺼내 삼각형과 직접 교차 계산한다.
 */
namespace KataHitGeometry
{
    enum class EShapeType : uint8
    {
        Sphere,
        Capsule,
        Box
    };

    /** 월드 공간의 판정 도형 하나. */
    struct FShape
    {
        EShapeType Type = EShapeType::Sphere;
        FVector Center = FVector::ZeroVector;
        FQuat Rotation = FQuat::Identity;
        /** Box의 반 크기. */
        FVector BoxExtent = FVector::ZeroVector;
        /** Sphere와 Capsule의 반지름. */
        float Radius = 0.0f;
        /** Capsule 가운데 선분의 반길이. 선분은 로컬 Z축을 따른다. */
        float HalfSegment = 0.0f;
    };

    /** HurtBox의 현재 트랜스폼과 스케일을 적용한 월드 도형을 만든다. 크기 규칙은 HurtBox의 물리 바디와 같다. */
    FShape MakeShape(const UKataHurtBoxComponent& HurtBox);

    /**
     * 삼각형과 도형이 Inflate 거리 안에서 닿는지 판정한다.
     * 넓이가 0에 가까운 삼각형은 가장 긴 변의 선분으로, 세 점이 겹치면 점으로 판정한다.
     * Box는 반 크기를 Inflate만큼 늘려 판정하므로 모서리 근처에서는 실제 둥근 부풀림보다 조금 넓게 맞는다.
     *
     * @param OutContact 닿았을 때 삼각형 위의 접점.
     */
    bool IntersectTriangle(const FVector& A, const FVector& B, const FVector& C, const FShape& Shape, float Inflate, FVector& OutContact);
}
