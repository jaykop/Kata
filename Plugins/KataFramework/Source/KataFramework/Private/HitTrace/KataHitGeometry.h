#pragma once

#include "CoreMinimal.h"

class UPrimitiveComponent;

/**
 * SocketTrace 면 판정의 정밀 단계에 쓰는 기하 계산.
 *
 * 엔진에는 삼각형으로 하는 충돌 질의가 없어서, 후보 컴포넌트에서 Sphere·Capsule·Box 도형을 꺼내 삼각형과 직접 교차 계산한다.
 * 나중에 HurtBox를 Shape 컴포넌트로 만들면 도형 수집 규칙만 늘리고 교차 계산은 그대로 쓴다.
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
        /** 스켈레탈 메시 바디의 본. 부위 구분에 쓴다. */
        FName BoneName;
        /** Physics Asset 바디 번호. 바디가 아니면 INDEX_NONE이다. */
        int32 BodyIndex = INDEX_NONE;
    };

    /**
     * 컴포넌트의 판정 도형을 모은다.
     * Sphere·Capsule·Box 컴포넌트는 그 도형, 스켈레탈 메시는 Physics Asset 바디, 그 밖은 BodySetup의 집합 도형을 쓴다.
     * Convex 요소는 감싸는 상자로 근사하고, Tapered Capsule은 큰 쪽 반지름의 Capsule로 근사한다.
     * 회전된 요소의 비균등 스케일은 근사한다.
     */
    void CollectShapes(UPrimitiveComponent& Component, TArray<FShape>& OutShapes);

    /**
     * 삼각형과 도형이 Inflate 거리 안에서 닿는지 판정한다.
     * 넓이가 0에 가까운 삼각형은 가장 긴 변의 선분으로, 세 점이 겹치면 점으로 판정한다.
     * Box는 반 크기를 Inflate만큼 늘려 판정하므로 모서리 근처에서는 실제 둥근 부풀림보다 조금 넓게 맞는다.
     *
     * @param OutContact 닿았을 때 삼각형 위의 접점.
     */
    bool IntersectTriangle(const FVector& A, const FVector& B, const FVector& C, const FShape& Shape, float Inflate, FVector& OutContact);
}
