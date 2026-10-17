#include "Spawning/KataSpawnerComponent_SpawnArea.h"

#include "KataFrameworkLog.h"

int32 UKataSpawnerComponent_SpawnArea::CalculateSpawnCount_Implementation() const
{
    // Details는 두 값을 맞춰 주지만 Blueprint 파생 기본값이나 ini에서 들어온 값은 거르지 못한다.
    if (MinSpawnCount < 1 || MaxSpawnCount < 1 || MinSpawnCount > MaxSpawnCount)
    {
        UE_LOG(LogKataFramework, Warning, TEXT("Spawn Area %s has an invalid count range (Min %d, Max %d). Both must be at least 1 and Min must not exceed Max."),
            *GetName(), MinSpawnCount, MaxSpawnCount);
        return -1;
    }
    return FMath::RandRange(MinSpawnCount, MaxSpawnCount);
}

bool UKataSpawnerComponent_SpawnArea::GetSpawnTransform_Implementation(const FTransform& SpawnerTransform,
    int32 SpawnIndex, FTransform& OutTransform) const
{
    if (!SpawnAreaTransform.IsValid() || !SpawnerTransform.IsValid())
    {
        return false;
    }

    // 후보는 발 위치이므로 영역 바닥면(로컬 Z 0)에서만 고른다. 영역 높이는 후보 위치의 Z값에 더하지 않는다.
    FVector LocalPosition;
    if (AreaShape == EKataSpawnAreaShape::Cylinder)
    {
        if (!FMath::IsFinite(CylinderRadius) || CylinderRadius < 0.f || !FMath::IsFinite(CylinderHalfHeight) || CylinderHalfHeight < 0.f)
        {
            return false;
        }
        // 반지름을 제곱근으로 뽑아야 원판 넓이 안에서 균일하다. 그대로 뽑으면 중심 쪽에 몰린다.
        const float Distance = CylinderRadius * FMath::Sqrt(FMath::FRand());
        const float Angle = FMath::FRand() * UE_TWO_PI;
        LocalPosition = FVector(Distance * FMath::Cos(Angle), Distance * FMath::Sin(Angle), 0.0);
    }
    else
    {
        if (BoxExtent.ContainsNaN() || BoxExtent.X < 0.0 || BoxExtent.Y < 0.0 || BoxExtent.Z < 0.0)
        {
            return false;
        }
        LocalPosition = FVector(FMath::FRandRange(-BoxExtent.X, BoxExtent.X), FMath::FRandRange(-BoxExtent.Y, BoxExtent.Y), 0.0);
    }

    const FTransform AreaTransform = SpawnAreaTransform * SpawnerTransform;
    if (!AreaTransform.IsValid())
    {
        return false;
    }
    OutTransform = FTransform(AreaTransform.GetRotation(), AreaTransform.TransformPosition(LocalPosition), FVector::OneVector);
    return OutTransform.IsValid();
}

float UKataSpawnerComponent_SpawnArea::GetAreaHalfHeight() const
{
    const double HalfHeight = AreaShape == EKataSpawnAreaShape::Cylinder ? static_cast<double>(CylinderHalfHeight) : BoxExtent.Z;
    return FMath::IsFinite(HalfHeight) ? static_cast<float>(FMath::Max(HalfHeight, 0.0)) : 0.f;
}

float UKataSpawnerComponent_SpawnArea::GetAreaHorizontalRadius() const
{
    const double Radius = AreaShape == EKataSpawnAreaShape::Cylinder ? static_cast<double>(CylinderRadius) : FMath::Max(BoxExtent.X, BoxExtent.Y);
    return FMath::IsFinite(Radius) ? static_cast<float>(FMath::Max(Radius, 0.0)) : 0.f;
}

#if WITH_EDITOR
void UKataSpawnerComponent_SpawnArea::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    // 방금 바꾼 값을 존중하고 반대쪽 값을 맞춘다. 그래야 최소를 올리거나 최대를 내리는 편집이 막히지 않는다.
    const FName PropertyName = PropertyChangedEvent.GetPropertyName();
    if (PropertyName == GET_MEMBER_NAME_CHECKED(UKataSpawnerComponent_SpawnArea, MinSpawnCount) && MinSpawnCount > MaxSpawnCount)
    {
        MaxSpawnCount = MinSpawnCount;
    }
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(UKataSpawnerComponent_SpawnArea, MaxSpawnCount) && MaxSpawnCount < MinSpawnCount)
    {
        MinSpawnCount = MaxSpawnCount;
    }

    Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif
