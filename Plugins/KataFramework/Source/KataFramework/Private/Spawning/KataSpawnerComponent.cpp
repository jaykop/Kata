#include "Spawning/KataSpawnerComponent.h"

void UKataSpawnerComponent::OnCharacterSpawned_Implementation(AKataCharacterSpawner* Spawner, AKataCharacter* Character,
    const FKataCharacterId& CharacterId) const
{
}

bool UKataSpawnerComponent::AdjustSpawnTransform_Implementation(AKataCharacterSpawner* Spawner,
    const UKataSpawnerComponent_SpawnArea* SpawnArea, int32 SpawnIndex, const FTransform& CandidateTransform, FTransform& OutTransform) const
{
    OutTransform = CandidateTransform;
    return true;
}

int32 UKataSpawnerComponent::GetPlacementAttempts_Implementation() const
{
    return 1;
}