#include "Spawning/KataSpawnerComponent_Dummy.h"

void UKataSpawnerComponent_Dummy::OnCharacterSpawned_Implementation(AKataCharacterSpawner* Spawner, AKataCharacter* Character,
    const FKataCharacterId& CharacterId) const
{
    Super::OnCharacterSpawned_Implementation(Spawner, Character, CharacterId);
    UKataDummyComponent::AddDummyComponent(Character, Settings, Spawner);
}
