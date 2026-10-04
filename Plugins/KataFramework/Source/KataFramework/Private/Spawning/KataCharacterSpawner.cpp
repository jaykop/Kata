#include "Spawning/KataCharacterSpawner.h"

#include "Character/KataCharacter.h"
#include "Character/KataCharacterRow.h"
#include "Data/KataDataCollection.h"
#include "Data/KataDataSettings.h"
#include "Engine/DataTable.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "KataFrameworkLog.h"
#include "Spawning/KataSpawnerComponent.h"
#include "Spawning/KataSpawnerComponent_SpawnArea.h"
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

    SphereAreaPreview = CreateEditorOnlyDefaultSubobject<USphereComponent>(TEXT("SphereAreaPreview"));
    if (SphereAreaPreview != nullptr)
    {
        SphereAreaPreview->SetupAttachment(GetRootComponent());
        SphereAreaPreview->InitSphereRadius(200.f);
        SphereAreaPreview->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        SphereAreaPreview->SetCanEverAffectNavigation(false);
        SphereAreaPreview->SetHiddenInGame(true);
        SphereAreaPreview->ShapeColor = FColor(255, 170, 0);
        SphereAreaPreview->SetLineThickness(2.0f);
        SphereAreaPreview->bIsEditorOnly = true;
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

    // Details에서 Spawn Area를 편집하면 액터의 Construction이 다시 실행되므로 여기서 미리보기를 갱신한다.
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

const UKataSpawnerComponent_SpawnArea* AKataCharacterSpawner::FindEnabledSpawnArea() const
{
    for (const TObjectPtr<UKataSpawnerComponent>& Component : SpawnerComponents)
    {
        const UKataSpawnerComponent_SpawnArea* Settings = Cast<UKataSpawnerComponent_SpawnArea>(Component);
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

    const FKataCharacterRow* Row = CharacterId.Find();
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
    if (const UKataSpawnerComponent_SpawnArea* Settings = FindEnabledSpawnArea())
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
    if (SpawnAreaPreview == nullptr || SphereAreaPreview == nullptr)
    {
        return;
    }

    const UKataSpawnerComponent_SpawnArea* Settings = FindEnabledSpawnArea();
    if (Settings == nullptr)
    {
        // 설정이 없으면 액터 위치 한 곳에서만 생성하므로 보여 줄 영역이 없다.
        SpawnAreaPreview->SetVisibility(false);
        SphereAreaPreview->SetVisibility(false);
        return;
    }

    // GetSpawnTransform_Implementation과 같은 기준을 쓴다. 상대 Transform의 스케일은 영역 크기에 곱해진다.
    const bool bSphere = Settings->AreaShape == EKataSpawnAreaShape::Sphere;
    SpawnAreaPreview->SetRelativeTransform(Settings->SpawnAreaTransform);
    SpawnAreaPreview->SetBoxExtent(Settings->BoxExtent.ComponentMax(FVector::ZeroVector), false);
    SpawnAreaPreview->SetVisibility(!bSphere);
    SphereAreaPreview->SetRelativeTransform(Settings->SpawnAreaTransform);
    SphereAreaPreview->SetSphereRadius(FMath::Max(Settings->SphereRadius, 0.f), false);
    SphereAreaPreview->SetVisibility(bSphere);
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
    // 위치 선택 Blueprint가 ID를 바꿔도 이번 작업은 시작 시점의 ID로 고정한다.
    const FKataCharacterId IdForBatch = CharacterId;
    // ID는 PC·NPC 테이블을 함께 찾으므로 NPC 행인지, Source Table이 있으면 그 테이블의 행인지 여기서 확인한다. 테이블은 데이터 설정이 유지한다.
    const UDataTable* Table = nullptr;
    if (IdForBatch.Find(&Table) == nullptr || !Table->GetRowStruct()->IsChildOf(FKataNPCCharacterRow::StaticStruct()))
    {
        UE_LOG(LogKataFramework, Warning, TEXT("Spawner %s rejected spawn: a valid NPC row is required (%s)."),
            *GetName(), *IdForBatch.ToString());
        return false;
    }
    if (SourceTable != nullptr)
    {
        const UKataDataCollection* Collection = UKataDataSettings::Get()->GetDataCollection();
        if (Collection == nullptr || !Collection->IsNPCCharacterTable(SourceTable))
        {
            UE_LOG(LogKataFramework, Warning, TEXT("Spawner %s rejected spawn: Source Table %s is not an NPC table of the data collection."),
                *GetName(), *GetNameSafe(SourceTable));
            return false;
        }
        if (Table != SourceTable)
        {
            UE_LOG(LogKataFramework, Warning, TEXT("Spawner %s rejected spawn: row %s is in %s, not in Source Table %s."),
                *GetName(), *IdForBatch.ToString(), *GetNameSafe(Table), *GetNameSafe(SourceTable));
            return false;
        }
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
    const UKataSpawnerComponent_SpawnArea* Settings = nullptr;
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
        if (const UKataSpawnerComponent_SpawnArea* SpawnArea = Cast<UKataSpawnerComponent_SpawnArea>(Snapshot))
        {
            if (Settings != nullptr)
            {
                UE_LOG(LogKataFramework, Warning, TEXT("Spawner %s rejected spawn: more than one enabled Spawn Area entry."), *GetName());
                return false;
            }
            Settings = SpawnArea;
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
    // 위치 보정 설정이 후보를 거절하면 Spawn Area에서 다시 뽑는다. 가장 많은 시도를 요구하는 설정을 따른다.
    int32 MaxPlacementAttempts = 1;
    for (const TStrongObjectPtr<UKataSpawnerComponent>& Component : PreparedComponents)
    {
        MaxPlacementAttempts = FMath::Max(MaxPlacementAttempts, Component->GetPlacementAttempts());
    }

    TArray<FTransform> Transforms;
    TArray<bool> PlacementFailed;
    Transforms.Reserve(RequestedCount);
    PlacementFailed.Reserve(RequestedCount);
    for (int32 SpawnIndex = 0; SpawnIndex < RequestedCount; ++SpawnIndex)
    {
        FTransform SpawnTransform = SpawnerTransform;
        bool bPlaced = false;
        for (int32 Attempt = 0; Attempt < MaxPlacementAttempts && !bPlaced; ++Attempt)
        {
            SpawnTransform = SpawnerTransform;
            // 후보를 낼 수 없는 것은 설정 오류이므로 다시 뽑지 않고 작업 전체를 거절한다.
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

            bPlaced = true;
            for (const TStrongObjectPtr<UKataSpawnerComponent>& Component : PreparedComponents)
            {
                FTransform Adjusted = SpawnTransform;
                const bool bAccepted = Component->AdjustSpawnTransform(this, Settings, SpawnIndex, SpawnTransform, Adjusted);
                if (!IsValid(this) || bEndingPlay || World->bIsTearingDown)
                {
                    return false;
                }
                if (!bAccepted || !Adjusted.IsValid())
                {
                    bPlaced = false;
                    break;
                }
                SpawnTransform = Adjusted;
            }
        }
        if (!bPlaced)
        {
            UE_LOG(LogKataFramework, Warning, TEXT("Spawner %s could not place index %d after %d attempt(s). It will be reported as failed."),
                *GetName(), SpawnIndex, MaxPlacementAttempts);
        }
        Transforms.Add(SpawnTransform);
        PlacementFailed.Add(!bPlaced);
    }

    ++BatchId;
    if (BatchId == 0)
    {
        ++BatchId;
    }
    const uint32 SubmittedBatchId = BatchId;
    ActiveCharacterId = IdForBatch;
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
        if (PlacementFailed[SpawnIndex])
        {
            // 요청 단계에서 실패한 생성과 같은 경로로 알린다. 실패 이벤트와 완료 집계가 한 곳에서 처리된다.
            HandleSpawnCompleted(nullptr, SubmittedBatchId, SpawnIndex);
            continue;
        }
        const FKataCharacterSpawnHandle Handle = Subsystem->RequestSpawn(IdForBatch, Transforms[SpawnIndex],
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
        const FKataCharacterId CompletedCharacterId = ActiveCharacterId;
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
                Component->OnCharacterSpawned(this, Character, CompletedCharacterId);
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
    ActiveCharacterId = FKataCharacterId();
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
