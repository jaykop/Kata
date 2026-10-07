#pragma once

#include "CoreMinimal.h"
#include "Character/KataCharacterSpawnSubsystem.h"
#include "Containers/Queue.h"
#include "Subsystems/WorldSubsystem.h"
#include "UObject/ObjectKey.h"
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

    /**
     * 거리 관리 스포너를 현재 위치의 그리드 셀에 등록한다. 같은 스포너의 중복 등록은 무시한다.
     * 셀은 등록 시점 위치로 정하며 이후 스포너 이동은 반영하지 않는다.
     * SpawnDistance는 플레이어 주변에서 조회할 셀 반경을 정하는 데 쓴다.
     */
    void RegisterDistanceSpawner(AKataCharacterSpawner* Spawner, float SpawnDistance);

    /** 그리드와 활성 목록에서 제외한다. 이미 만든 평가 목록의 약한 참조는 스포너가 관리 해제 상태라 평가를 건너뛴다. */
    void UnregisterDistanceSpawner(AKataCharacterSpawner* Spawner);

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
    void ProcessDistanceChecks(float DeltaTime, double Deadline);
    void BuildDistancePass(const FVector& PlayerLocation);
    FIntPoint GetDistanceCell(const FVector& Location) const;

    TArray<FManagedBatch> Batches;

    /** 월드 XY를 DistanceCellSize 정사각형으로 나눈 셀별 거리 관리 스포너. 높이는 무시하고 실제 판정은 스포너가 3D 거리로 한다. */
    TMap<FIntPoint, TArray<TWeakObjectPtr<AKataCharacterSpawner>>> DistanceCells;
    TMap<TObjectKey<AKataCharacterSpawner>, FIntPoint> DistanceSpawnerCells;

    /** 원점이 범위 안이거나 생성 중이거나 NPC 기록이 남은 스포너. 조회 셀 밖이어도 매 평가에 포함한다. */
    TMap<TObjectKey<AKataCharacterSpawner>, TWeakObjectPtr<AKataCharacterSpawner>> ActiveDistanceSpawners;

    /** 진행 중인 평가에서 방문할 스포너. 평가 시작 때 조회 셀과 활성 목록을 합쳐 만든다. */
    TArray<TWeakObjectPtr<AKataCharacterSpawner>> DistancePass;
    float DistanceCellSize = 5000.f;
    float MaxDistanceSpawnRange = 0.f;
    double DistanceElapsed = 0.0;
    int32 NextDistanceIndex = 0;
    int32 DistancePassRemaining = 0;
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
