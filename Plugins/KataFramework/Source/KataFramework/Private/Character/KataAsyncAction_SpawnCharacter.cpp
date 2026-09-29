#include "Character/KataAsyncAction_SpawnCharacter.h"

#include "Character/KataCharacter.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "KataFrameworkLog.h"

UKataAsyncAction_SpawnCharacter* UKataAsyncAction_SpawnCharacter::SpawnKataCharacter(UObject* WorldContextObject, FDataTableRowHandle Row,
    FTransform SpawnTransform)
{
    UKataAsyncAction_SpawnCharacter* Action = NewObject<UKataAsyncAction_SpawnCharacter>();
    Action->World = GEngine != nullptr ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull) : nullptr;
    Action->Row = Row;
    Action->SpawnTransform = SpawnTransform;
    // 게임 인스턴스에 등록해 완료나 취소 전까지 GC되지 않게 한다.
    Action->RegisterWithGameInstance(WorldContextObject);
    return Action;
}

void UKataAsyncAction_SpawnCharacter::Activate()
{
    UKataCharacterSpawnSubsystem* Subsystem = World.IsValid() ? World->GetSubsystem<UKataCharacterSpawnSubsystem>() : nullptr;
    if (Subsystem == nullptr)
    {
        UE_LOG(LogKataFramework, Warning, TEXT("Spawn Kata Character failed: no valid world for row %s."), *Row.RowName.ToString());
        HandleSpawnCompleted(nullptr);
        return;
    }

    // 월드 정리 시 서브시스템은 완료 콜백을 부르지 않으므로 Async Action을 별도로 해제한다.
    WorldTearDownHandle = FWorldDelegates::OnWorldBeginTearDown.AddUObject(this, &UKataAsyncAction_SpawnCharacter::HandleWorldBeginTearDown);
    SpawnHandle = Subsystem->RequestSpawn(Row, SpawnTransform,
        FKataCharacterSpawnDelegate::CreateUObject(this, &UKataAsyncAction_SpawnCharacter::HandleSpawnCompleted));
}

void UKataAsyncAction_SpawnCharacter::Cancel()
{
    UnbindWorldTearDown();
    if (UKataCharacterSpawnSubsystem* Subsystem = World.IsValid() ? World->GetSubsystem<UKataCharacterSpawnSubsystem>() : nullptr)
    {
        Subsystem->CancelSpawn(SpawnHandle);
    }
    SpawnHandle = FKataCharacterSpawnHandle();

    Super::Cancel();
}

void UKataAsyncAction_SpawnCharacter::HandleSpawnCompleted(AKataCharacter* Character)
{
    UnbindWorldTearDown();
    SpawnHandle = FKataCharacterSpawnHandle();

    if (ShouldBroadcastDelegates())
    {
        if (Character != nullptr)
        {
            OnSpawned.Broadcast(Character);
        }
        else
        {
            OnFailed.Broadcast(nullptr);
        }
    }

    SetReadyToDestroy();
}

void UKataAsyncAction_SpawnCharacter::HandleWorldBeginTearDown(UWorld* TearingDownWorld)
{
    if (World.Get() == TearingDownWorld)
    {
        Cancel();
    }
}

void UKataAsyncAction_SpawnCharacter::UnbindWorldTearDown()
{
    if (WorldTearDownHandle.IsValid())
    {
        FWorldDelegates::OnWorldBeginTearDown.Remove(WorldTearDownHandle);
        WorldTearDownHandle.Reset();
    }
}
