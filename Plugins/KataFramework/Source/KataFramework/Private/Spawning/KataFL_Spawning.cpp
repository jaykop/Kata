#include "Spawning/KataFL_Spawning.h"

#include "AIController.h"
#include "Character/KataCharacter.h"
#include "Character/KataCharacterSpawnOwnership.h"
#include "UObject/StrongObjectPtr.h"

bool KataFL::DestroySpawnedCharacter(FKataCharacterSpawnOwnership& Ownership)
{
    Ownership.bDespawnRequested = true;
    AKataCharacter* Character = Ownership.Character.Get();
    if (Character == nullptr || Character->IsActorBeingDestroyed())
    {
        return true;
    }
    const TStrongObjectPtr<AKataCharacter> KeepAlive(Character);
    return Character->Destroy();
}

bool KataFL::DestroySpawnOwnedController(const FKataCharacterSpawnOwnership& Ownership, int32 ControllerIndex)
{
    AController* Controller = Ownership.OwnedControllers[ControllerIndex].Get();
    if (Controller == nullptr || Controller->IsActorBeingDestroyed() || !Controller->IsA<AAIController>())
    {
        return true;
    }
    const TStrongObjectPtr<AController> KeepAlive(Controller);
    if (Controller->GetPawn() != nullptr && Controller->GetPawn() != Ownership.Character.Get())
    {
        return true;
    }
    if (Controller->GetPawn() != nullptr)
    {
        Controller->UnPossess();
    }
    // UnPossess 안의 확장 코드가 다른 Pawn에 재사용했으면 제거하지 않는다.
    if (!IsValid(Controller) || Controller->IsActorBeingDestroyed() || Controller->GetPawn() != nullptr)
    {
        return true;
    }
    return Controller->Destroy();
}
