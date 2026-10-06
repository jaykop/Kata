#include "Spawning/KataSpawnerSubsystem.h"

#include "Engine/World.h"
#include "HAL/PlatformTime.h"
#include "Spawning/KataCharacterSpawner.h"
#include "Character/KataCharacter.h"
#include "Spawning/KataDespawnBatchTypes.h"
#include "Spawning/KataFL_Spawning.h"
#include "KataFrameworkLog.h"
#include "Spawning/KataSpawnerSettings.h"
#include "Stats/Stats.h"
#include "Subsystems/SubsystemCollection.h"

void UKataSpawnerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Collection.InitializeDependency<UKataCharacterSpawnSubsystem>();
    Super::Initialize(Collection);
    SpawnSubsystem = GetWorld()->GetSubsystem<UKataCharacterSpawnSubsystem>();
    bStopping = false;
}

void UKataSpawnerSubsystem::OnWorldEndPlay(UWorld& InWorld)
{
    StopAllBatches();
    Super::OnWorldEndPlay(InWorld);
}

void UKataSpawnerSubsystem::Deinitialize()
{
    StopAllBatches();
    SpawnSubsystem.Reset();
    Super::Deinitialize();
}

bool UKataSpawnerSubsystem::DoesSupportWorldType(EWorldType::Type WorldType) const
{
    return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

bool UKataSpawnerSubsystem::IsTickable() const
{
    const UWorld* World = GetWorld();
    return IsInitialized() && !IsTemplate() && !bStopping && (!Batches.IsEmpty() || DespawnBatchCount > 0)
        && World != nullptr && !World->bIsTearingDown;
}

bool UKataSpawnerSubsystem::CanScheduleWork() const
{
    const UWorld* World = GetWorld();
    return IsInitialized() && !bStopping && World != nullptr && World->IsGameWorld() && !World->bIsTearingDown;
}

void UKataSpawnerSubsystem::RegisterDespawnBatch(const TSharedRef<FKataDespawnBatchState>& Batch)
{
    if (CanScheduleWork())
    {
        DespawnBatches.Enqueue(Batch);
        ++DespawnBatchCount;
    }
}

bool UKataSpawnerSubsystem::ProcessDespawnStep(FKataDespawnBatchState& Batch)
{
    if (Batch.RecordIndex >= Batch.Records.Num())
    {
        return true;
    }
    const TSharedPtr<FKataCharacterSpawnOwnership> Ownership = Batch.Records[Batch.RecordIndex];
    if (!Batch.bCharacterProcessed)
    {
        AKataCharacter* Character = Ownership->Character.Get();
        const bool bHadCharacter = Character != nullptr && !Character->IsActorBeingDestroyed();
        Batch.bCharacterProcessed = true;
        if (!KataFL::DestroySpawnedCharacter(*Ownership))
        {
            ++Batch.FailedActorCount;
            Batch.bRecordFailed = true;
            // NPC가 제거를 거절하면 실행 중 Controller도 유지하고 이 기록을 재시도 대상으로 반환한다.
            Batch.ControllerIndex = Ownership->OwnedControllers.Num();
            UE_LOG(LogKataFramework, Warning, TEXT("Despawn failed: character refused Destroy (generation %u)."),
                Ownership->Generation.IsValid() ? Ownership->Generation->Id : 0);
        }
        else if (bHadCharacter)
        {
            ++Batch.RemovedCharacterCount;
        }
    }
    else if (Batch.ControllerIndex < Ownership->OwnedControllers.Num())
    {
        if (!KataFL::DestroySpawnOwnedController(*Ownership, Batch.ControllerIndex++))
        {
            ++Batch.FailedActorCount;
            Batch.bRecordFailed = true;
            UE_LOG(LogKataFramework, Warning, TEXT("Despawn failed: owned controller refused Destroy (generation %u)."),
                Ownership->Generation.IsValid() ? Ownership->Generation->Id : 0);
        }
    }
    if (Batch.ControllerIndex >= Ownership->OwnedControllers.Num())
    {
        if (Batch.bRecordFailed)
        {
            Batch.FailedRecords.Add(Ownership);
        }
        // 메모리 해제도 개체 처리에 나눠 포함해 전체 완료 순간에 많은 기록을 한꺼번에 버리지 않는다.
        Batch.Records[Batch.RecordIndex++].Reset();
        Batch.bCharacterProcessed = false;
        Batch.bRecordFailed = false;
        Batch.ControllerIndex = 0;
    }
    return Batch.RecordIndex >= Batch.Records.Num();
}

TStatId UKataSpawnerSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UKataSpawnerSubsystem, STATGROUP_Tickables);
}

uint32 UKataSpawnerSubsystem::RegisterSpawnBatch(AKataCharacterSpawner* Spawner, uint32 BatchId,
    FKataCharacterSpawnGroupHandle Group)
{
    if (bStopping || !IsValid(Spawner) || Spawner->GetWorld() != GetWorld() || !Group.IsValid()
        || !Spawner->IsScheduledBatchActive(BatchId))
    {
        return 0;
    }
    ++LastRegistrationId;
    if (LastRegistrationId == 0)
    {
        ++LastRegistrationId;
    }
    FManagedBatch Batch;
    Batch.Spawner = Spawner;
    Batch.Group = Group;
    Batch.BatchId = BatchId;
    Batch.RegistrationId = LastRegistrationId;
    BatchIndices.Add(Batch.RegistrationId, Batches.Add(Batch));
    return Batch.RegistrationId;
}

void UKataSpawnerSubsystem::UnregisterSpawnBatch(uint32 RegistrationId)
{
    int32 Index = INDEX_NONE;
    if (!BatchIndices.RemoveAndCopyValue(RegistrationId, Index))
    {
        return;
    }
    const int32 LastIndex = Batches.Num() - 1;
    Batches.RemoveAtSwap(Index, 1, EAllowShrinking::No);
    if (Index != LastIndex)
    {
        BatchIndices.FindChecked(Batches[Index].RegistrationId) = Index;
    }
    // 차례가 옮겨진 항목도 순환에 포함하고 끝에서 삭제했으면 첫 항목부터 이어간다.
    if (NextBatchIndex == LastIndex && Index != LastIndex)
    {
        NextBatchIndex = Index;
    }
    if (NextBatchIndex >= Batches.Num())
    {
        NextBatchIndex = 0;
    }
    if (NextReadyIndex == LastIndex && Index != LastIndex)
    {
        NextReadyIndex = Index;
    }
    if (NextReadyIndex >= Batches.Num())
    {
        NextReadyIndex = 0;
    }
}

void UKataSpawnerSubsystem::StopAllBatches()
{
    bStopping = true;
    TArray<FManagedBatch> Removed = MoveTemp(Batches);
    Batches.Reset();
    BatchIndices.Reset();
    NextBatchIndex = 0;
    NextReadyIndex = 0;
    for (const FManagedBatch& Batch : Removed)
    {
        if (AKataCharacterSpawner* Spawner = Batch.Spawner.Get())
        {
            Spawner->StopScheduledBatch(Batch.BatchId);
        }
        if (SpawnSubsystem.IsValid())
        {
            SpawnSubsystem->CancelSpawnGroup(Batch.Group);
        }
    }
    TSharedPtr<FKataDespawnBatchState> DespawnBatch;
    while (DespawnBatches.Dequeue(DespawnBatch))
    {
        if (AKataCharacterSpawner* Spawner = DespawnBatch->Spawner.Get())
        {
            Spawner->FinishDespawnBatch(DespawnBatch, false);
        }
    }
    DespawnBatchCount = 0;
}

void UKataSpawnerSubsystem::ProcessDespawnBatches(int32 MaxSteps, double Deadline)
{
    for (int32 Step = 0; Step < MaxSteps && IsTickable() && FPlatformTime::Seconds() < Deadline; ++Step)
    {
        TSharedPtr<FKataDespawnBatchState> DespawnBatch;
        if (!DespawnBatches.Dequeue(DespawnBatch))
        {
            break;
        }
        --DespawnBatchCount;
        const bool bComplete = ProcessDespawnStep(*DespawnBatch);
        if (bComplete)
        {
            if (AKataCharacterSpawner* Spawner = DespawnBatch->Spawner.Get())
            {
                Spawner->FinishDespawnBatch(DespawnBatch, CanScheduleWork());
            }
        }
        else if (CanScheduleWork())
        {
            DespawnBatches.Enqueue(DespawnBatch);
            ++DespawnBatchCount;
        }
    }
}

void UKataSpawnerSubsystem::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    if (bTicking || !IsTickable() || !SpawnSubsystem.IsValid())
    {
        return;
    }
    TGuardValue<bool> TickGuard(bTicking, true);
    const UKataSpawnerSettings* Settings = GetDefault<UKataSpawnerSettings>();
    const double BudgetMs = FMath::IsFinite(Settings->TimeBudgetMs) ? FMath::Max(0.01, Settings->TimeBudgetMs) : 1.0;
    const double Deadline = FPlatformTime::Seconds() + BudgetMs * 0.001;
    const int32 MaxSteps = FMath::Max(1, Settings->MaxPreparationStepsPerFrame);
    const int32 MaxSpawns = FMath::Max(1, Settings->MaxSpawnAttemptsPerFrame);
    const int32 MaxDespawnSteps = FMath::Max(1, Settings->MaxDespawnStepsPerFrame);
    const int32 GlobalLimit = FMath::Max(1, Settings->MaxOutstandingRequests);
    const int32 SpawnerLimit = FMath::Max(1, Settings->MaxOutstandingRequestsPerSpawner);
    int32 SpawnAttempts = 0;
    int32 IdleVisits = 0;
    int32 Steps = 0;

    // 긴 단일 작업이 예산을 넘겨도 한 종류가 계속 밀리지 않도록 첫 처리 차례를 프레임마다 바꾼다.
    const bool bRunDespawnFirst = bDespawnFirst;
    bDespawnFirst = !bDespawnFirst;
    if (bRunDespawnFirst)
    {
        ProcessDespawnBatches(MaxDespawnSteps, Deadline);
    }

    // 생성 차례와 준비 차례를 따로 보존해 준비량이 많은 배치가 다음 프레임의 생성 차례를 독점하지 않게 한다.
    while (Steps < MaxSteps && SpawnAttempts < MaxSpawns && IsTickable() && !Batches.IsEmpty()
        && SpawnSubsystem->GetBudgetedPendingSpawnCount() > 0 && FPlatformTime::Seconds() < Deadline)
    {
        ++Steps;
        NextReadyIndex %= Batches.Num();
        // 생성·결과 콜백에서 배열을 바꿀 수 있으므로 방문할 항목을 값으로 복사한다.
        const FManagedBatch Batch = Batches[NextReadyIndex];
        NextReadyIndex = (NextReadyIndex + 1) % Batches.Num();
        AKataCharacterSpawner* Spawner = Batch.Spawner.Get();
        bool bDidWork = false;
        if (Spawner == nullptr || !Spawner->IsScheduledBatchActive(Batch.BatchId))
        {
            if (Spawner != nullptr)
            {
                Spawner->StopScheduledBatch(Batch.BatchId);
            }
            UnregisterSpawnBatch(Batch.RegistrationId);
            SpawnSubsystem->CancelSpawnGroup(Batch.Group);
            bDidWork = true;
        }
        else
        {
            const EKataReadySpawnResult Result = SpawnSubsystem->ProcessNextReadySpawn(Batch.Group);
            bDidWork = Result != EKataReadySpawnResult::NoWork;
            SpawnAttempts += Result == EKataReadySpawnResult::Processed ? 1 : 0;
        }
        IdleVisits = bDidWork ? 0 : IdleVisits + 1;
        if (IdleVisits >= Batches.Num())
        {
            break;
        }
    }

    IdleVisits = 0;
    Steps = 0;
    while (Steps < MaxSteps && IsTickable() && !Batches.IsEmpty() && FPlatformTime::Seconds() < Deadline)
    {
        ++Steps;
        NextBatchIndex %= Batches.Num();
        const FManagedBatch Batch = Batches[NextBatchIndex];
        NextBatchIndex = (NextBatchIndex + 1) % Batches.Num();
        AKataCharacterSpawner* Spawner = Batch.Spawner.Get();
        bool bDidWork = false;
        if (Spawner == nullptr || !Spawner->IsScheduledBatchActive(Batch.BatchId))
        {
            if (Spawner != nullptr)
            {
                Spawner->StopScheduledBatch(Batch.BatchId);
            }
            UnregisterSpawnBatch(Batch.RegistrationId);
            SpawnSubsystem->CancelSpawnGroup(Batch.Group);
            bDidWork = true;
        }
        else
        {
            bDidWork = Spawner->ProcessTimeSlicedStep(Batch.BatchId, GlobalLimit, SpawnerLimit);
        }
        IdleVisits = bDidWork ? 0 : IdleVisits + 1;
        if (IdleVisits >= Batches.Num())
        {
            break;
        }
    }
    if (!bRunDespawnFirst)
    {
        ProcessDespawnBatches(MaxDespawnSteps, Deadline);
    }
}
