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

    FVector LocalPosition;
    if (AreaShape == EKataSpawnAreaShape::Sphere)
    {
        if (!FMath::IsFinite(SphereRadius) || SphereRadius < 0.f)
        {
            return false;
        }
        // 반지름을 세제곱근으로 뽑아야 구 부피 안에서 균일하다. 그대로 뽑으면 중심 쪽에 몰린다.
        LocalPosition = FMath::VRand() * (SphereRadius * FMath::Pow(FMath::FRand(), 1.0f / 3.0f));
    }
    else
    {
        if (BoxExtent.ContainsNaN() || BoxExtent.X < 0.0 || BoxExtent.Y < 0.0 || BoxExtent.Z < 0.0)
        {
            return false;
        }
        LocalPosition = FVector(FMath::FRandRange(-BoxExtent.X, BoxExtent.X),
            FMath::FRandRange(-BoxExtent.Y, BoxExtent.Y), FMath::FRandRange(-BoxExtent.Z, BoxExtent.Z));
    }

    const FTransform AreaTransform = SpawnAreaTransform * SpawnerTransform;
    if (!AreaTransform.IsValid())
    {
        return false;
    }
    OutTransform = FTransform(AreaTransform.GetRotation(), AreaTransform.TransformPosition(LocalPosition), FVector::OneVector);
    return OutTransform.IsValid();
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
