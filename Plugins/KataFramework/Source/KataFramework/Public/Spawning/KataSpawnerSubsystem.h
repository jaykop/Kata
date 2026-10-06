#pragma once

#include "CoreMinimal.h"
#include "Character/KataCharacterSpawnSubsystem.h"
#include "Containers/Queue.h"
#include "Subsystems/WorldSubsystem.h"
#include "KataSpawnerSubsystem.generated.h"

class AKataCharacterSpawner;
struct FKataDespawnBatchState;

/**
 * 스포너의 생성·제거 작업을 순환 처리하는 월드 관리자. 액터별 Tick 없이 위치 준비·로드 제출·생성·제거에 공용 예산을 적용한다.
 * 생성 등록은 활성 배치 수명을 따르고 제거 작업은 소유자 종료 후에도 이어간다. 일반 단일 생성 API의 비용은 포함하지 않는다.
 */
UCLASS()
class KATAFRAMEWORK_API UKataSpawnerSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual void OnWorldEndPlay(UWorld& InWorld) override;
    virtual void Tick(float DeltaTime) override;
    virtual bool IsTickable() const override;
    virtual TStatId GetStatId() const override;

    /** 스포너의 유효한 분산 배치를 등록한다. 반환 ID는 완료·취소 시 해제에 쓰며 0은 거절이다. */
    uint32 RegisterSpawnBatch(AKataCharacterSpawner* Spawner, uint32 BatchId, FKataCharacterSpawnGroupHandle Group);

    /** 등록을 해제한다. 배치와 생성 요청의 취소는 소유 스포너가 수행한다. */
    void UnregisterSpawnBatch(uint32 RegistrationId);

    /** 월드가 유지되고 종료 처리를 시작하지 않았으면 새 작업을 받을 수 있다. */
    bool CanScheduleWork() const;

    /** 소유자가 종료돼도 이어갈 제거 작업을 등록한다. 호출 전에 CanScheduleWork를 확인해야 한다. */
    void RegisterDespawnBatch(const TSharedRef<FKataDespawnBatchState>& Batch);

protected:
    virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;

private:
    struct FManagedBatch
    {
        TWeakObjectPtr<AKataCharacterSpawner> Spawner;
        FKataCharacterSpawnGroupHandle Group;
        uint32 BatchId = 0;
        uint32 RegistrationId = 0;
    };

    void StopAllBatches();
    bool ProcessDespawnStep(FKataDespawnBatchState& Batch);
    void ProcessDespawnBatches(int32 MaxSteps, double Deadline);

    TArray<FManagedBatch> Batches;
    TMap<uint32, int32> BatchIndices;
    TWeakObjectPtr<UKataCharacterSpawnSubsystem> SpawnSubsystem;
    TQueue<TSharedPtr<FKataDespawnBatchState>> DespawnBatches;
    int32 DespawnBatchCount = 0;
    int32 NextBatchIndex = 0;
    int32 NextReadyIndex = 0;
    uint32 LastRegistrationId = 0;
    bool bStopping = false;
    bool bTicking = false;
    bool bDespawnFirst = true;
};
