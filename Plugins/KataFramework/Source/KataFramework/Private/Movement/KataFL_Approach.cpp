#include "Movement/KataFL_Approach.h"

#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "HitTrace/KataHurtBoxComponent.h"
#include "Targeting/KataPlayerTargetingComponent.h"
#include "TargetingSystem/TargetingPreset.h"
#include "TargetingSystem/TargetingSubsystem.h"
#include "Types/TargetingSystemTypes.h"

namespace
{
    /** Box 최근접점을 번갈아 투영할 최대 횟수. 볼록 도형끼리라 몇 번이면 수렴한다. */
    constexpr int32 MaxBoxProjectionSteps = 8;

    /** 겹친 상태로 보는 거리(cm). */
    constexpr float OverlapTolerance = 0.01f;

    /** 점(구 중심)에서 선분까지. 반지름 0이면 점과 선분 사이 거리다. */
    float ClosestSpherePointToSegment(const FVector& Center, float Radius, const FVector& SegmentStart, const FVector& SegmentEnd,
        FVector& OutSurfacePoint, FVector& OutSegmentPoint)
    {
        OutSegmentPoint = FMath::ClosestPointOnSegment(Center, SegmentStart, SegmentEnd);
        const FVector Offset = OutSegmentPoint - Center;
        const float CenterDistance = Offset.Size();
        if (CenterDistance <= Radius + OverlapTolerance)
        {
            OutSurfacePoint = Center;
            return 0.0f;
        }
        OutSurfacePoint = Center + Offset * (Radius / CenterDistance);
        return CenterDistance - Radius;
    }

    /** 캡슐(축 선분 AxisStart~AxisEnd, 반지름)에서 선분까지. */
    float ClosestCapsulePointToSegment(const FVector& AxisStart, const FVector& AxisEnd, float Radius, const FVector& SegmentStart,
        const FVector& SegmentEnd, FVector& OutSurfacePoint, FVector& OutSegmentPoint)
    {
        FVector OnSegment;
        FVector OnAxis;
        FMath::SegmentDistToSegmentSafe(SegmentStart, SegmentEnd, AxisStart, AxisEnd, OnSegment, OnAxis);

        OutSegmentPoint = OnSegment;
        const FVector Offset = OnSegment - OnAxis;
        const float AxisDistance = Offset.Size();
        if (AxisDistance <= Radius + OverlapTolerance)
        {
            OutSurfacePoint = (AxisStart + AxisEnd) * 0.5f;
            return 0.0f;
        }
        OutSurfacePoint = OnAxis + Offset * (Radius / AxisDistance);
        return AxisDistance - Radius;
    }

    /** 회전한 상자(BoxTransform은 회전·위치만, Extent는 반 크기)에서 선분까지. */
    float ClosestBoxPointToSegment(const FTransform& BoxTransform, const FVector& Extent, const FVector& SegmentStart, const FVector& SegmentEnd,
        FVector& OutSurfacePoint, FVector& OutSegmentPoint)
    {
        const FVector LocalStart = BoxTransform.InverseTransformPositionNoScale(SegmentStart);
        const FVector LocalEnd = BoxTransform.InverseTransformPositionNoScale(SegmentEnd);
        const auto ClampToBox = [&Extent](const FVector& Point)
        {
            return FVector(FMath::Clamp(Point.X, -Extent.X, Extent.X), FMath::Clamp(Point.Y, -Extent.Y, Extent.Y), FMath::Clamp(Point.Z, -Extent.Z, Extent.Z));
        };

        // 선분과 상자는 둘 다 볼록하므로 서로의 최근접점으로 번갈아 투영하면 가장 가까운 두 점으로 수렴한다.
        FVector OnSegment = FMath::ClosestPointOnSegment(FVector::ZeroVector, LocalStart, LocalEnd);
        FVector OnBox = ClampToBox(OnSegment);
        for (int32 Step = 0; Step < MaxBoxProjectionSteps; ++Step)
        {
            const FVector NextOnSegment = FMath::ClosestPointOnSegment(OnBox, LocalStart, LocalEnd);
            const FVector NextOnBox = ClampToBox(NextOnSegment);
            const bool bConverged = FVector::DistSquared(NextOnSegment, OnSegment) < KINDA_SMALL_NUMBER;
            OnSegment = NextOnSegment;
            OnBox = NextOnBox;
            if (bConverged)
            {
                break;
            }
        }

        OutSegmentPoint = BoxTransform.TransformPositionNoScale(OnSegment);
        const float Distance = FVector::Dist(OnSegment, OnBox);
        if (Distance <= OverlapTolerance)
        {
            OutSurfacePoint = BoxTransform.GetLocation();
            return 0.0f;
        }
        OutSurfacePoint = BoxTransform.TransformPositionNoScale(OnBox);
        return Distance;
    }

    void AddActorHurtBoxes(const AActor& Actor, const AActor& Avatar, TArray<TWeakObjectPtr<const UKataHurtBoxComponent>>& OutHurtBoxes)
    {
        if (&Actor == &Avatar)
        {
            return;
        }
        TInlineComponentArray<UKataHurtBoxComponent*> HurtBoxes(&Actor);
        for (const UKataHurtBoxComponent* HurtBox : HurtBoxes)
        {
            OutHurtBoxes.AddUnique(HurtBox);
        }
    }

    /** Preset을 즉시 실행해 결과의 HurtBox(또는 결과 액터의 HurtBox 전부)를 모은다. */
    void GatherPresetHurtBoxes(AActor& Avatar, const UTargetingPreset& Preset, TArray<TWeakObjectPtr<const UKataHurtBoxComponent>>& OutHurtBoxes)
    {
        UTargetingSubsystem* Subsystem = UTargetingSubsystem::Get(Avatar.GetWorld());
        if (Subsystem == nullptr)
        {
            return;
        }

        FTargetingSourceContext SourceContext;
        SourceContext.SourceActor = &Avatar;
        SourceContext.InstigatorActor = &Avatar;
        SourceContext.SourceLocation = Avatar.GetActorLocation();

        // 즉시 실행 요청은 호출 안에서 끝나지만 핸들은 직접 해제해야 한다.
        FTargetingRequestHandle Handle = UTargetingSubsystem::MakeTargetRequestHandle(&Preset, SourceContext);
        Subsystem->ExecuteTargetingRequestWithHandle(Handle);
        if (const FTargetingDefaultResultsSet* Results = FTargetingDefaultResultsSet::Find(Handle))
        {
            for (const FTargetingDefaultResultData& Result : Results->TargetResults)
            {
                if (const UKataHurtBoxComponent* HurtBox = Cast<UKataHurtBoxComponent>(Result.HitResult.GetComponent()))
                {
                    if (HurtBox->GetOwner() != &Avatar)
                    {
                        OutHurtBoxes.AddUnique(HurtBox);
                    }
                }
                else if (const AActor* ResultActor = Result.HitResult.GetActor())
                {
                    AddActorHurtBoxes(*ResultActor, Avatar, OutHurtBoxes);
                }
            }
        }
        UTargetingSubsystem::ReleaseTargetRequestHandle(Handle);
    }
}

void KataFL::GetCapsuleAxis(const UCapsuleComponent& Capsule, FVector& OutStart, FVector& OutEnd, float& OutRadius)
{
    OutRadius = Capsule.GetScaledCapsuleRadius();
    const float InnerHalfHeight = FMath::Max(0.0f, Capsule.GetScaledCapsuleHalfHeight() - OutRadius);
    const FVector Center = Capsule.GetComponentLocation();
    const FVector Up = Capsule.GetUpVector();
    OutStart = Center - Up * InnerHalfHeight;
    OutEnd = Center + Up * InnerHalfHeight;
}

bool KataFL::GetActorCapsuleAxis(const AActor& Actor, FVector& OutStart, FVector& OutEnd, float& OutRadius)
{
    const UCapsuleComponent* Capsule = Cast<UCapsuleComponent>(Actor.GetRootComponent());
    if (Capsule == nullptr)
    {
        return false;
    }
    GetCapsuleAxis(*Capsule, OutStart, OutEnd, OutRadius);
    return true;
}

float KataFL::GetClosestHurtBoxPointToSegment(const UKataHurtBoxComponent& HurtBox, const FVector& SegmentStart, const FVector& SegmentEnd,
    FVector& OutSurfacePoint, FVector& OutSegmentPoint)
{
    const FVector Center = HurtBox.GetComponentLocation();
    switch (HurtBox.Shape)
    {
    case EKataHurtBoxShape::Sphere:
        return ClosestSpherePointToSegment(Center, HurtBox.GetScaledSphereRadius(), SegmentStart, SegmentEnd, OutSurfacePoint, OutSegmentPoint);

    case EKataHurtBoxShape::Capsule:
    {
        // 반높이는 반구를 포함하므로 축 선분은 반지름만큼 짧다. HurtBox 캡슐은 로컬 Z축을 따른다.
        const float Radius = HurtBox.GetScaledCapsuleRadius();
        const float InnerHalfHeight = FMath::Max(0.0f, HurtBox.GetScaledCapsuleHalfHeight() - Radius);
        const FVector Up = HurtBox.GetUpVector();
        return ClosestCapsulePointToSegment(Center - Up * InnerHalfHeight, Center + Up * InnerHalfHeight, Radius, SegmentStart, SegmentEnd,
            OutSurfacePoint, OutSegmentPoint);
    }

    case EKataHurtBoxShape::Box:
    default:
        return ClosestBoxPointToSegment(FTransform(HurtBox.GetComponentQuat(), Center), HurtBox.GetScaledBoxExtent(), SegmentStart, SegmentEnd,
            OutSurfacePoint, OutSegmentPoint);
    }
}

float KataFL::GetClosestBodyPointToSegment(const AActor& Actor, const FVector& SegmentStart, const FVector& SegmentEnd,
    FVector& OutSurfacePoint, FVector& OutSegmentPoint)
{
    FVector AxisStart;
    FVector AxisEnd;
    float Radius = 0.0f;
    if (GetActorCapsuleAxis(Actor, AxisStart, AxisEnd, Radius))
    {
        return ClosestCapsulePointToSegment(AxisStart, AxisEnd, Radius, SegmentStart, SegmentEnd, OutSurfacePoint, OutSegmentPoint);
    }
    return ClosestSpherePointToSegment(Actor.GetActorLocation(), 0.0f, SegmentStart, SegmentEnd, OutSurfacePoint, OutSegmentPoint);
}

void KataFL::GatherApproachHurtBoxes(AActor& Avatar, AActor* Target, const UTargetingPreset* SoftLockPreset,
    TArray<TWeakObjectPtr<const UKataHurtBoxComponent>>& OutHurtBoxes)
{
    OutHurtBoxes.Reset();

    // 락온 중인 PC와 AI는 대상이 정해져 있으므로 대상의 몸만 본다. 소프트락은 대상이 방향 기준일 뿐이라
    // 앞쪽의 다른 적에게 파고들 수 있으므로 Preset으로 주변 후보를 모은다.
    const UKataPlayerTargetingComponent* PlayerTargeting = Avatar.FindComponentByClass<UKataPlayerTargetingComponent>();
    const bool bSoftLock = PlayerTargeting != nullptr && !PlayerTargeting->IsLockOnActive();
    if (bSoftLock && SoftLockPreset != nullptr)
    {
        GatherPresetHurtBoxes(Avatar, *SoftLockPreset, OutHurtBoxes);
    }

    if (OutHurtBoxes.IsEmpty() && IsValid(Target))
    {
        AddActorHurtBoxes(*Target, Avatar, OutHurtBoxes);
    }
}

bool KataFL::FindClosestHurtBoxPoint(TConstArrayView<TWeakObjectPtr<const UKataHurtBoxComponent>> HurtBoxes, const FVector& SegmentStart,
    const FVector& SegmentEnd, FVector& OutSurfacePoint, FVector& OutSegmentPoint, float& OutDistance)
{
    // 중심점이 가까운 HurtBox를 먼저 고르면, 길거나 큰 도형(꼬리·몸통)은 중심이 멀어도 표면이 더 가까울 수 있어 틀린다.
    // 후보마다 도형 표면까지 계산한다. 도형 계산은 가벼워 후보 수십 개도 비용이 작다.
    bool bFound = false;
    for (const TWeakObjectPtr<const UKataHurtBoxComponent>& WeakHurtBox : HurtBoxes)
    {
        const UKataHurtBoxComponent* HurtBox = WeakHurtBox.Get();
        if (HurtBox == nullptr || !HurtBox->IsCollisionEnabled())
        {
            continue;
        }

        FVector SurfacePoint;
        FVector SegmentPoint;
        const float Distance = GetClosestHurtBoxPointToSegment(*HurtBox, SegmentStart, SegmentEnd, SurfacePoint, SegmentPoint);
        if (!bFound || Distance < OutDistance)
        {
            bFound = true;
            OutDistance = Distance;
            OutSurfacePoint = SurfacePoint;
            OutSegmentPoint = SegmentPoint;
        }
    }
    return bFound;
}
