#include "Player/KataPlayerController.h"

#include "KataPlayerCameraManager.h"

AKataPlayerController::AKataPlayerController(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    PlayerCameraManagerClass = AKataPlayerCameraManager::StaticClass();
}
