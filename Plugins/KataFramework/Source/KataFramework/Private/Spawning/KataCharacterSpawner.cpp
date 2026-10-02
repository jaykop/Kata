#include "Spawning/KataCharacterSpawner.h"

#include "Character/KataCharacter.h"
#include "Character/KataCharacterRow.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "KataFrameworkLog.h"
#include "Spawning/KataSpawnerComponent.h"
#include "Spawning/KataSpawnerComponent_SpawnSettings.h"
#include "UObject/UObjectGlobals.h"

AKataCharacterSpawner::AKataCharacterSpawner()
{
    PrimaryActorTick.bCanEverTick = false;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));

#if WITH_EDITORONLY_DATA
    SpawnAreaPreview = CreateEditorOnlyDefaultSubobject<UBoxComponent>(TEXT("SpawnAreaPreview"));
    if (SpawnAreaPreview != nullptr)
    {
        SpawnAreaPreview->SetupAttachment(GetRootComponent());
        // 생성자에서 SetBoxExtent를 부르면 BodySetup 생성이 생성자 검사에 걸리므로 Init 함수로 초기값만 둔다.
        SpawnAreaPreview->InitBoxExtent(FVector(200.0, 200.0, 0.0));
        SpawnAreaPreview->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        SpawnAreaPreview->SetCanEverAffectNavigation(false);
        SpawnAreaPreview->SetHiddenInGame(true);
        SpawnAreaPreview->ShapeColor = FColor(255, 170, 0);
        SpawnAreaPreview->SetLineThickness(2.0f);
        SpawnAreaPreview->bIsEditorOnly = true;
    }

    CharacterPreview = CreateEditorOnlyDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterPreview"));
    if (CharacterPreview != nullptr)
    {
        CharacterPreview->SetupAttachment(GetRootComponent());
        CharacterPreview->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        CharacterPreview->SetCanEverAffectNavigation(false);
        CharacterPreview->SetHiddenInGame(true);
        CharacterPreview->SetGenerateOverlapEvents(false);
        CharacterPreview->PrimaryComponentTick.bCanEverTick = false;
        CharacterPreview->bIsEditorOnly = true;
    }
#endif
}

void AKataCharacterSpawner::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    // Details에서 Spawn Settings를 편집하면 액터의 Construction이 다시 실행되므로 여기서 미리보기를 갱신한다.
    UpdateSpawnAreaPreview();
    UpdateCharacterPreview();
}

void AKataCharacterSpawner::BeginPlay()
{
    Super::BeginPlay();

    if (bSpawnOnBeginPlay)
    {
        SpawnCharacters();
    }
}

const UKataSpawnerComponent_SpawnSettings* AKataCharacterSpawner::FindEnabledSpawnSettings() const
{
    for (const TObjectPtr<UKataSpawnerComponent>& Component : SpawnerComponents)
    {
        const UKataSpawnerComponent_SpawnSettings* Settings = Cast<UKataSpawnerComponent_SpawnSettings>(Component);
        if (IsValid(Settings) && Settings->bEnabled)
        {
            return Settings;
        }
    }
    return nullptr;
}

void AKataCharacterSpawner::UpdateCharacterPreview()
{
#if WITH_EDITORONLY_DATA
    if (CharacterPreview == nullptr)
    {
        return;
    }

    // 게임 월드에서는 미리보기가 숨겨져 있으므로 동기 로드를 하지 않는다.
    const UWorld* World = GetWorld();
    if (World == nullptr || World->IsGameWorld())
    {
        return;
    }

    const UScriptStruct* RowStruct = CharacterRow.DataTable != nullptr ? CharacterRow.DataTable->GetRowStruct() : nullptr;
    const uint8* RowMemory = RowStruct != nullptr && RowStruct->IsChildOf(FKataCharacterRow::StaticStruct())
        ? CharacterRow.DataTable->FindRowUnchecked(CharacterRow.RowName) : nullptr;
    const FKataCharacterRow* Row = reinterpret_cast<const FKataCharacterRow*>(RowMemory);
    UClass* CharacterClass = Row != nullptr ? Row->CharacterClass.LoadSynchronous() : nullptr;
    const AKataCharacter* CharacterDefaults = CharacterClass != nullptr ? CharacterClass->GetDefaultObject<AKataCharacter>() : nullptr;
    const USkeletalMeshComponent* DefaultMesh = CharacterDefaults != nullptr ? CharacterDefaults->GetMesh() : nullptr;
    if (DefaultMesh == nullptr)
    {
        CharacterPreview->SetSkinnedAssetAndUpdate(nullptr);
        CharacterPreview->SetVisibility(false);
        return;
    }

    // 행의 메시가 비어 있으면 실제 생성과 같이 캐릭터 Blueprint의 기본 메시를 쓴다.
    USkeletalMesh* PreviewMesh = Row->SkeletalMesh.LoadSynchronous();
    if (PreviewMesh == nullptr)
    {
        PreviewMesh = DefaultMesh->GetSkeletalMeshAsset();
    }
    CharacterPreview->SetSkeletalMeshAsset(PreviewMesh);
    CharacterPreview->EmptyOverrideMaterials();
    for (int32 Index = 0; Index < DefaultMesh->OverrideMaterials.Num(); ++Index)
    {
        CharacterPreview->SetMaterial(Index, DefaultMesh->OverrideMaterials[Index]);
    }

    // GetSpawnTransform_Implementation의 기준을 따른다. 영역 중심에 영역 회전으로 놓고 스케일은 1로 둔다.
    FTransform SpawnTransform(GetActorRotation(), GetActorLocation());
    if (const UKataSpawnerComponent_SpawnSettings* Settings = FindEnabledSpawnSettings())
    {
        const FTransform AreaTransform = Settings->SpawnAreaTransform * GetActorTransform();
        SpawnTransform = FTransform(AreaTransform.GetRotation(), AreaTransform.GetLocation());
    }

    // 캐릭터의 메시는 캡슐 기준으로 내려가 있고 회전되어 있으므로 Blueprint의 상대 Transform을 곱해야 실제 모습과 겹친다.
    CharacterPreview->SetWorldTransform(DefaultMesh->GetRelativeTransform() * SpawnTransform);
    CharacterPreview->SetVisibility(PreviewMesh != nullptr);
#endif
}

void AKataCharacterSpawner::UpdateSpawnAreaPreview()
{
#if WITH_EDITORONLY_DATA
    if (SpawnAreaPreview == nullptr)
    {
        return;
    }

    const UKataSpawnerComponent_SpawnSettings* Settings = FindEnabledSpawnSettings();
    if (Settings == nullptr)
    {
        // 설정이 없으면 액터 위치 한 곳에서만 생성하므로 보여 줄 영역이 없다.
        SpawnAreaPreview->SetVisibility(false);
        return;
    }

    // GetSpawnTransform_Implementation과 같은 기준을 쓴다. 상대 Transform의 스케일은 영역 크기에 곱해진다.
    SpawnAreaPreview->SetRelativeTransform(Settings->SpawnAreaTransform);
    SpawnAreaPreview->SetBoxExtent(Settings->BoxExtent.ComponentMax(FVector::ZeroVector), false);
    SpawnAreaPreview->SetVisibility(true);
#endif
}

bool AKataCharacterSpawner::SpawnCharacters()
{
    UWorld* World = GetWorld();
    if (bStartingBatch || bSpawnBatchActive || bEndingPlay || World == nullptr || !World->IsGameWorld() || World->bIsTearingDown)
    {
        UE_LOG(LogKataFramework, Warning, TEXT("Spawner %s rejected spawn: a batch is active or the game world is unavailable."), *GetName());
        return false;
    }

    // Blueprint의 수량·위치 선택 함수가 같은 스포너를 재호출해도 설정 준비를 중복하지 않는다.
    TGuardValue<bool> StartingGuard(bStartingBatch, true);
    const FDataTableRowHandle RowForBatch = CharacterRow;
    // 위치 선택 Blueprint가 Row를 바꾸고 GC를 실행해도 요청 준비 중 원본 테이블을 유지한다.
    const TStrongObjectPtr<const UDataTable> TableKeepAlive(RowForBatch.DataTable.Get());
    const UScriptStruct* RowStruct = RowForBatch.DataTable != nullptr ? RowForBatch.DataTable->GetRowStruct() : nullptr;
    if (RowStruct == nullptr || !RowStruct->IsChildOf(FKataNPCCharacterRow::StaticStruct())
        || RowForBatch.DataTable->FindRowUnchecked(RowForBatch.RowName) == nullptr)
    {
        UE_LOG(LogKataFramework, Warning, TEXT("Spawner %s rejected spawn: a valid NPC row is required (%s:%s)."),
            *GetName(), *GetNameSafe(RowForBatch.DataTable), *RowForBatch.RowName.ToString());
        return false;
    }

    UKataCharacterSpawnSubsystem* Subsystem = World->GetSubsystem<UKataCharacterSpawnSubsystem>();
    if (Subsystem == nullptr)
    {
        return false;
    }

    // UObject 설정을 요청용으로 복사한다. 인라인 목록을 편집해도 진행 중인 완료 통지와 계산 설정은 고정된다.
    TArray<TStrongObjectPtr<UKataSpawnerComponent>> SourceComponents;
    for (const TObjectPtr<UKataSpawnerComponent>& Component : SpawnerComponents)
    {
        if (IsValid(Component) && Component->bEnabled)
        {
            SourceComponents.Emplace(Component.Get());
        }
    }

    TArray<TStrongObjectPtr<UKataSpawnerComponent>> PreparedComponents;
    const UKataSpawnerComponent_SpawnSettings* Settings = nullptr;
    for (const TStrongObjectPtr<UKataSpawnerComponent>& SourceComponent : SourceComponents)
    {
        UKataSpawnerComponent* Snapshot = DuplicateObject<UKataSpawnerComponent>(SourceComponent.Get(), this);
        if (!IsValid(Snapshot))
        {
            UE_LOG(LogKataFramework, Warning, TEXT("Spawner %s rejected spawn: failed to copy a spawner component."), *GetName());
            return false;
        }
        Snapshot->SetFlags(RF_Transient);
        PreparedComponents.Emplace(Snapshot);
        if (const UKataSpawnerComponent_SpawnSettings* SpawnSettings = Cast<UKataSpawnerComponent_SpawnSettings>(Snapshot))
        {
            if (Settings != nullptr)
            {
                UE_LOG(LogKataFramework, Warning, TEXT("Spawner %s rejected spawn: more than one enabled Spawn Settings entry."), *GetName());
                return false;
            }
            Settings = SpawnSettings;
        }
    }

    const FTransform SpawnerTransform = GetActorTransform();
    const int32 RequestedCount = Settings != nullptr ? Settings->CalculateSpawnCount() : 1;
    if (!IsValid(this) || bEndingPlay || World->bIsTearingDown)
    {
        return false;
    }
    if (RequestedCount < 0)
    {
        UE_LOG(LogKataFramework, Warning, TEXT("Spawner %s rejected spawn: negative spawn count %d."), *GetName(), RequestedCount);
        return false;
    }

    const ESpawnActorCollisionHandlingMethod CollisionHandling = Settings != nullptr
        ? Settings->CollisionHandling : ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    TArray<FTransform> Transforms;
    Transforms.Reserve(RequestedCount);
    for (int32 SpawnIndex = 0; SpawnIndex < RequestedCount; ++SpawnIndex)
    {
        FTransform SpawnTransform = SpawnerTransform;
        if ((Settings != nullptr && !Settings->GetSpawnTransform(SpawnerTransform, SpawnIndex, SpawnTransform))
            || !SpawnTransform.IsValid())
        {
            UE_LOG(LogKataFramework, Warning, TEXT("Spawner %s rejected spawn: invalid transform for index %d."), *GetName(), SpawnIndex);
            return false;
        }
        if (!IsValid(this) || bEndingPlay || World->bIsTearingDown)
        {
            return false;
        }
        Transforms.Add(SpawnTransform);
    }

    ++BatchId;
    if (BatchId == 0)
    {
        ++BatchId;
    }
    const uint32 SubmittedBatchId = BatchId;
    ActiveRow = RowForBatch;
    ActiveSubsystem = Subsystem;
    SucceededCount = 0;
    FailedCount = 0;
    bSpawnBatchActive = true;
    SpawnedCharacters.RemoveAll([](const TWeakObjectPtr<AKataCharacter>& Character) { return !Character.IsValid(); });

    for (const TStrongObjectPtr<UKataSpawnerComponent>& Component : PreparedComponents)
    {
        ActiveComponents.Add(Component.Get());
    }

    // 요청 단계의 실패는 즉시 콜백을 부를 수 있다. 전체 예약을 먼저 만들어 작업이 중간에 완료되지 않게 한다.
    for (int32 SpawnIndex = 0; SpawnIndex < RequestedCount; ++SpawnIndex)
    {
        PendingRequests.Add(SpawnIndex, FKataCharacterSpawnHandle());
    }
    if (RequestedCount == 0)
    {
        FinishSpawnBatch(false, true);
        return true;
    }

    for (int32 SpawnIndex = 0; SpawnIndex < RequestedCount; ++SpawnIndex)
    {
        if (!bSpawnBatchActive || BatchId != SubmittedBatchId || bEndingPlay)
        {
            break;
        }
        const FKataCharacterSpawnHandle Handle = Subsystem->RequestSpawn(RowForBatch, Transforms[SpawnIndex],
            FKataCharacterSpawnDelegate::CreateUObject(this, &AKataCharacterSpawner::HandleSpawnCompleted, SubmittedBatchId, SpawnIndex),
            CollisionHandling);

        if (bSpawnBatchActive && BatchId == SubmittedBatchId)
        {
            if (FKataCharacterSpawnHandle* StoredHandle = PendingRequests.Find(SpawnIndex))
            {
                *StoredHandle = Handle;
                continue;
            }
        }
        // 즉시 콜백 안에서 취소된 경우에도 반환된 핸들이 남지 않도록 한다.
        if (IsValid(Subsystem))
        {
            Subsystem->CancelSpawn(Handle);
        }
    }
    return true;
}

void AKataCharacterSpawner::CancelSpawning()
{
    if (bSpawnBatchActive)
    {
        FinishSpawnBatch(true, true);
    }
}

int32 AKataCharacterSpawner::GetSpawnedCharacterCount() const
{
    int32 Count = 0;
    for (const TWeakObjectPtr<AKataCharacter>& Character : SpawnedCharacters)
    {
        if (Character.IsValid() && !Character->IsActorBeingDestroyed())
        {
            ++Count;
        }
    }
    return Count;
}

TArray<AKataCharacter*> AKataCharacterSpawner::GetSpawnedCharacters() const
{
    TArray<AKataCharacter*> Characters;
    for (const TWeakObjectPtr<AKataCharacter>& Character : SpawnedCharacters)
    {
        if (Character.IsValid() && !Character->IsActorBeingDestroyed())
        {
            Characters.Add(Character.Get());
        }
    }
    return Characters;
}

void AKataCharacterSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    bEndingPlay = true;
    if (bSpawnBatchActive)
    {
        FinishSpawnBatch(true, false);
    }
    SpawnedCharacters.Reset();
    Super::EndPlay(EndPlayReason);
}

void AKataCharacterSpawner::HandleSpawnCompleted(AKataCharacter* Character, uint32 RequestBatchId, int32 SpawnIndex)
{
    if (!bSpawnBatchActive || BatchId != RequestBatchId || bEndingPlay || PendingRequests.Remove(SpawnIndex) == 0)
    {
        return;
    }

    if (IsValid(Character))
    {
        ++SucceededCount;
        SpawnedCharacters.Add(Character);
        const FDataTableRowHandle CompletedRow = ActiveRow;
        TArray<TWeakObjectPtr<UKataSpawnerComponent>> ComponentsForResult;
        for (const TObjectPtr<UKataSpawnerComponent>& Component : ActiveComponents)
        {
            ComponentsForResult.Add(Component.Get());
        }
        for (const TWeakObjectPtr<UKataSpawnerComponent>& Component : ComponentsForResult)
        {
            if (!bSpawnBatchActive || BatchId != RequestBatchId || bEndingPlay || !IsValid(Character))
            {
                break;
            }
            if (Component.IsValid())
            {
                // 옵션 안에서 취소와 GC를 실행해도 현재 실행하는 설정 사본의 수명은 유지한다.
                const TStrongObjectPtr<UKataSpawnerComponent> ComponentKeepAlive(Component.Get());
                Component->OnCharacterSpawned(this, Character, CompletedRow);
            }
        }
        if (bSpawnBatchActive && BatchId == RequestBatchId && !bEndingPlay && IsValid(Character))
        {
            OnCharacterSpawned.Broadcast(Character, SpawnIndex);
        }
    }
    else
    {
        ++FailedCount;
        OnCharacterSpawnFailed.Broadcast(SpawnIndex);
    }

    // 결과 이벤트에서 작업을 취소하거나 스포너를 제거했을 수 있다.
    if (bSpawnBatchActive && BatchId == RequestBatchId && !bEndingPlay && PendingRequests.Num() == 0)
    {
        FinishSpawnBatch(false, true);
    }
}

void AKataCharacterSpawner::FinishSpawnBatch(bool bCancelled, bool bBroadcast)
{
    const int32 CompletedSuccessCount = SucceededCount;
    const int32 CompletedFailureCount = FailedCount;
    TMap<int32, FKataCharacterSpawnHandle> RequestsToCancel = MoveTemp(PendingRequests);
    PendingRequests.Reset();
    const TWeakObjectPtr<UKataCharacterSpawnSubsystem> Subsystem = ActiveSubsystem;
    bSpawnBatchActive = false;
    ActiveRow = FDataTableRowHandle();
    ActiveComponents.Reset();
    ActiveSubsystem.Reset();

    if (Subsystem.IsValid())
    {
        for (const TPair<int32, FKataCharacterSpawnHandle>& Request : RequestsToCancel)
        {
            Subsystem->CancelSpawn(Request.Value);
        }
    }
    if (bBroadcast && !bEndingPlay)
    {
        OnBatchFinished.Broadcast(CompletedSuccessCount, CompletedFailureCount, bCancelled);
    }
}
