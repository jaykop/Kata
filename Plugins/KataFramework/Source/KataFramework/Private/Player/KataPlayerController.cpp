#include "Player/KataPlayerController.h"

#include "KataPlayerCameraManager.h"
#include "KataLockOnData.h"
#include "Death/KataDeathComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/DataAsset.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "KataFrameworkLog.h"
#include "Player/KataGameMode.h"
#include "Targeting/KataPlayerTargetingComponent.h"
#include "Targeting/KataTargetPointComponent.h"
#include "UI/KataHUDSubsystem.h"
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
    RefreshHUDObservedPawn(GetPawn());
}

void AKataPlayerController::SetPawn(APawn* InPawn)
{
    ClearTargetingBindings();
    ClearDeathBinding();
    Super::SetPawn(InPawn);
    RefreshTargetingBindings();
    RefreshDeathBinding();
    RefreshHUDObservedPawn(GetPawn());
}

void AKataPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ClearTargetingBindings();
    ClearDeathBinding();
    RefreshHUDObservedPawn(nullptr);
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

void AKataPlayerController::RefreshHUDObservedPawn(APawn* ObservedPawn)
{
    // 로컬 플레이어가 아직 붙지 않은 시점의 SetPawn은 건너뛴다. BeginPlay에서 현재 폰으로 다시 전달한다.
    ULocalPlayer* LocalPlayer = IsLocalController() ? GetLocalPlayer() : nullptr;
    if (UKataHUDSubsystem* HUDSubsystem = LocalPlayer != nullptr ? LocalPlayer->GetSubsystem<UKataHUDSubsystem>() : nullptr)
    {
        HUDSubsystem->SetObservedPawn(ObservedPawn);
    }
}

void AKataPlayerController::ClearDeathBinding()
{
    if (UKataDeathComponent* DeathComponent = PawnDeathComponent.Get())
    {
        DeathComponent->OnDeathNative.Remove(PawnDeathHandle);
    }
    PawnDeathHandle.Reset();
    PawnDeathComponent.Reset();
}

void AKataPlayerController::RefreshDeathBinding()
{
    ClearDeathBinding();
    APawn* ControlledPawn = GetPawn();
    UKataDeathComponent* DeathComponent = ControlledPawn != nullptr ? ControlledPawn->FindComponentByClass<UKataDeathComponent>() : nullptr;
    // 이미 죽은 폰에 다시 빙의한 경우는 재시작을 다시 요청하지 않는다. 시체 빙의는 재시작을 기다리는 동안에만 유지된다.
    if (DeathComponent != nullptr && !DeathComponent->IsDead())
    {
        PawnDeathComponent = DeathComponent;
        PawnDeathHandle = DeathComponent->OnDeathNative.AddUObject(this, &AKataPlayerController::HandlePawnDied);
    }
}

void AKataPlayerController::HandlePawnDied(UKataDeathComponent* DeathComponent)
{
    UWorld* World = GetWorld();
    AKataGameMode* GameMode = World != nullptr ? World->GetAuthGameMode<AKataGameMode>() : nullptr;
    if (GameMode == nullptr)
    {
        UE_LOG(LogKataFramework, Warning, TEXT("Player pawn '%s' died, but the game mode is not a Kata Game Mode. The player is not restarted."),
            *GetNameSafe(DeathComponent != nullptr ? DeathComponent->GetOwner() : nullptr));
        return;
    }
    GameMode->RequestPlayerRestart(this);
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
