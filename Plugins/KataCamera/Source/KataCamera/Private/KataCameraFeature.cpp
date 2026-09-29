#include "KataCameraFeature.h"

#include "KataPlayerCameraManager.h"

void UKataCameraFeature::Initialize(AKataPlayerCameraManager* InCameraManager)
{
    CameraManager = InCameraManager;
}

void UKataCameraFeature::Deinitialize()
{
    CameraManager.Reset();
}
