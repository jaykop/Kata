#include "HitTrace/KataHitGeometry.h"

#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "PhysicsEngine/AggregateGeom.h"
#include "PhysicsEngine/BodySetup.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/SkeletalBodySetup.h"

namespace KataHitGeometry
{
    namespace
    {
        /** 길이 제곱이 이보다 작으면 점으로 본다(cm²). */
        constexpr double DegenerateLengthSquared = 1.0e-4;

        void AddAggregateShapes(const FKAggregateGeom& Geometry, const FTransform& BaseTransform, FName BoneName, int32 BodyIndex, TArray<FShape>& OutShapes)
        {
            // 도형 크기는 float이므로 스케일도 float로 다룬다.
            const FVector3f Scale(BaseTransform.GetScale3D().GetAbs());
            const float MinScale = Scale.GetMin();
            const FQuat BaseRotation = BaseTransform.GetRotation();

            const auto MakeShape = [&](EShapeType Type, const FVector& LocalCenter, const FQuat& LocalRotation)
            {
                FShape& Shape = OutShapes.AddDefaulted_GetRef();
                Shape.Type = Type;
                Shape.Center = BaseTransform.TransformPosition(LocalCenter);
                Shape.Rotation = BaseRotation * LocalRotation;
                Shape.BoneName = BoneName;
                Shape.BodyIndex = BodyIndex;
                return &Shape;
            };

            for (const FKSphereElem& Element : Geometry.SphereElems)
            {
                MakeShape(EShapeType::Sphere, Element.Center, FQuat::Identity)->Radius = Element.Radius * MinScale;
            }
            for (const FKSphylElem& Element : Geometry.SphylElems)
            {
                FShape* Shape = MakeShape(EShapeType::Capsule, Element.Center, Element.Rotation.Quaternion());
                Shape->Radius = Element.Radius * FMath::Max(Scale.X, Scale.Y);
                Shape->HalfSegment = Element.Length * 0.5f * Scale.Z;
            }
            for (const FKTaperedCapsuleElem& Element : Geometry.TaperedCapsuleElems)
            {
                FShape* Shape = MakeShape(EShapeType::Capsule, Element.Center, Element.Rotation.Quaternion());
                Shape->Radius = FMath::Max(Element.Radius0, Element.Radius1) * FMath::Max(Scale.X, Scale.Y);
                Shape->HalfSegment = Element.Length * 0.5f * Scale.Z;
            }
            for (const FKBoxElem& Element : Geometry.BoxElems)
            {
                MakeShape(EShapeType::Box, Element.Center, Element.Rotation.Quaternion())->BoxExtent = FVector(Element.X, Element.Y, Element.Z) * 0.5 * FVector(Scale);
            }
            for (const FKConvexElem& Element : Geometry.ConvexElems)
            {
                // 볼록 도형을 정확히 계산하려면 GJK가 필요하다. 판정 면이 칼날 크기라 감싸는 상자로도 충분하다고 보고 근사한다.
                const FTransform ElementTransform = Element.GetTransform();
                const FBox& Bounds = Element.ElemBox;
                FShape* Shape = MakeShape(EShapeType::Box, ElementTransform.TransformPosition(Bounds.GetCenter()), ElementTransform.GetRotation());
                Shape->BoxExtent = Bounds.GetExtent() * FVector(Scale);
            }
        }

        /** 선분과 원점 중심 상자(반 크기 Extent)가 겹치는지 슬랩 방식으로 판정한다. */
        bool SegmentIntersectsBox(const FVector& Start, const FVector& End, const FVector& Extent)
        {
            const FVector Direction = End - Start;
            double EnterTime = 0.0;
            double ExitTime = 1.0;
            for (int32 Axis = 0; Axis < 3; ++Axis)
            {
                if (FMath::Abs(Direction[Axis]) < UE_SMALL_NUMBER)
                {
                    if (FMath::Abs(Start[Axis]) > Extent[Axis])
                    {
                        return false;
                    }
                    continue;
                }
                double Near = (-Extent[Axis] - Start[Axis]) / Direction[Axis];
                double Far = (Extent[Axis] - Start[Axis]) / Direction[Axis];
                if (Near > Far)
                {
                    Swap(Near, Far);
                }
                EnterTime = FMath::Max(EnterTime, Near);
                ExitTime = FMath::Min(ExitTime, Far);
                if (EnterTime > ExitTime)
                {
                    return false;
                }
            }
            return true;
        }

        /** 삼각형과 원점 중심 상자(반 크기 Extent)의 분리축 판정. 상자 축 3개, 면 법선 1개, 변과 상자 축의 외적 9개를 검사한다. */
        bool TriangleIntersectsBox(const FVector& V0, const FVector& V1, const FVector& V2, const FVector& Extent)
        {
            const auto IsSeparated = [&](const FVector& Axis)
            {
                if (Axis.SizeSquared() < UE_SMALL_NUMBER)
                {
                    return false;
                }
                const double P0 = FVector::DotProduct(V0, Axis);
                const double P1 = FVector::DotProduct(V1, Axis);
                const double P2 = FVector::DotProduct(V2, Axis);
                const double Radius = Extent.X * FMath::Abs(Axis.X) + Extent.Y * FMath::Abs(Axis.Y) + Extent.Z * FMath::Abs(Axis.Z);
                return FMath::Min3(P0, P1, P2) > Radius || FMath::Max3(P0, P1, P2) < -Radius;
            };

            const FVector BoxAxes[3] = { FVector::XAxisVector, FVector::YAxisVector, FVector::ZAxisVector };
            const FVector Edges[3] = { V1 - V0, V2 - V1, V0 - V2 };
            for (const FVector& Axis : BoxAxes)
            {
                if (IsSeparated(Axis))
                {
                    return false;
                }
            }
            if (IsSeparated(FVector::CrossProduct(Edges[0], Edges[1])))
            {
                return false;
            }
            for (const FVector& Edge : Edges)
            {
                for (const FVector& Axis : BoxAxes)
                {
                    if (IsSeparated(FVector::CrossProduct(Edge, Axis)))
                    {
                        return false;
                    }
                }
            }
            return true;
        }

        /** 삼각형과 선분 사이의 최단 거리. 교차하면 0이다. */
        double TriangleSegmentDistance(const FVector& A, const FVector& B, const FVector& C, const FVector& Start, const FVector& End, FVector& OutTrianglePoint)
        {
            FVector Intersection;
            FVector Normal;
            if (FMath::SegmentTriangleIntersection(Start, End, A, B, C, Intersection, Normal))
            {
                OutTrianglePoint = Intersection;
                return 0.0;
            }

            double BestDistanceSquared = TNumericLimits<double>::Max();
            for (const FVector& Point : { Start, End })
            {
                const FVector Closest = FMath::ClosestPointOnTriangleToPoint(Point, A, B, C);
                const double DistanceSquared = FVector::DistSquared(Closest, Point);
                if (DistanceSquared < BestDistanceSquared)
                {
                    BestDistanceSquared = DistanceSquared;
                    OutTrianglePoint = Closest;
                }
            }
            const FVector EdgeStarts[3] = { A, B, C };
            const FVector EdgeEnds[3] = { B, C, A };
            for (int32 Edge = 0; Edge < 3; ++Edge)
            {
                FVector EdgePoint;
                FVector SegmentPoint;
                FMath::SegmentDistToSegmentSafe(EdgeStarts[Edge], EdgeEnds[Edge], Start, End, EdgePoint, SegmentPoint);
                const double DistanceSquared = FVector::DistSquared(EdgePoint, SegmentPoint);
                if (DistanceSquared < BestDistanceSquared)
                {
                    BestDistanceSquared = DistanceSquared;
                    OutTrianglePoint = EdgePoint;
                }
            }
            return FMath::Sqrt(BestDistanceSquared);
        }

        void GetCapsuleSegment(const FShape& Shape, FVector& OutStart, FVector& OutEnd)
        {
            const FVector Axis = Shape.Rotation.GetUpVector() * Shape.HalfSegment;
            OutStart = Shape.Center - Axis;
            OutEnd = Shape.Center + Axis;
        }

        /** 넓이가 없는 삼각형을 대신하는 선분(점이면 두 끝이 같다)과 도형의 판정. */
        bool IntersectSegment(const FVector& Start, const FVector& End, const FShape& Shape, float Inflate, FVector& OutContact)
        {
            switch (Shape.Type)
            {
            case EShapeType::Sphere:
                OutContact = FMath::ClosestPointOnSegment(Shape.Center, Start, End);
                return FVector::Dist(OutContact, Shape.Center) <= Shape.Radius + Inflate;
            case EShapeType::Capsule:
            {
                FVector CapsuleStart;
                FVector CapsuleEnd;
                GetCapsuleSegment(Shape, CapsuleStart, CapsuleEnd);
                FVector CapsulePoint;
                FMath::SegmentDistToSegmentSafe(Start, End, CapsuleStart, CapsuleEnd, OutContact, CapsulePoint);
                return FVector::Dist(OutContact, CapsulePoint) <= Shape.Radius + Inflate;
            }
            case EShapeType::Box:
            default:
            {
                const FVector LocalStart = Shape.Rotation.UnrotateVector(Start - Shape.Center);
                const FVector LocalEnd = Shape.Rotation.UnrotateVector(End - Shape.Center);
                OutContact = FMath::ClosestPointOnSegment(Shape.Center, Start, End);
                return SegmentIntersectsBox(LocalStart, LocalEnd, Shape.BoxExtent + FVector(Inflate));
            }
            }
        }
    }

    void CollectShapes(UPrimitiveComponent& Component, TArray<FShape>& OutShapes)
    {
        // Shape 컴포넌트는 BodySetup보다 컴포넌트 값이 정확하다. 이후 HurtBox도 이 경로를 탄다.
        if (const USphereComponent* Sphere = Cast<USphereComponent>(&Component))
        {
            FShape& Shape = OutShapes.AddDefaulted_GetRef();
            Shape.Type = EShapeType::Sphere;
            Shape.Center = Sphere->GetComponentLocation();
            Shape.Radius = Sphere->GetScaledSphereRadius();
            return;
        }
        if (const UCapsuleComponent* Capsule = Cast<UCapsuleComponent>(&Component))
        {
            FShape& Shape = OutShapes.AddDefaulted_GetRef();
            Shape.Type = EShapeType::Capsule;
            Shape.Center = Capsule->GetComponentLocation();
            Shape.Rotation = Capsule->GetComponentQuat();
            Shape.Radius = Capsule->GetScaledCapsuleRadius();
            Shape.HalfSegment = FMath::Max(0.0f, Capsule->GetScaledCapsuleHalfHeight() - Shape.Radius);
            return;
        }
        if (const UBoxComponent* Box = Cast<UBoxComponent>(&Component))
        {
            FShape& Shape = OutShapes.AddDefaulted_GetRef();
            Shape.Type = EShapeType::Box;
            Shape.Center = Box->GetComponentLocation();
            Shape.Rotation = Box->GetComponentQuat();
            Shape.BoxExtent = Box->GetScaledBoxExtent();
            return;
        }

        const USkeletalMeshComponent* SkeletalMesh = Cast<USkeletalMeshComponent>(&Component);
        if (const UPhysicsAsset* PhysicsAsset = SkeletalMesh != nullptr ? SkeletalMesh->GetPhysicsAsset() : nullptr)
        {
            // 바디 인스턴스가 아니라 본 포즈에서 위치를 읽는다. 물리 상태가 없는 프리뷰에서도 같은 결과를 얻는다.
            for (int32 BodyIndex = 0; BodyIndex < PhysicsAsset->SkeletalBodySetups.Num(); ++BodyIndex)
            {
                const USkeletalBodySetup* BodySetup = PhysicsAsset->SkeletalBodySetups[BodyIndex];
                if (BodySetup == nullptr || SkeletalMesh->GetBoneIndex(BodySetup->BoneName) == INDEX_NONE)
                {
                    continue;
                }
                const FTransform BoneTransform = SkeletalMesh->GetSocketTransform(BodySetup->BoneName, RTS_World);
                AddAggregateShapes(BodySetup->AggGeom, BoneTransform, BodySetup->BoneName, BodyIndex, OutShapes);
            }
            return;
        }

        if (const UBodySetup* BodySetup = Component.GetBodySetup())
        {
            AddAggregateShapes(BodySetup->AggGeom, Component.GetComponentTransform(), NAME_None, INDEX_NONE, OutShapes);
        }
    }

    bool IntersectTriangle(const FVector& A, const FVector& B, const FVector& C, const FShape& Shape, float Inflate, FVector& OutContact)
    {
        const FVector AB = B - A;
        const FVector AC = C - A;
        const FVector BC = C - B;
        const double LongestSquared = FMath::Max3(AB.SizeSquared(), AC.SizeSquared(), BC.SizeSquared());
        if (LongestSquared < DegenerateLengthSquared)
        {
            return IntersectSegment(A, A, Shape, Inflate, OutContact);
        }
        // 칼날이 움직이지 않은 칸이나 시작 시점 판정은 넓이 없는 삼각형이 된다. 가장 긴 변을 선분으로 판정한다.
        if (FVector::CrossProduct(AB, AC).SizeSquared() < 1.0e-6 * LongestSquared * LongestSquared)
        {
            if (AB.SizeSquared() >= LongestSquared)
            {
                return IntersectSegment(A, B, Shape, Inflate, OutContact);
            }
            if (AC.SizeSquared() >= LongestSquared)
            {
                return IntersectSegment(A, C, Shape, Inflate, OutContact);
            }
            return IntersectSegment(B, C, Shape, Inflate, OutContact);
        }

        switch (Shape.Type)
        {
        case EShapeType::Sphere:
            OutContact = FMath::ClosestPointOnTriangleToPoint(Shape.Center, A, B, C);
            return FVector::Dist(OutContact, Shape.Center) <= Shape.Radius + Inflate;
        case EShapeType::Capsule:
        {
            FVector CapsuleStart;
            FVector CapsuleEnd;
            GetCapsuleSegment(Shape, CapsuleStart, CapsuleEnd);
            return TriangleSegmentDistance(A, B, C, CapsuleStart, CapsuleEnd, OutContact) <= Shape.Radius + Inflate;
        }
        case EShapeType::Box:
        default:
        {
            const FVector LocalA = Shape.Rotation.UnrotateVector(A - Shape.Center);
            const FVector LocalB = Shape.Rotation.UnrotateVector(B - Shape.Center);
            const FVector LocalC = Shape.Rotation.UnrotateVector(C - Shape.Center);
            OutContact = FMath::ClosestPointOnTriangleToPoint(Shape.Center, A, B, C);
            return TriangleIntersectsBox(LocalA, LocalB, LocalC, Shape.BoxExtent + FVector(Inflate));
        }
        }
    }
}
