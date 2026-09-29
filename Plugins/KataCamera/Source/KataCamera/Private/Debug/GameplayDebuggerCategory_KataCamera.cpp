#include "Debug/GameplayDebuggerCategory_KataCamera.h"

#if WITH_GAMEPLAY_DEBUGGER

#include "GameFramework/PlayerController.h"
#include "KataCameraFeature.h"
#include "KataPlayerCameraManager.h"

FGameplayDebuggerCategory_KataCamera::FGameplayDebuggerCategory_KataCamera()
{
    // 카메라는 플레이어 단위라 디버그 대상 액터를 고르지 않아도 표시한다.
    bShowOnlyWithDebugActor = false;
}

TSharedRef<FGameplayDebuggerCategory> FGameplayDebuggerCategory_KataCamera::MakeInstance()
{
    return MakeShareable(new FGameplayDebuggerCategory_KataCamera());
}

void FGameplayDebuggerCategory_KataCamera::CollectData(APlayerController* OwnerPC, AActor* DebugActor)
{
    const AKataPlayerCameraManager* CameraManager =
        OwnerPC != nullptr ? Cast<AKataPlayerCameraManager>(OwnerPC->PlayerCameraManager) : nullptr;
    if (CameraManager == nullptr)
    {
        AddTextLine(TEXT("{red}Player camera manager is not AKataPlayerCameraManager"));
        return;
    }

    const FKataCameraDebugSnapshot& Snapshot = CameraManager->GetDebugSnapshot();
    AddTextLine(FString::Printf(TEXT("{white}Camera Data: {yellow}%s"), *Snapshot.CameraDataName));

    if (!Snapshot.bPipelineActive)
    {
        AddTextLine(TEXT("{orange}Kata pipeline inactive (no pawn view target, camera data or placement). Using engine default camera."));
        return;
    }

    AddTextLine(FString::Printf(TEXT("{white}Placement: {yellow}%s"), *Snapshot.PlacementName));
    AddTextLine(FString::Printf(TEXT("{white}View Rotation: {yellow}P=%.1f Y=%.1f"), Snapshot.ViewRotation.Pitch, Snapshot.ViewRotation.Yaw));
    AddTextLine(FString::Printf(TEXT("{white}Distance: {yellow}%.1f  {white}FOV: {yellow}%.1f"),
        FVector::Dist(Snapshot.PivotLocation, Snapshot.CameraLocation), Snapshot.FieldOfView));

    const TArray<TObjectPtr<UKataCameraFeature>>& Features = CameraManager->GetOrderedFeatures();
    if (Features.IsEmpty())
    {
        AddTextLine(TEXT("{white}Features: {grey}none"));
    }
    for (const UKataCameraFeature* Feature : Features)
    {
        if (Feature == nullptr)
        {
            continue;
        }

        const FString StageName = StaticEnum<EKataCameraStage>()->GetNameStringByValue(static_cast<int64>(Feature->GetStage()));
        AddTextLine(FString::Printf(TEXT("{white}Feature [%s %d]: %s%s"),
            *StageName, Feature->GetPriority(), Feature->IsEnabled() ? TEXT("{green}") : TEXT("{grey}"), *Feature->GetClass()->GetName()));
    }

    AddShape(FGameplayDebuggerShape::MakePoint(Snapshot.PivotLocation, 8.0f, FColor::Yellow, TEXT("Pivot")));
}

#endif // WITH_GAMEPLAY_DEBUGGER
