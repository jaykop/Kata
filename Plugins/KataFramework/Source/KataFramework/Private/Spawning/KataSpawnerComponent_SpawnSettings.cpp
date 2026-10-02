#include "Spawning/KataSpawnerComponent_SpawnSettings.h"

int32 UKataSpawnerComponent_SpawnSettings::CalculateSpawnCount_Implementation() const
{
    return SpawnCount;
}

bool UKataSpawnerComponent_SpawnSettings::GetSpawnTransform_Implementation(const FTransform& SpawnerTransform,
    int32 SpawnIndex, FTransform& OutTransform) const
{
    if (BoxExtent.ContainsNaN() || BoxExtent.X < 0.0 || BoxExtent.Y < 0.0 || BoxExtent.Z < 0.0
        || !SpawnAreaTransform.IsValid() || !SpawnerTransform.IsValid())
    {
        return false;
    }

    const FTransform AreaTransform = SpawnAreaTransform * SpawnerTransform;
    if (!AreaTransform.IsValid())
    {
        return false;
    }
    const FVector LocalPosition(FMath::FRandRange(-BoxExtent.X, BoxExtent.X),
        FMath::FRandRange(-BoxExtent.Y, BoxExtent.Y), FMath::FRandRange(-BoxExtent.Z, BoxExtent.Z));
    OutTransform = FTransform(AreaTransform.GetRotation(), AreaTransform.TransformPosition(LocalPosition), FVector::OneVector);
    return OutTransform.IsValid();
}
