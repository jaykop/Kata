#include "Character/KataCharacterSpawnSubsystem.h"

#include "Character/KataCharacter.h"
#include "Character/KataCharacterSpawnOwnership.h"
#include "Character/KataAICharacter.h"
#include "Character/KataCharacterRow.h"
#include "Character/KataPlayerCharacter.h"
#include "Engine/AssetManager.h"
#include "Engine/DataTable.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"
#include "KataFrameworkLog.h"
#include "Spawning/KataFL_Spawning.h"

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
    SpawnGroups.Empty();
    BudgetedPendingCount = 0;

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

    // 로드 중에 테이블이 다시 로드되거나 편집돼도 요청이 영향을 받지 않도록 행을 실제 행 구조로 복사해 둔다.
    FInstancedStruct RowData;
    RowData.InitializeAs(Table->GetRowStruct(), reinterpret_cast<const uint8*>(FoundRow));
    return RequestSpawnPrepared(CharacterId, MoveTemp(RowData), SpawnTransform, MoveTemp(OnComplete), CollisionHandling);
}

FKataCharacterSpawnHandle UKataCharacterSpawnSubsystem::RequestSpawnFromRow(const FKataCharacterId& CharacterId,
    const FInstancedStruct& RowData, const FTransform& SpawnTransform, FKataCharacterSpawnDelegate OnComplete,
    ESpawnActorCollisionHandlingMethod CollisionHandling, TSharedPtr<FKataCharacterSpawnOwnership> Ownership)
{
    return RequestSpawnPrepared(CharacterId, RowData, SpawnTransform, MoveTemp(OnComplete), CollisionHandling, 0, MoveTemp(Ownership));
}

FKataCharacterSpawnHandle UKataCharacterSpawnSubsystem::RequestSpawnPrepared(const FKataCharacterId& CharacterId,
    FInstancedStruct RowData, const FTransform& SpawnTransform, FKataCharacterSpawnDelegate OnComplete,
    ESpawnActorCollisionHandlingMethod CollisionHandling, uint32 GroupId, TSharedPtr<FKataCharacterSpawnOwnership> Ownership)
{
    if (GroupId != 0 && !SpawnGroups.Contains(GroupId))
    {
        OnComplete.ExecuteIfBound(nullptr);
        return FKataCharacterSpawnHandle();
    }
    if (RowData.GetPtr<FKataCharacterRow>() == nullptr)
    {
        UE_LOG(LogKataFramework, Warning, TEXT("RequestSpawn failed: character %s requires a valid character row snapshot."),
            *CharacterId.ToString());
        OnComplete.ExecuteIfBound(nullptr);
        return FKataCharacterSpawnHandle();
    }

    FPendingRequest Request;
    Request.CharacterId = CharacterId;
    Request.RowData = MoveTemp(RowData);
    Request.SpawnTransform = SpawnTransform;
    Request.CollisionHandling = CollisionHandling;
    Request.GroupId = GroupId;
    Request.Ownership = Ownership.IsValid() ? MoveTemp(Ownership) : MakeShared<FKataCharacterSpawnOwnership>();

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

    if (GroupId != 0)
    {
        SpawnGroups.FindChecked(GroupId)->RequestIds.Add(RequestId);
        ++BudgetedPendingCount;
    }

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
        TakePendingRequest(RequestId, Failed);
        Failed.OnComplete.ExecuteIfBound(nullptr);
        return FKataCharacterSpawnHandle();
    }

    FKataCharacterSpawnHandle Handle;
    Handle.Id = RequestId;
    return Handle;
}

FKataCharacterSpawnGroupHandle UKataCharacterSpawnSubsystem::CreateSpawnGroup()
{
    ++LastGroupId;
    if (LastGroupId == 0)
    {
        ++LastGroupId;
    }
    SpawnGroups.Add(LastGroupId, MakeUnique<FSpawnGroup>());
    FKataCharacterSpawnGroupHandle Group;
    Group.Id = LastGroupId;
    return Group;
}

void UKataCharacterSpawnSubsystem::CancelSpawnGroup(FKataCharacterSpawnGroupHandle Group)
{
    TUniquePtr<FSpawnGroup>* Found = SpawnGroups.Find(Group.Id);
    if (!Group.IsValid() || Found == nullptr)
    {
        return;
    }
    TUniquePtr<FSpawnGroup> Removed = MoveTemp(*Found);
    SpawnGroups.Remove(Group.Id);
    // 콜백에 진입한 요청도 그룹 부재로 취소를 알 수 있게 먼저 등록을 해제한다.
    for (uint32 RequestId : Removed->RequestIds)
    {
        FKataCharacterSpawnHandle Handle;
        Handle.Id = RequestId;
        CancelSpawn(Handle);
    }
}

FKataCharacterSpawnHandle UKataCharacterSpawnSubsystem::RequestSpawnFromRowBudgeted(FKataCharacterSpawnGroupHandle Group,
    const FKataCharacterId& CharacterId, const FInstancedStruct& RowData, const FTransform& SpawnTransform,
    FKataCharacterSpawnDelegate OnComplete, ESpawnActorCollisionHandlingMethod CollisionHandling,
    TSharedPtr<FKataCharacterSpawnOwnership> Ownership)
{
    if (!Group.IsValid())
    {
        OnComplete.ExecuteIfBound(nullptr);
        return FKataCharacterSpawnHandle();
    }
    return RequestSpawnPrepared(CharacterId, RowData, SpawnTransform, MoveTemp(OnComplete), CollisionHandling, Group.Id, MoveTemp(Ownership));
}

bool UKataCharacterSpawnSubsystem::TakePendingRequest(uint32 RequestId, FPendingRequest& OutRequest)
{
    if (!PendingRequests.RemoveAndCopyValue(RequestId, OutRequest))
    {
        return false;
    }
    if (OutRequest.GroupId != 0)
    {
        --BudgetedPendingCount;
        if (TUniquePtr<FSpawnGroup>* Group = SpawnGroups.Find(OutRequest.GroupId))
        {
            (*Group)->RequestIds.Remove(RequestId);
        }
    }
    return true;
}

EKataReadySpawnResult UKataCharacterSpawnSubsystem::ProcessNextReadySpawn(FKataCharacterSpawnGroupHandle Group)
{
    TUniquePtr<FSpawnGroup>* Found = SpawnGroups.Find(Group.Id);
    uint32 RequestId = 0;
    if (Found == nullptr || !(*Found)->ReadyRequests.Dequeue(RequestId))
    {
        return EKataReadySpawnResult::NoWork;
    }
    const FPendingRequest* Request = PendingRequests.Find(RequestId);
    if (Request == nullptr || Request->GroupId != Group.Id || !Request->bReady)
    {
        return EKataReadySpawnResult::Discarded;
    }
    // 생성·Blueprint 콜백이 그룹이나 맵을 변경하므로 그 안의 참조를 호출 이후 사용하지 않는다.
    CompleteSpawnRequest(RequestId);
    return EKataReadySpawnResult::Processed;
}

void UKataCharacterSpawnSubsystem::CancelSpawn(FKataCharacterSpawnHandle Handle)
{
    FPendingRequest Request;
    if (!Handle.IsValid() || !TakePendingRequest(Handle.Id, Request))
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
    FPendingRequest* Pending = PendingRequests.Find(RequestId);
    if (Pending == nullptr)
    {
        return;
    }
    if (Pending->GroupId != 0)
    {
        if (!Pending->bReady)
        {
            if (TUniquePtr<FSpawnGroup>* Group = SpawnGroups.Find(Pending->GroupId))
            {
                Pending->bReady = true;
                (*Group)->ReadyRequests.Enqueue(RequestId);
            }
        }
        // 준비 완료 에셋은 실제 생성이나 취소까지 로드 핸들로 유지한다.
        return;
    }
    CompleteSpawnRequest(RequestId);
}

void UKataCharacterSpawnSubsystem::CompleteSpawnRequest(uint32 RequestId)
{
    FPendingRequest Request;
    if (!TakePendingRequest(RequestId, Request))
    {
        // 취소된 요청이다.
        return;
    }

    AKataCharacter* Character = SpawnFromRequest(Request);

    const bool bCancelledWhileSpawning = Request.GroupId != 0 && !SpawnGroups.Contains(Request.GroupId);
    const bool bDespawnQueued = Request.Ownership->IsDespawnRequested();
    if ((bCancelledWhileSpawning || Character == nullptr) && !bDespawnQueued)
    {
        // 별도 디스폰 작업이 없다면 결과 전달 전 취소된 생성 자원만 여기서 정리한다.
        if (KataFL::DestroySpawnedCharacter(*Request.Ownership))
        {
            for (int32 Index = 0; Index < Request.Ownership->OwnedControllers.Num(); ++Index)
            {
                KataFL::DestroySpawnOwnedController(*Request.Ownership, Index);
            }
        }
        Character = nullptr;
    }

    // 적용된 에셋은 캐릭터와 컴포넌트가 참조하므로 로드 핸들을 더 붙잡지 않는다.
    if (Request.LoadHandle.IsValid())
    {
        Request.LoadHandle->ReleaseHandle();
    }

    if (bCancelledWhileSpawning || bDespawnQueued)
    {
        return;
    }

    Request.OnComplete.ExecuteIfBound(Character);
    if (IsValid(Character) && !Character->IsActorBeingDestroyed())
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

    const FKataNPCCharacterRow* NPCRow = Request.RowData.GetPtr<FKataNPCCharacterRow>();
    if (NPCRow != nullptr && NPCRow->AIData.IsNull() && !NPCRow->StateTree.IsNull())
    {
        UE_LOG(LogKataFramework, Warning, TEXT("Spawn failed for row %s: move the legacy StateTree into AIData before spawning."), *RowName);
        return nullptr;
    }
    if (NPCRow != nullptr && (!NPCRow->AIControllerClass.IsNull() || !NPCRow->AIData.IsNull())
        && !CharacterClass->IsChildOf(AKataAICharacter::StaticClass()))
    {
        UE_LOG(LogKataFramework, Warning, TEXT("Spawn failed for row %s: AI settings require an AKataAICharacter class."), *RowName);
        return nullptr;
    }

    AKataCharacter* Character = World->SpawnActorDeferred<AKataCharacter>(CharacterClass, Request.SpawnTransform,
        nullptr, nullptr, Request.CollisionHandling);
    if (Character == nullptr)
    {
        UE_LOG(LogKataFramework, Warning, TEXT("Spawn failed for row %s: SpawnActorDeferred returned null."), *RowName);
        return nullptr;
    }

    Request.Ownership->Character = Character;
    Character->SetSpawnOwnership(Request.Ownership);

    Character->FinishSpawningWithCharacterRow(Request.SpawnTransform, Request.RowData);

    // 충돌 처리 방식에 따라 FinishSpawning 중에 파괴될 수 있다.
    if (!IsValid(Character))
    {
        UE_LOG(LogKataFramework, Warning, TEXT("Spawn failed for row %s: character was destroyed while finishing spawn."), *RowName);
        return nullptr;
    }

    return Character;
}
