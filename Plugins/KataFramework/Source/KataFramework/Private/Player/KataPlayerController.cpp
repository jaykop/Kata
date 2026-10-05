#include "Player/KataPlayerController.h"

#include "KataPlayerCameraManager.h"
#include "KataLockOnData.h"
#include "Engine/DataAsset.h"
#include "GameFramework/Pawn.h"
#include "Targeting/KataPlayerTargetingComponent.h"
#include "Targeting/KataTargetPointComponent.h"
#include "UI/KataMainHUD.h"

AKataPlayerController::AKataPlayerController(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    PlayerCameraManagerClass = AKataPlayerCameraManager::StaticClass();
    MainHUDClass = UKataMainHUD::StaticClass();
}

void AKataPlayerController::BeginPlay()
{
    Super::BeginPlay();
    if (IsLocalController() && MainHUDClass != nullptr)
    {
        MainHUD = CreateWidget<UKataMainHUD>(this, MainHUDClass);
        if (MainHUD != nullptr)
        {
            MainHUD->SetVisibility(ESlateVisibility::HitTestInvisible);
            MainHUD->AddToPlayerScreen();
        }
    }
    RefreshTargetingBindings();
}

void AKataPlayerController::SetPawn(APawn* InPawn)
{
    ClearTargetingBindings();
    Super::SetPawn(InPawn);
    RefreshTargetingBindings();
}

void AKataPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ClearTargetingBindings();
    if (MainHUD != nullptr)
    {
        MainHUD->RemoveFromParent();
        MainHUD = nullptr;
    }
    Super::EndPlay(EndPlayReason);
}

void AKataPlayerController::ClearTargetingBindings()
{
    if (UKataPlayerTargetingComponent* Targeting = PlayerTargeting.Get())
    {
        Targeting->OnLockTargetChanged.RemoveDynamic(this, &AKataPlayerController::HandleLockTargetChanged);
    }
    PlayerTargeting.Reset();
    HandleLockTargetChanged(nullptr, nullptr);
}

void AKataPlayerController::RefreshTargetingBindings()
{
    ClearTargetingBindings();
    APawn* ControlledPawn = GetPawn();
    UKataPlayerTargetingComponent* Targeting = ControlledPawn != nullptr ? ControlledPawn->FindComponentByClass<UKataPlayerTargetingComponent>() : nullptr;
    if (IsLocalController() && Targeting != nullptr)
    {
        PlayerTargeting = Targeting;
        Targeting->OnLockTargetChanged.AddUniqueDynamic(this, &AKataPlayerController::HandleLockTargetChanged);
        HandleLockTargetChanged(nullptr, Targeting->GetLockPoint());
    }
}

void AKataPlayerController::HandleLockTargetChanged(UKataTargetPointComponent* OldPoint, UKataTargetPointComponent* NewPoint)
{
    if (AKataPlayerCameraManager* Camera = Cast<AKataPlayerCameraManager>(PlayerCameraManager))
    {
        Camera->SetLockOnFocus(NewPoint, NewPoint != nullptr ? Cast<UKataLockOnData>(NewPoint->GetLockOnCameraData()) : nullptr);
    }
    if (MainHUD != nullptr)
    {
        MainHUD->SetLockPoint(NewPoint);
    }
}
