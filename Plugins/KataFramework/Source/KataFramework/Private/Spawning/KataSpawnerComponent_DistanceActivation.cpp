#include "Spawning/KataSpawnerComponent_DistanceActivation.h"

bool UKataSpawnerComponent_DistanceActivation::HasValidDistances() const
{
    return FMath::IsFinite(SpawnDistance) && FMath::IsFinite(DespawnDistance) && SpawnDistance > 0.f
        && DespawnDistance > SpawnDistance;
}
