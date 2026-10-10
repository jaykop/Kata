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
#include "GameFramework/Controller.h"
#include "KataFrameworkLog.h"
#include "Spawning/KataSpawnerComponent.h"
#include "Spawning/KataSpawnerComponent_AIOverride.h"
#include "Spawning/KataSpawnerComponent_SpawnArea.h"
#include "Spawning/KataSpawnerSubsystem.h"
#include "UObject/StrongObjectPtr.h"
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

    // 거리 범위는 수십 m라 항상 그리면 여러 스포너의 원이 겹쳐 영역 미리보기를 가리므로 선택했을 때만 그린다.
    auto CreateDistancePreview = [this](const TCHAR* Name, const FColor& Color) -> USphereComponent*
    {
        USphereComponent* Preview = CreateEditorOnlyDefaultSubobject<USphereComponent>(Name);
        if (Preview != nullptr)
        {
            Preview->SetupAttachment(GetRootComponent());
            Preview->InitSphereRadius(0.f);
            Preview->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Preview->SetCanEverAffectNavigation(false);
            Preview->SetHiddenInGame(true);
            Preview->SetUsingAbsoluteScale(true);
            Preview->ShapeColor = Color;
            Preview->bDrawOnlyIfSelected = true;
            Preview->bIsEditorOnly = true;
        }
        return Preview;
    };
    SpawnDistancePreview = CreateDistancePreview(TEXT("SpawnDistancePreview"), FColor(80, 220, 120));
    DespawnDistancePreview = CreateDistancePreview(TEXT("DespawnDistancePreview"), FColor(230, 80, 80));
#endif
}

void AKataCharacterSpawner::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    // Details에서 Spawn Area를 편집하면 액터의 Construction이 다시 실행되므로 여기서 미리보기를 갱신한다.
    UpdateSpawnAreaPreview();
    UpdateCharacterPreview();
    UpdateDistancePreview();
}

void AKataCharacterSpawner::PostLoad()
{
    Super::PostLoad();

    // 이전 bSpawnOnBeginPlay=false는 직접 호출 방식이었다. 기본값 true는 저장되지 않으므로 false일 때만 옮긴다.
    if (!bSpawnOnBeginPlay_DEPRECATED)
    {
        Activation = EKataSpawnerActivation::Manual;
        bSpawnOnBeginPlay_DEPRECATED = true;
    }
}

void AKataCharacterSpawner::UpdateDistancePreview()
{
#if WITH_EDITORONLY_DATA
    if (SpawnDistancePreview == nullptr || DespawnDistancePreview == nullptr)
    {
        return;
    }
    const bool bVisible = Activation == EKataSpawnerActivation::PlayerDistance;
    // 판정은 액터 위치 기준 3D 거리이므로 액터 스케일과 무관한 절대 반지름으로 그린다.
    SpawnDistancePreview->SetSphereRadius(FMath::Max(SpawnDistance, 0.f), false);
    DespawnDistancePreview->SetSphereRadius(FMath::Max(DespawnDistance, 0.f), false);
    SpawnDistancePreview->SetVisibility(bVisible);
    DespawnDistancePreview->SetVisibility(bVisible);
#endif
}

void AKataCharacterSpawner::BeginPlay()
{
    Super::BeginPlay();

    switch (Activation)
    {
    case EKataSpawnerActivation::BeginPlay:
        SpawnCharacters();
        break;
    case EKataSpawnerActivation::PlayerDistance:
        // 바로 생성하지 않고 관리자의 첫 거리 평가를 기다린다.
        InitializeDistanceManagement();
        break;
    default:
        break;
    }
}

void AKataCharacterSpawner::InitializeDistanceManagement()
{
    // 잘못된 거리로 일반 생성을 하면 거리 관리를 기대한 배치에서 NPC가 계속 남으므로 생성하지 않는다.
    if (!FMath::IsFinite(SpawnDistance) || !FMath::IsFinite(DespawnDistance) || SpawnDistance <= 0.f
        || DespawnDistance <= SpawnDistance)
    {
        UE_LOG(LogKataFramework, Warning,
            TEXT("Spawner %s will not spawn: Player Distance requires 0 < SpawnDistance (%.1f) < DespawnDistance (%.1f)."),
            *GetName(), SpawnDistance, DespawnDistance);
        return;
    }
    UWorld* World = GetWorld();
    UKataSpawnerSubsystem* Scheduler = World != nullptr ? World->GetSubsystem<UKataSpawnerSubsystem>() : nullptr;
    if (Scheduler == nullptr || !Scheduler->CanScheduleWork())
    {
        UE_LOG(LogKataFramework, Warning, TEXT("Spawner %s will not spawn: the spawner subsystem is unavailable."), *GetName());
        return;
    }
    DistanceSpawnRange = SpawnDistance;
    DistanceDespawnRange = DespawnDistance;
    PendingRespawnCount = 0;
    bOriginInRange = false;
    bInitialSpawnPending = true;
    bEntrySpawnPending = false;
    bDistanceManaged = true;
    Scheduler->RegisterDistanceSpawner(this, DistanceSpawnRange);
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
    if (Activation == EKataSpawnerActivation::Disabled)
    {
        UE_LOG(LogKataFramework, Warning, TEXT("Spawner %s rejected spawn: the spawner is disabled."), *GetName());
        return false;
    }
    if (bDistanceManaged)
    {
        UE_LOG(LogKataFramework, Warning, TEXT("Spawner %s rejected spawn: spawning is managed by Player Distance activation."), *GetName());
        return false;
    }
    return StartSpawnBatch(INDEX_NONE, false);
}

bool AKataCharacterSpawner::StartSpawnBatch(int32 CountOverride, bool bForceTimeSlicing)
{
    if (bStartingBatch || bSpawnBatchActive || IsDespawning() || !CanContinueSpawning())
    {
        UE_LOG(LogKataFramework, Warning, TEXT("Spawner %s rejected spawn: spawning, despawning, or game world unavailable."), *GetName());
        return false;
    }

    // 준비 중 Blueprint 재호출을 거절하고 호출 종료까지 실행 데이터의 수명을 유지한다.
    TGuardValue<bool> StartingGuard(bStartingBatch, true);
    const TStrongObjectPtr<UKataSpawnBatchState> Prepared(NewObject<UKataSpawnBatchState>(this));
    Prepared->bTimeSliced = bUseTimeSlicing || bForceTimeSlicing;
    TGuardValue<TObjectPtr<UKataSpawnBatchState>> PreparingGuard(PreparingBatch, Prepared.Get());
    if (!PrepareSpawnBatch(*Prepared.Get(), CountOverride))
    {
        return false;
    }

    // 기존 경로는 전체 후보 사전 확인을 유지하고 분산 경로는 수락 뒤 하나씩 확인한다.
    while (!Prepared->bTimeSliced && Prepared->Progress.PlacementIndex < Prepared->Context.RequestedCount)
    {
        if (!AdvanceSpawnPlacement(*Prepared.Get()))
        {
            return false;
        }
    }

    if (Prepared->bTimeSliced && Prepared->Context.RequestedCount > 0)
    {
        Prepared->Scheduler = GetWorld()->GetSubsystem<UKataSpawnerSubsystem>();
        if (!Prepared->Scheduler.IsValid())
        {
            return false;
        }
        Prepared->SpawnGroup = Prepared->Subsystem->CreateSpawnGroup();
    }

    ++BatchId;
    if (BatchId == 0)
    {
        ++BatchId;
    }
    const uint32 SubmittedBatchId = BatchId;
    Prepared->Generation = MakeShared<FKataSpawnGeneration>();
    Prepared->Generation->Id = SubmittedBatchId;
    Prepared->RemainingCount = Prepared->Context.RequestedCount;
    ActiveBatch = Prepared.Get();
    bSpawnBatchActive = true;
    SpawnedCharacters.RemoveAll([](const TWeakObjectPtr<AKataCharacter>& Character) { return !Character.IsValid(); });

    if (Prepared->RemainingCount == 0)
    {
        FinishSpawnBatch(false, true);
        return true;
    }

    if (Prepared->bTimeSliced)
    {
        Prepared->RegistrationId = Prepared->Scheduler->RegisterSpawnBatch(this, SubmittedBatchId, Prepared->SpawnGroup);
        if (Prepared->RegistrationId == 0)
        {
            FinishSpawnBatch(true, false);
            return false;
        }
        return true;
    }

    while (bSpawnBatchActive && BatchId == SubmittedBatchId && !bEndingPlay
        && Prepared->Progress.SubmissionIndex < Prepared->Context.RequestedCount)
    {
        SubmitNextSpawnRequest(SubmittedBatchId);
    }
    Prepared->PreparedTransforms.Reset();
    Prepared->PlacementFailed.Reset();
    return true;
}

bool AKataCharacterSpawner::CanContinueSpawning() const
{
    const UWorld* World = GetWorld();
    return IsValid(this) && !IsActorBeingDestroyed() && !bEndingPlay && World != nullptr
        && World->IsGameWorld() && !World->bIsTearingDown;
}

bool AKataCharacterSpawner::IsScheduledBatchActive(uint32 ExpectedBatchId) const
{
    return bSpawnBatchActive && BatchId == ExpectedBatchId && ActiveBatch != nullptr && ActiveBatch->bTimeSliced
        && CanContinueSpawning();
}

void AKataCharacterSpawner::StopScheduledBatch(uint32 ExpectedBatchId)
{
    if (bSpawnBatchActive && BatchId == ExpectedBatchId && ActiveBatch != nullptr && ActiveBatch->bTimeSliced)
    {
        FinishSpawnBatch(true, false);
    }
}

bool AKataCharacterSpawner::ProcessTimeSlicedStep(uint32 ExpectedBatchId, int32 GlobalRequestLimit, int32 SpawnerRequestLimit)
{
    if (!IsScheduledBatchActive(ExpectedBatchId))
    {
        StopScheduledBatch(ExpectedBatchId);
        return false;
    }
    const TStrongObjectPtr<UKataSpawnBatchState> Batch(ActiveBatch.Get());
    UKataCharacterSpawnSubsystem* Subsystem = Batch->Subsystem.Get();
    if (Subsystem == nullptr)
    {
        FinishSpawnBatch(true, false);
        return true;
    }
    if (Batch->bCandidateReady)
    {
        if (!Batch->bReadyPlacementFailed && (Batch->PendingRequests.Num() >= SpawnerRequestLimit
            || Subsystem->GetBudgetedPendingSpawnCount() >= GlobalRequestLimit))
        {
            return false;
        }
        SubmitNextSpawnRequest(ExpectedBatchId);
        return true;
    }
    if (Batch->Progress.PlacementIndex >= Batch->Context.RequestedCount)
    {
        return false;
    }
    if (!AdvanceSpawnPlacement(*Batch.Get()) && IsScheduledBatchActive(ExpectedBatchId) && ActiveBatch == Batch.Get())
    {
        // 수락 이후의 잘못된 후보는 해당 개체만 실패로 확정한다. 앞서 완료한 개체를 되돌리지 않는다.
        Batch->ReadyTransform = FTransform::Identity;
        Batch->bReadyPlacementFailed = true;
        Batch->bCandidateReady = true;
        ++Batch->Progress.PlacementIndex;
        Batch->Progress.AttemptIndex = 0;
        Batch->Progress.ComponentIndex = 0;
        Batch->Progress.PlacementPhase = EKataSpawnPlacementPhase::SelectCandidate;
    }
    if (!CanContinueSpawning())
    {
        StopScheduledBatch(ExpectedBatchId);
    }
    return true;
}

const FKataSpawnBatchContext* AKataCharacterSpawner::GetSpawnBatchContext() const
{
    const UKataSpawnBatchState* Batch = PreparingBatch != nullptr ? PreparingBatch.Get() : ActiveBatch.Get();
    return Batch != nullptr ? &Batch->Context : nullptr;
}

bool AKataCharacterSpawner::PrepareSpawnBatch(UKataSpawnBatchState& Batch, int32 CountOverride)
{
    FKataSpawnBatchContext& Context = Batch.Context;
    Context.CharacterId = CharacterId;
    Context.SpawnerTransform = GetActorTransform();

    const UDataTable* Table = nullptr;
    const FKataCharacterRow* Row = Context.CharacterId.Find(&Table);
    if (Row == nullptr || !Table->GetRowStruct()->IsChildOf(FKataNPCCharacterRow::StaticStruct()))
    {
        UE_LOG(LogKataFramework, Warning, TEXT("Spawner %s rejected spawn: a valid NPC row is required (%s)."),
            *GetName(), *Context.CharacterId.ToString());
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
                *GetName(), *Context.CharacterId.ToString(), *GetNameSafe(Table), *GetNameSafe(SourceTable));
            return false;
        }
    }
    // 위치 선택 훅이 테이블을 변경해도 배치 전체는 같은 행 사본을 사용한다.
    Context.RowData.InitializeAs(Table->GetRowStruct(), reinterpret_cast<const uint8*>(Row));
    // 덮어쓴 팩션도 행 적용 경로로 BeginPlay 전에 반영되도록 원본 테이블이 아닌 배치의 행 사본에 기록한다.
    if (FactionOverride.IsValid())
    {
        if (FKataCharacterRow* RowCopy = Context.RowData.GetMutablePtr<FKataCharacterRow>())
        {
            RowCopy->Faction = FactionOverride;
        }
    }
    Batch.Subsystem = GetWorld()->GetSubsystem<UKataCharacterSpawnSubsystem>();
    if (!Batch.Subsystem.IsValid())
    {
        return false;
    }

    TArray<TStrongObjectPtr<UKataSpawnerComponent>> SourceComponents;
    for (const TObjectPtr<UKataSpawnerComponent>& Component : SpawnerComponents)
    {
        if (IsValid(Component) && Component->bEnabled)
        {
            SourceComponents.Emplace(Component.Get());
        }
    }
    bool bHasAIOverride = false;
    for (const TStrongObjectPtr<UKataSpawnerComponent>& SourceComponent : SourceComponents)
    {
        UKataSpawnerComponent* Snapshot = DuplicateObject<UKataSpawnerComponent>(SourceComponent.Get(), this);
        if (!IsValid(Snapshot))
        {
            UE_LOG(LogKataFramework, Warning, TEXT("Spawner %s rejected spawn: failed to copy a spawner component."), *GetName());
            return false;
        }
        Snapshot->SetFlags(RF_Transient);
        Batch.Components.Add(Snapshot);
        if (UKataSpawnerComponent_SpawnArea* SpawnArea = Cast<UKataSpawnerComponent_SpawnArea>(Snapshot))
        {
            if (Context.SpawnArea != nullptr)
            {
                UE_LOG(LogKataFramework, Warning, TEXT("Spawner %s rejected spawn: more than one enabled Spawn Area entry."), *GetName());
                return false;
            }
            Context.SpawnArea = SpawnArea;
        }
        if (Snapshot->IsA<UKataSpawnerComponent_AIOverride>())
        {
            if (bHasAIOverride)
            {
                UE_LOG(LogKataFramework, Warning, TEXT("Spawner %s rejected spawn: more than one enabled AI Override entry."), *GetName());
                return false;
            }
            bHasAIOverride = true;
        }
    }
    // 중복 설정 검사를 모두 통과한 뒤 배열 순서로 행 사본을 수정한다. Faction Override는 이미 기록되어 있어 설정이 다시 덮어쓸 수 있다.
    for (const TObjectPtr<UKataSpawnerComponent>& Component : Batch.Components)
    {
        Component->ModifySpawnRow(Context.RowData);
    }

    // 거리 재생성은 처음 정한 수량 중 제거한 수만 채우므로 Spawn Area의 수량을 다시 뽑지 않는다.
    if (CountOverride >= 0)
    {
        Context.RequestedCount = CountOverride;
    }
    else
    {
        Context.RequestedCount = Context.SpawnArea != nullptr ? Context.SpawnArea->CalculateSpawnCount() : 1;
    }
    if (!CanContinueSpawning())
    {
        return false;
    }
    if (Context.RequestedCount < 0)
    {
        UE_LOG(LogKataFramework, Warning, TEXT("Spawner %s rejected spawn: negative spawn count %d."), *GetName(), Context.RequestedCount);
        return false;
    }
    Context.CollisionHandling = Context.SpawnArea != nullptr
        ? Context.SpawnArea->CollisionHandling : ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    for (const TObjectPtr<UKataSpawnerComponent>& Component : Batch.Components)
    {
        Context.MaxPlacementAttempts = FMath::Max(Context.MaxPlacementAttempts, Component->GetPlacementAttempts());
        if (!CanContinueSpawning())
        {
            return false;
        }
    }
    if (!Batch.bTimeSliced)
    {
        Batch.PreparedTransforms.Reserve(Context.RequestedCount);
        Batch.PlacementFailed.Reserve(Context.RequestedCount);
    }
    return true;
}

bool AKataCharacterSpawner::AdvanceSpawnPlacement(UKataSpawnBatchState& Batch)
{
    if (!CanContinueSpawning())
    {
        return false;
    }
    const FKataSpawnBatchContext& Context = Batch.Context;
    FKataSpawnBatchProgress& Progress = Batch.Progress;
    switch (Progress.PlacementPhase)
    {
    case EKataSpawnPlacementPhase::SelectCandidate:
    {
        Progress.CandidateTransform = Context.SpawnerTransform;
        const bool bCandidateValid = Context.SpawnArea == nullptr
            || Context.SpawnArea->GetSpawnTransform(Context.SpawnerTransform, Progress.PlacementIndex, Progress.CandidateTransform);
        if (!CanContinueSpawning() || (Batch.bTimeSliced && ActiveBatch != &Batch))
        {
            return false;
        }
        if (!bCandidateValid || !Progress.CandidateTransform.IsValid())
        {
            UE_LOG(LogKataFramework, Warning, TEXT("Spawner %s could not select a valid transform for index %d."), *GetName(), Progress.PlacementIndex);
            return false;
        }
        Progress.bPlacementAccepted = true;
        Progress.ComponentIndex = 0;
        Progress.PlacementPhase = EKataSpawnPlacementPhase::AdjustCandidate;
        return true;
    }
    case EKataSpawnPlacementPhase::AdjustCandidate:
    {
        if (Progress.ComponentIndex >= Batch.Components.Num())
        {
            Progress.PlacementPhase = EKataSpawnPlacementPhase::CompleteCandidate;
            return true;
        }
        FTransform Adjusted = Progress.CandidateTransform;
        const bool bAccepted = Batch.Components[Progress.ComponentIndex]->AdjustSpawnTransform(this, Context.SpawnArea.Get(),
            Progress.PlacementIndex, Progress.CandidateTransform, Adjusted);
        if (!CanContinueSpawning() || (Batch.bTimeSliced && ActiveBatch != &Batch))
        {
            return false;
        }
        if (!bAccepted || !Adjusted.IsValid())
        {
            Progress.bPlacementAccepted = false;
            ++Progress.AttemptIndex;
            Progress.PlacementPhase = Progress.AttemptIndex < Context.MaxPlacementAttempts
                ? EKataSpawnPlacementPhase::SelectCandidate : EKataSpawnPlacementPhase::CompleteCandidate;
        }
        else
        {
            Progress.CandidateTransform = Adjusted;
            ++Progress.ComponentIndex;
        }
        return true;
    }
    case EKataSpawnPlacementPhase::CompleteCandidate:
        if (!Progress.bPlacementAccepted)
        {
            UE_LOG(LogKataFramework, Warning, TEXT("Spawner %s could not place index %d after %d attempt(s). It will be reported as failed."),
                *GetName(), Progress.PlacementIndex, Context.MaxPlacementAttempts);
        }
        if (Batch.bTimeSliced)
        {
            Batch.ReadyTransform = Progress.CandidateTransform;
            Batch.bReadyPlacementFailed = !Progress.bPlacementAccepted;
            Batch.bCandidateReady = true;
        }
        else
        {
            Batch.PreparedTransforms.Add(Progress.CandidateTransform);
            Batch.PlacementFailed.Add(!Progress.bPlacementAccepted);
        }
        ++Progress.PlacementIndex;
        Progress.AttemptIndex = 0;
        Progress.ComponentIndex = 0;
        Progress.PlacementPhase = EKataSpawnPlacementPhase::SelectCandidate;
        return true;
    }
    return false;
}

void AKataCharacterSpawner::SubmitNextSpawnRequest(uint32 ExpectedBatchId)
{
    if (!bSpawnBatchActive || BatchId != ExpectedBatchId || !CanContinueSpawning() || ActiveBatch == nullptr)
    {
        if (bSpawnBatchActive && BatchId == ExpectedBatchId)
        {
            FinishSpawnBatch(true, false);
        }
        return;
    }
    const TStrongObjectPtr<UKataSpawnBatchState> Batch(ActiveBatch.Get());
    if (Batch->Progress.SubmissionIndex >= Batch->Context.RequestedCount)
    {
        return;
    }
    // 즉시 실패 콜백 전에 이번 개체만 예약한다. 전체 미완료 수는 핸들 맵과 독립적으로 유지한다.
    const int32 SpawnIndex = Batch->Progress.SubmissionIndex++;
    const FTransform SpawnTransform = Batch->bTimeSliced ? Batch->ReadyTransform : Batch->PreparedTransforms[SpawnIndex];
    const bool bPlacementFailed = Batch->bTimeSliced ? Batch->bReadyPlacementFailed : Batch->PlacementFailed[SpawnIndex];
    Batch->bCandidateReady = false;
    Batch->PendingRequests.Add(SpawnIndex, FKataCharacterSpawnHandle());
    UKataCharacterSpawnSubsystem* Subsystem = Batch->Subsystem.Get();
    if (bPlacementFailed || Subsystem == nullptr)
    {
        HandleSpawnCompleted(nullptr, ExpectedBatchId, SpawnIndex);
        return;
    }
    FKataCharacterSpawnDelegate OnComplete = FKataCharacterSpawnDelegate::CreateUObject(this,
        &AKataCharacterSpawner::HandleSpawnCompleted, ExpectedBatchId, SpawnIndex);
    const TSharedPtr<FKataCharacterSpawnOwnership> Ownership = MakeShared<FKataCharacterSpawnOwnership>();
    Ownership->Generation = Batch->Generation;
    Batch->PendingOwnership.Add(SpawnIndex, Ownership);
    const FKataCharacterSpawnHandle Handle = Batch->bTimeSliced
        ? Subsystem->RequestSpawnFromRowBudgeted(Batch->SpawnGroup, Batch->Context.CharacterId, Batch->Context.RowData,
            SpawnTransform, MoveTemp(OnComplete), Batch->Context.CollisionHandling, Ownership)
        : Subsystem->RequestSpawnFromRow(Batch->Context.CharacterId, Batch->Context.RowData,
            SpawnTransform, MoveTemp(OnComplete), Batch->Context.CollisionHandling, Ownership);

    if (bSpawnBatchActive && BatchId == ExpectedBatchId && ActiveBatch == Batch.Get())
    {
        if (FKataCharacterSpawnHandle* StoredHandle = Batch->PendingRequests.Find(SpawnIndex))
        {
            *StoredHandle = Handle;
            return;
        }
    }
    // 결과 이벤트에서 취소되어도 방금 반환된 핸들이 남지 않게 한다.
    if (IsValid(Subsystem))
    {
        Subsystem->CancelSpawn(Handle);
    }
}
void AKataCharacterSpawner::CancelSpawning()
{
    if (Activation == EKataSpawnerActivation::Disabled)
    {
        UE_LOG(LogKataFramework, Warning, TEXT("Spawner %s ignored cancel: the spawner is disabled."), *GetName());
        return;
    }
    if (bDistanceManaged)
    {
        UE_LOG(LogKataFramework, Warning, TEXT("Spawner %s ignored cancel: spawning is managed by Player Distance activation."), *GetName());
        return;
    }
    if (bSpawnBatchActive)
    {
        FinishSpawnBatch(true, true);
    }
}

bool AKataCharacterSpawner::DespawnCharacters()
{
    if (Activation == EKataSpawnerActivation::Disabled)
    {
        UE_LOG(LogKataFramework, Warning, TEXT("Spawner %s rejected despawn: the spawner is disabled."), *GetName());
        return false;
    }
    if (bDistanceManaged)
    {
        UE_LOG(LogKataFramework, Warning, TEXT("Spawner %s rejected despawn: despawning is managed by Player Distance activation."), *GetName());
        return false;
    }
    if (bStartingBatch || IsDespawning() || !CanContinueSpawning())
    {
        UE_LOG(LogKataFramework, Warning, TEXT("Spawner %s rejected despawn: preparing, despawning, or game world unavailable."), *GetName());
        return false;
    }
    UKataSpawnerSubsystem* Scheduler = GetWorld()->GetSubsystem<UKataSpawnerSubsystem>();
    if (Scheduler == nullptr || !Scheduler->CanScheduleWork())
    {
        return false;
    }
    // 취소 결과 이벤트의 재호출 전에 제거 상태를 고정해 새 생성이 제거 대상에 섞이지 않게 한다.
    const bool bHadSpawnBatch = bSpawnBatchActive;
    const int32 SpawnSuccessCount = ActiveBatch != nullptr ? ActiveBatch->SucceededCount : 0;
    const int32 SpawnFailureCount = ActiveBatch != nullptr ? ActiveBatch->FailedCount : 0;
    ActiveDespawnBatch = MakeShared<FKataDespawnBatchState>();
    ActiveDespawnBatch->Spawner = this;
    if (ActiveBatch != nullptr)
    {
        ActiveBatch->Generation->bDespawnRequested = true;
        FinishSpawnBatch(true, false);
    }
    ActiveDespawnBatch->Records = MoveTemp(SpawnOwnershipRecords);
    Scheduler->RegisterDespawnBatch(ActiveDespawnBatch.ToSharedRef());
    if (bHadSpawnBatch && !bEndingPlay)
    {
        OnBatchFinished.Broadcast(SpawnSuccessCount, SpawnFailureCount, true);
    }
    return true;
}

int32 AKataCharacterSpawner::GetPendingDespawnCount() const
{
    return ActiveDespawnBatch.IsValid() ? ActiveDespawnBatch->Records.Num() - ActiveDespawnBatch->RecordIndex : 0;
}

void AKataCharacterSpawner::FinishDespawnBatch(const TSharedPtr<FKataDespawnBatchState>& Batch, bool bBroadcast)
{
    if (Batch.IsValid() && Batch->bDistanceDespawn)
    {
        PendingDistanceDespawnBatches = FMath::Max(0, PendingDistanceDespawnBatches - 1);
        if (bEndingPlay)
        {
            return;
        }
        // Destroy를 거절한 NPC는 살아 있으므로 다음 평가에서 다시 판정하고 재생성 수에서 뺀다.
        // 이미 시작한 재생성 배치가 이 수를 소비했으면 0에서 멈추며, 그 차이만큼 개체가 더 생길 수 있다.
        SpawnOwnershipRecords.Append(MoveTemp(Batch->FailedRecords));
        PendingRespawnCount = FMath::Max(0, PendingRespawnCount - Batch->RefusedCharacterCount);
        return;
    }
    if (ActiveDespawnBatch != Batch)
    {
        return;
    }
    // 실패 기록은 재시도할 수 있도록 반환하고 완료 콜백 전에 상태를 해제한다.
    SpawnOwnershipRecords.Append(MoveTemp(Batch->FailedRecords));
    ActiveDespawnBatch.Reset();
    if (bBroadcast && !bEndingPlay)
    {
        OnDespawnFinished.Broadcast(Batch->RemovedCharacterCount, Batch->FailedActorCount);
    }
}

void AKataCharacterSpawner::EvaluateDistance(const FVector& PlayerLocation)
{
    if (!bDistanceManaged || !CanContinueSpawning())
    {
        return;
    }
    const double SpawnRangeSquared = FMath::Square(static_cast<double>(DistanceSpawnRange));
    const double DespawnRangeSquared = FMath::Square(static_cast<double>(DistanceDespawnRange));

    // 원점은 두 거리 사이에서 직전 상태를 유지해 경계 근처의 반복 진입·이탈을 막는다.
    const double OriginDistanceSquared = FVector::DistSquared(GetActorLocation(), PlayerLocation);
    if (!bOriginInRange && OriginDistanceSquared <= SpawnRangeSquared)
    {
        bOriginInRange = true;
        bEntrySpawnPending = true;
    }
    else if (bOriginInRange && OriginDistanceSquared > DespawnRangeSquared)
    {
        bOriginInRange = false;
        bEntrySpawnPending = false;
        CancelDistanceSpawnBatch();
        // 취소 완료 이벤트에서 스포너를 제거했을 수 있다.
        if (!bDistanceManaged || !CanContinueSpawning())
        {
            return;
        }
    }

    // NPC는 스포너 원점이 아니라 자기 현재 위치로 판정한다. 원점에서 멀리 이동해도 플레이어 근처에 있으면 유지한다.
    TSharedPtr<FKataDespawnBatchState> DistanceBatch;
    for (int32 Index = SpawnOwnershipRecords.Num() - 1; Index >= 0; --Index)
    {
        const TSharedPtr<FKataCharacterSpawnOwnership> Record = SpawnOwnershipRecords[Index];
        const AKataCharacter* Character = Record.IsValid() ? Record->Character.Get() : nullptr;
        if (Character == nullptr || Character->IsActorBeingDestroyed())
        {
            // 외부 제거·사망으로 사라진 NPC는 재생성하지 않는다. 정리할 Controller가 남았으면 기록을 유지한다.
            const bool bHasController = Record.IsValid() && Record->OwnedControllers.ContainsByPredicate(
                [](const TWeakObjectPtr<AController>& Controller) { return Controller.IsValid(); });
            if (!bHasController)
            {
                SpawnOwnershipRecords.RemoveAtSwap(Index, 1, EAllowShrinking::No);
            }
            continue;
        }
        if (FVector::DistSquared(Character->GetActorLocation(), PlayerLocation) <= DespawnRangeSquared)
        {
            continue;
        }
        if (!DistanceBatch.IsValid())
        {
            DistanceBatch = MakeShared<FKataDespawnBatchState>();
            DistanceBatch->Spawner = this;
            DistanceBatch->bDistanceDespawn = true;
        }
        DistanceBatch->Records.Add(Record);
        SpawnOwnershipRecords.RemoveAtSwap(Index, 1, EAllowShrinking::No);
    }
    if (DistanceBatch.IsValid())
    {
        UKataSpawnerSubsystem* Scheduler = GetWorld()->GetSubsystem<UKataSpawnerSubsystem>();
        if (Scheduler != nullptr && Scheduler->CanScheduleWork())
        {
            PendingRespawnCount += DistanceBatch->Records.Num();
            ++PendingDistanceDespawnBatches;
            Scheduler->RegisterDespawnBatch(DistanceBatch.ToSharedRef());
        }
        else
        {
            SpawnOwnershipRecords.Append(MoveTemp(DistanceBatch->Records));
        }
    }

    // 범위 안에 머무는 것만으로 반복 생성하지 않도록 진입 한 번에 한 번만 시도한다. 실패하면 다음 진입에서 다시 시도한다.
    if (bEntrySpawnPending && !bSpawnBatchActive && !bStartingBatch)
    {
        bEntrySpawnPending = false;
        if (bInitialSpawnPending)
        {
            bInitialSpawnPending = !StartSpawnBatch(INDEX_NONE, true);
        }
        else if (PendingRespawnCount > 0)
        {
            const int32 RespawnCount = PendingRespawnCount;
            PendingRespawnCount = 0;
            if (!StartSpawnBatch(RespawnCount, true))
            {
                PendingRespawnCount += RespawnCount;
            }
        }
    }
}

bool AKataCharacterSpawner::HasActiveDistanceState() const
{
    // 거리 제거 중인 배치가 Destroy 거절 기록을 돌려줄 수 있으므로 완료 전까지 활성으로 유지한다.
    return bOriginInRange || bSpawnBatchActive || !SpawnOwnershipRecords.IsEmpty() || PendingDistanceDespawnBatches > 0;
}

void AKataCharacterSpawner::CancelDistanceSpawnBatch()
{
    if (!bSpawnBatchActive || ActiveBatch == nullptr)
    {
        return;
    }
    // 생성 도중인 NPC는 FinishSpawnBatch가 생존 기록으로 옮기므로 재생성 수에서 제외한다.
    int32 UnfinishedCount = ActiveBatch->RemainingCount;
    for (const TPair<int32, TSharedPtr<FKataCharacterSpawnOwnership>>& Pair : ActiveBatch->PendingOwnership)
    {
        if (Pair.Value.IsValid() && Pair.Value->Character.IsValid())
        {
            --UnfinishedCount;
        }
    }
    PendingRespawnCount += FMath::Max(0, UnfinishedCount);
    bInitialSpawnPending = false;
    FinishSpawnBatch(true, true);
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
    if (bDistanceManaged)
    {
        bDistanceManaged = false;
        UWorld* World = GetWorld();
        if (UKataSpawnerSubsystem* Scheduler = World != nullptr ? World->GetSubsystem<UKataSpawnerSubsystem>() : nullptr)
        {
            Scheduler->UnregisterDistanceSpawner(this);
        }
    }
    const bool bQueueOwnedDespawn = bDespawnOnEndPlay && !IsDespawning();
    if (bSpawnBatchActive)
    {
        if (bQueueOwnedDespawn)
        {
            ActiveBatch->Generation->bDespawnRequested = true;
        }
        FinishSpawnBatch(true, false);
    }
    if (bQueueOwnedDespawn)
    {
        UWorld* World = GetWorld();
        UKataSpawnerSubsystem* Scheduler = World != nullptr ? World->GetSubsystem<UKataSpawnerSubsystem>() : nullptr;
        if (Scheduler != nullptr && Scheduler->CanScheduleWork())
        {
            const TSharedRef<FKataDespawnBatchState> Cleanup = MakeShared<FKataDespawnBatchState>();
            Cleanup->Records = MoveTemp(SpawnOwnershipRecords);
            Scheduler->RegisterDespawnBatch(Cleanup);
        }
    }
    // 이미 관리자에게 제출한 제거 작업은 약한 소유자 참조와 무관하게 계속한다.
    ActiveDespawnBatch.Reset();
    SpawnOwnershipRecords.Reset();
    SpawnedCharacters.Reset();
    Super::EndPlay(EndPlayReason);
}

void AKataCharacterSpawner::HandleSpawnCompleted(AKataCharacter* Character, uint32 RequestBatchId, int32 SpawnIndex)
{
    if (!bSpawnBatchActive || BatchId != RequestBatchId || bEndingPlay || ActiveBatch == nullptr)
    {
        return;
    }

    const TStrongObjectPtr<UKataSpawnBatchState> Batch(ActiveBatch.Get());
    if (Batch->PendingRequests.Remove(SpawnIndex) == 0)
    {
        return;
    }
    --Batch->RemainingCount;
    TSharedPtr<FKataCharacterSpawnOwnership> Ownership;
    Batch->PendingOwnership.RemoveAndCopyValue(SpawnIndex, Ownership);

    if (IsValid(Character))
    {
        ++Batch->SucceededCount;
        SpawnedCharacters.Add(Character);
        if (Ownership.IsValid())
        {
            SpawnOwnershipRecords.Add(Ownership);
        }
        const FKataCharacterId CompletedCharacterId = Batch->Context.CharacterId;
        TArray<TWeakObjectPtr<UKataSpawnerComponent>> ComponentsForResult;
        for (const TObjectPtr<UKataSpawnerComponent>& Component : Batch->Components)
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
        if (Ownership.IsValid() && !Ownership->OwnedControllers.IsEmpty())
        {
            SpawnOwnershipRecords.Add(Ownership);
        }
        ++Batch->FailedCount;
        OnCharacterSpawnFailed.Broadcast(SpawnIndex);
    }

    // 결과 이벤트에서 작업을 취소하거나 스포너를 제거했을 수 있다.
    if (bSpawnBatchActive && BatchId == RequestBatchId && !bEndingPlay && Batch->RemainingCount == 0)
    {
        FinishSpawnBatch(false, true);
    }
}

void AKataCharacterSpawner::FinishSpawnBatch(bool bCancelled, bool bBroadcast)
{
    if (ActiveBatch == nullptr)
    {
        return;
    }
    const TStrongObjectPtr<UKataSpawnBatchState> Batch(ActiveBatch.Get());
    const int32 CompletedSuccessCount = Batch->SucceededCount;
    const int32 CompletedFailureCount = Batch->FailedCount;
    TMap<int32, FKataCharacterSpawnHandle> RequestsToCancel = MoveTemp(Batch->PendingRequests);
    for (const TPair<int32, TSharedPtr<FKataCharacterSpawnOwnership>>& Pair : Batch->PendingOwnership)
    {
        // BeginPlay 안의 취소는 서비스가 실행 중인 NPC까지 추적해야 한다. 아직 생성하지 않은 요청은 기록할 자원이 없다.
        if (Pair.Value->Character.IsValid() || !Pair.Value->OwnedControllers.IsEmpty())
        {
            SpawnOwnershipRecords.Add(Pair.Value);
        }
    }
    Batch->PendingOwnership.Reset();
    Batch->PendingRequests.Reset();
    Batch->RemainingCount = 0;
    const TWeakObjectPtr<UKataCharacterSpawnSubsystem> Subsystem = Batch->Subsystem;
    bSpawnBatchActive = false;
    ActiveBatch = nullptr;

    if (Batch->Scheduler.IsValid())
    {
        Batch->Scheduler->UnregisterSpawnBatch(Batch->RegistrationId);
    }

    if (Subsystem.IsValid())
    {
        if (Batch->SpawnGroup.IsValid())
        {
            Subsystem->CancelSpawnGroup(Batch->SpawnGroup);
        }
        else
        {
            for (const TPair<int32, FKataCharacterSpawnHandle>& Request : RequestsToCancel)
            {
                Subsystem->CancelSpawn(Request.Value);
            }
        }
    }
    if (bBroadcast && !bEndingPlay)
    {
        OnBatchFinished.Broadcast(CompletedSuccessCount, CompletedFailureCount, bCancelled);
    }
}
