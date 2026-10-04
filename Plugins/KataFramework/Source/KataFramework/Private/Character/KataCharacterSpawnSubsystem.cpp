#include "Character/KataCharacterSpawnSubsystem.h"

#include "Character/KataCharacter.h"
#include "Character/KataCharacterRow.h"
#include "Character/KataPlayerCharacter.h"
#include "Engine/AssetManager.h"
#include "Engine/DataTable.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"
#include "KataFrameworkLog.h"

void UKataCharacterSpawnSubsystem::Deinitialize()
{
    // 월드가 정리되면 남은 요청은 생성할 곳이 없으므로 로드를 멈추고 콜백 없이 버린다.
    for (TPair<uint32, FPendingRequest>& Pair : PendingRequests)
    {
        if (Pair.Value.LoadHandle.IsValid())
        {
            Pair.Value.LoadHandle->CancelHandle();
        }
    }
    PendingRequests.Empty();

    Super::Deinitialize();
}

FKataCharacterSpawnHandle UKataCharacterSpawnSubsystem::RequestSpawn(const FKataCharacterId& CharacterId, const FTransform& SpawnTransform,
    FKataCharacterSpawnDelegate OnComplete, ESpawnActorCollisionHandlingMethod CollisionHandling)
{
    const UDataTable* Table = nullptr;
    const FKataCharacterRow* FoundRow = CharacterId.Find(&Table);
    if (FoundRow == nullptr)
    {
        UE_LOG(LogKataFramework, Warning, TEXT("RequestSpawn failed: character %s not found in the data collection."),
            *CharacterId.ToString());
        OnComplete.ExecuteIfBound(nullptr);
        return FKataCharacterSpawnHandle();
    }

    FPendingRequest Request;
    Request.CharacterId = CharacterId;
    // 로드 중에 테이블이 다시 로드되거나 편집돼도 요청이 영향을 받지 않도록 행을 실제 행 구조로 복사해 둔다.
    Request.RowData.InitializeAs(Table->GetRowStruct(), reinterpret_cast<const uint8*>(FoundRow));
    Request.SpawnTransform = SpawnTransform;
    Request.CollisionHandling = CollisionHandling;

    const FKataCharacterRow& CharacterRow = Request.RowData.Get<FKataCharacterRow>();
    if (CharacterRow.CharacterClass.IsNull())
    {
        UE_LOG(LogKataFramework, Warning, TEXT("RequestSpawn failed: row %s has no Character Class."), *CharacterId.ToString());
        OnComplete.ExecuteIfBound(nullptr);
        return FKataCharacterSpawnHandle();
    }

    TArray<FSoftObjectPath> AssetsToLoad;
    CharacterRow.GatherAssetsToLoad(AssetsToLoad);
    Request.OnComplete = MoveTemp(OnComplete);

    // 0은 무효 핸들이므로 건너뛴다.
    ++LastRequestId;
    if (LastRequestId == 0)
    {
        ++LastRequestId;
    }
    const uint32 RequestId = LastRequestId;

    // 로드 완료 콜백은 에셋이 이미 로드되어 있어도 다음 틱에 불리므로, 요청을 먼저 등록하고 로드를 시작해도 순서가 꼬이지 않는다.
    FPendingRequest& Stored = PendingRequests.Add(RequestId, MoveTemp(Request));
    Stored.LoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(AssetsToLoad,
        FStreamableDelegate::CreateUObject(this, &UKataCharacterSpawnSubsystem::HandleAssetsLoaded, RequestId),
        FStreamableManager::DefaultAsyncLoadPriority, false, false,
        FString::Printf(TEXT("KataCharacterSpawn %s"), *CharacterId.ToString()));

    if (!Stored.LoadHandle.IsValid())
    {
        // 로드 요청 자체가 만들어지지 않으면 완료 콜백도 오지 않으므로 여기서 실패로 끝낸다.
        UE_LOG(LogKataFramework, Warning, TEXT("RequestSpawn failed: could not start loading assets for row %s."), *CharacterId.ToString());
        FPendingRequest Failed;
        PendingRequests.RemoveAndCopyValue(RequestId, Failed);
        Failed.OnComplete.ExecuteIfBound(nullptr);
        return FKataCharacterSpawnHandle();
    }

    FKataCharacterSpawnHandle Handle;
    Handle.Id = RequestId;
    return Handle;
}

void UKataCharacterSpawnSubsystem::CancelSpawn(FKataCharacterSpawnHandle Handle)
{
    FPendingRequest Request;
    if (!Handle.IsValid() || !PendingRequests.RemoveAndCopyValue(Handle.Id, Request))
    {
        return;
    }

    if (Request.LoadHandle.IsValid())
    {
        Request.LoadHandle->CancelHandle();
    }
}

bool UKataCharacterSpawnSubsystem::IsSpawnPending(FKataCharacterSpawnHandle Handle) const
{
    return Handle.IsValid() && PendingRequests.Contains(Handle.Id);
}

void UKataCharacterSpawnSubsystem::HandleAssetsLoaded(uint32 RequestId)
{
    FPendingRequest Request;
    if (!PendingRequests.RemoveAndCopyValue(RequestId, Request))
    {
        // 취소된 요청이다.
        return;
    }

    AKataCharacter* Character = SpawnFromRequest(Request);

    // 적용된 에셋은 캐릭터와 컴포넌트가 참조하므로 로드 핸들을 더 붙잡지 않는다.
    if (Request.LoadHandle.IsValid())
    {
        Request.LoadHandle->ReleaseHandle();
    }

    Request.OnComplete.ExecuteIfBound(Character);
    if (Character != nullptr)
    {
        OnCharacterSpawned.Broadcast(Character, Request.CharacterId);
    }
}

AKataCharacter* UKataCharacterSpawnSubsystem::SpawnFromRequest(const FPendingRequest& Request) const
{
    const FString RowName = Request.CharacterId.ToString();
    UWorld* World = GetWorld();
    if (World == nullptr || World->bIsTearingDown)
    {
        return nullptr;
    }

    const FKataCharacterRow& Row = Request.RowData.Get<FKataCharacterRow>();
    TArray<FSoftObjectPath> AssetPaths;
    Row.GatherAssetsToLoad(AssetPaths);
    for (const FSoftObjectPath& AssetPath : AssetPaths)
    {
        // 선택 항목이라도 경로를 지정했다면 로드 실패를 Blueprint 기본값으로 숨기지 않는다.
        if (AssetPath.ResolveObject() == nullptr)
        {
            UE_LOG(LogKataFramework, Warning, TEXT("Spawn failed for row %s: asset %s failed to load."),
                *RowName, *AssetPath.ToString());
            return nullptr;
        }
    }

    UClass* CharacterClass = Row.CharacterClass.Get();
    if (CharacterClass == nullptr)
    {
        UE_LOG(LogKataFramework, Warning, TEXT("Spawn failed for row %s: Character Class is empty or failed to load."), *RowName);
        return nullptr;
    }

    // PC 행의 입력 설정과 그래프는 AKataPlayerCharacter만 적용할 수 있다. 적용되지 않은 채 생성되는 것을 막는다.
    if (Request.RowData.GetPtr<FKataPlayerCharacterRow>() != nullptr && !CharacterClass->IsChildOf(AKataPlayerCharacter::StaticClass()))
    {
        UE_LOG(LogKataFramework, Warning, TEXT("Spawn failed for row %s: player character rows require an AKataPlayerCharacter class, but %s is not."),
            *RowName, *CharacterClass->GetName());
        return nullptr;
    }

    AKataCharacter* Character = World->SpawnActorDeferred<AKataCharacter>(CharacterClass, Request.SpawnTransform,
        nullptr, nullptr, Request.CollisionHandling);
    if (Character == nullptr)
    {
        UE_LOG(LogKataFramework, Warning, TEXT("Spawn failed for row %s: SpawnActorDeferred returned null."), *RowName);
        return nullptr;
    }

    Character->FinishSpawningWithCharacterRow(Request.SpawnTransform, Request.RowData);

    // 충돌 처리 방식에 따라 FinishSpawning 중에 파괴될 수 있다.
    if (!IsValid(Character))
    {
        UE_LOG(LogKataFramework, Warning, TEXT("Spawn failed for row %s: character was destroyed while finishing spawn."), *RowName);
        return nullptr;
    }

    return Character;
}
