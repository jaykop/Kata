#include "Player/KataGameMode.h"

#include "Character/KataCharacter.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "KataFrameworkLog.h"
#include "Player/KataPlayerController.h"

AKataGameMode::AKataGameMode(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    PlayerControllerClass = AKataPlayerController::StaticClass();
}

void AKataGameMode::RestartPlayerAtPlayerStart(AController* NewPlayer, AActor* StartSpot)
{
    // 행이 없거나, 이미 폰이 있거나, 시작 지점이 없거나, 관전 전용이면 엔진 흐름이 처리하게 둔다.
    if (PlayerCharacterRow.IsNull() || NewPlayer == nullptr || NewPlayer->IsPendingKillPending()
        || NewPlayer->GetPawn() != nullptr || StartSpot == nullptr || MustSpectate(Cast<APlayerController>(NewPlayer)))
    {
        Super::RestartPlayerAtPlayerStart(NewPlayer, StartSpot);
        return;
    }

    // 로드 중에 다시 시작 요청이 와도 요청을 중복으로 만들지 않는다.
    if (PendingPlayerSpawns.Contains(NewPlayer))
    {
        return;
    }

    UKataCharacterSpawnSubsystem* Subsystem = GetWorld() != nullptr ? GetWorld()->GetSubsystem<UKataCharacterSpawnSubsystem>() : nullptr;
    if (Subsystem == nullptr)
    {
        return;
    }

    // 엔진의 기본 폰 생성과 같이 시작 지점의 Yaw만 쓴다.
    const FRotator SpawnRotation = StartSpot->GetActorRotation();
    const FRotator SpawnYaw(0.0, SpawnRotation.Yaw, 0.0);
    const FTransform SpawnTransform(SpawnYaw, StartSpot->GetActorLocation());

    const FKataCharacterSpawnHandle Handle = Subsystem->RequestSpawn(PlayerCharacterRow, SpawnTransform,
        FKataCharacterSpawnDelegate::CreateUObject(this, &AKataGameMode::HandlePlayerCharacterSpawned,
            TWeakObjectPtr<AController>(NewPlayer), TWeakObjectPtr<AActor>(StartSpot), SpawnRotation));

    // 요청 단계에서 실패하면 콜백이 이미 불렸고 핸들은 무효다.
    if (Handle.IsValid())
    {
        PendingPlayerSpawns.Add(NewPlayer, Handle);
    }
}

void AKataGameMode::Logout(AController* Exiting)
{
    FKataCharacterSpawnHandle Handle;
    if (PendingPlayerSpawns.RemoveAndCopyValue(Exiting, Handle))
    {
        if (UKataCharacterSpawnSubsystem* Subsystem = GetWorld() != nullptr ? GetWorld()->GetSubsystem<UKataCharacterSpawnSubsystem>() : nullptr)
        {
            Subsystem->CancelSpawn(Handle);
        }
    }

    Super::Logout(Exiting);
}

bool AKataGameMode::IsPlayerCharacterPending(AController* Player) const
{
    return Player != nullptr && PendingPlayerSpawns.Contains(Player);
}

void AKataGameMode::HandlePlayerCharacterSpawned(AKataCharacter* Character, TWeakObjectPtr<AController> Player, TWeakObjectPtr<AActor> StartSpot,
    FRotator StartRotation)
{
    PendingPlayerSpawns.Remove(Player);

    if (Character == nullptr)
    {
        UE_LOG(LogKataFramework, Warning, TEXT("Player character spawn failed for row %s. The player has no pawn."),
            *PlayerCharacterRow.RowName.ToString());
        return;
    }

    AController* Controller = Player.Get();
    if (Controller == nullptr || Controller->IsPendingKillPending() || Controller->GetPawn() != nullptr)
    {
        // 로드 중에 플레이어가 나갔거나 다른 폰에 빙의했다. 주인 없는 캐릭터를 남기지 않는다.
        UE_LOG(LogKataFramework, Warning, TEXT("Player character for row %s is no longer needed and is destroyed."),
            *PlayerCharacterRow.RowName.ToString());
        Character->Destroy();
        return;
    }

    // RestartPlayerAtPlayerStart가 기본 폰을 만든 뒤 하는 처리를 그대로 이어서 한다.
    Controller->SetPawn(Character);
    if (AActor* Spot = StartSpot.Get())
    {
        InitStartSpot(Spot, Controller);
    }
    FinishRestartPlayer(Controller, StartRotation);
}
