#pragma once

#include "CoreMinimal.h"
#include "Character/KataCharacterSpawnSubsystem.h"
#include "Data/KataRowId.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Spawning/KataSpawnBatchTypes.h"
#include "Spawning/KataDespawnBatchTypes.h"
#include "KataCharacterSpawner.generated.h"

class AKataCharacter;
class UBoxComponent;
class UDataTable;
class USphereComponent;
class USkeletalMeshComponent;
class UKataDeathComponent;
class UKataSpawnerComponent;
class UKataSpawnerComponent_SpawnArea;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FKataSpawnerCharacterSpawnedSignature, AKataCharacter*, Character, int32, SpawnIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKataSpawnerCharacterDiedSignature, AKataCharacter*, Character);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKataSpawnerCharacterFailedSignature, int32, SpawnIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FKataSpawnerBatchFinishedSignature, int32, SpawnedCount, int32, FailedCount, bool, bCancelled);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FKataSpawnerDespawnFinishedSignature, int32, RemovedCharacterCount, int32, FailedActorCount);

/** 스포너가 생성을 시작하는 방식. */
UENUM(BlueprintType)
enum class EKataSpawnerActivation : uint8
{
    /** BeginPlay에서 SpawnCharacters를 한 번 호출한다. */
    BeginPlay UMETA(DisplayName = "Begin Play"),
    /** 자동으로 생성하지 않는다. Blueprint 등에서 SpawnCharacters를 직접 호출한다. */
    Manual,
    /** 월드 관리자가 플레이어 Pawn과의 거리로 생성·제거하며 수동 생성·취소·제거 호출을 거절한다. */
    PlayerDistance UMETA(DisplayName = "Player Distance"),
    /** 레벨에 배치한 채로 끈다. 자동 생성하지 않으며 수동 생성·취소·제거 호출도 거절한다. */
    Disabled
};

/**
 * NPC 테이블의 Row를 명시적으로 생성하는 스포너.
 * SpawnerComponents에 GEComponent 방식의 설정 UObject를 인라인으로 추가한다.
 * Spawn Area가 개체 수와 영역을 제공하며, 없으면 액터 위치에서 1개를 생성한다.
 * 한 번에 하나의 생성 작업만 받는다. 행·설정·영역을 고정하고, 선택한 실행 방식으로 위치 확인과 요청을 진행한다.
 * 취소는 이미 생성한 NPC를 유지한다. 디스폰은 별도 호출이며 완료까지 새 생성을 받지 않는다.
 * Activation이 Player Distance이면 BeginPlay부터 월드 관리자가 플레이어 거리로 생성·제거를 관리하며 수동 호출을 거절한다.
 */
UCLASS(Blueprintable, PrioritizeCategories = "Kata|Spawning", meta = (DisplayName = "Kata Character Spawner"))
class KATAFRAMEWORK_API AKataCharacterSpawner : public AActor
{
    GENERATED_BODY()

public:
    AKataCharacterSpawner();

    /**
     * 캐릭터를 고를 NPC 테이블. 데이터 컬렉션의 NPC 테이블 목록에 있는 테이블이어야 한다.
     * Details 선택기는 그 목록의 테이블만 보여 준다. 비워 두면 모든 NPC 테이블에서 고른다.
     * Blueprint에서 컬렉션에 없는 테이블을 넣으면 드롭다운이 비고 생성 요청이 거부된다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kata|Spawning")
    TObjectPtr<UDataTable> SourceTable;

    /**
     * 생성할 NPC 캐릭터 ID. 다음 요청부터 적용되며 진행 중인 요청은 바뀌지 않는다.
     * Details 드롭다운은 Source Table(비었으면 모든 NPC 테이블)의 행만 보여 준다.
     * Blueprint에서 그 범위에 없는 ID를 넣으면 생성 요청이 거부된다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kata|Spawning",
        meta = (RowType = "/Script/KataFramework.KataNPCCharacterRow", SourceTableProperty = "SourceTable"))
    FKataCharacterId CharacterId;

    /**
     * 이 스포너가 만드는 캐릭터의 팩션. 비워 두면 행의 Faction을 쓰고, 행도 비었으면 Character Class의 기본값을 쓴다.
     * 배치를 시작할 때 행 사본에 기록하므로 다음 배치부터 적용되며 원본 테이블은 바뀌지 않는다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kata|Spawning", meta = (Categories = "Faction"))
    FGameplayTag FactionOverride;

    /**
     * 생성을 시작하는 방식. BeginPlay에서 한 번 읽으며 이후 변경은 반영하지 않는다.
     * Begin Play는 시작 시 한 번, Manual은 SpawnCharacters 호출 때, Player Distance는 월드 관리자의 거리 판정으로 생성한다.
     * Disabled는 생성하지 않고 수동 호출도 거절한다. 레벨에 배치한 스포너를 지우지 않고 끌 때 쓴다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Spawning")
    EKataSpawnerActivation Activation = EKataSpawnerActivation::BeginPlay;

    /** Player Distance에서 스포너 원점과 플레이어 Pawn의 3D 거리가 이 값 이하이면 진입으로 본다(cm). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Spawning",
        meta = (EditCondition = "Activation == EKataSpawnerActivation::PlayerDistance", EditConditionHides, ClampMin = "1.0", Units = "cm"))
    float SpawnDistance = 3000.f;

    /**
     * Player Distance에서 NPC 또는 스포너 원점과 플레이어 Pawn의 거리가 이 값을 넘으면 이탈로 본다(cm).
     * 경계에서 생성·제거가 반복되지 않도록 SpawnDistance보다 커야 하며, 그렇지 않으면 경고 후 생성하지 않는다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Spawning",
        meta = (EditCondition = "Activation == EKataSpawnerActivation::PlayerDistance", EditConditionHides, ClampMin = "1.0", Units = "cm"))
    float DespawnDistance = 4000.f;

    /**
     * 공용 월드 예산으로 위치 준비·로드 제출·생성을 분산한다. 변경은 다음 배치부터 적용한다.
     * Player Distance는 항상 분산하고 Disabled는 생성하지 않으므로 숨긴다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kata|Spawning",
        meta = (EditCondition = "Activation != EKataSpawnerActivation::PlayerDistance && Activation != EKataSpawnerActivation::Disabled",
            EditConditionHides))
    bool bUseTimeSlicing = false;

    /** 월드가 유지되는 동안 스포너가 종료되면 생성 NPC를 관리자에게 넘겨 제거한다. 기본 false는 기존 유지 계약이다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Spawning")
    bool bDespawnOnEndPlay = false;

    /** Details에서 타입을 선택해 추가하는 설정 객체 목록. ActorComponent가 아니며 각 스포너가 소유한다. */
    UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Kata|Spawning")
    TArray<TObjectPtr<UKataSpawnerComponent>> SpawnerComponents;

    /**
     * 설정을 복사하고 한 번의 생성 작업을 시작한다.
     * 성공적으로 요청을 받으면 true다. 설정 오류, 게임 월드 부재, 중복 요청은 false와 로그로 알린다.
     * 개체 수가 0이면 완료 이벤트를 즉시 부른다. 그 외에는 각 결과 뒤 완료 이벤트를 부른다.
     * 요청을 준비하는 동안 같은 스포너를 다시 호출하면 거절한다.
     * 분산 모드의 true는 수락이며, 이후 위치 계산 실패는 개별 실패 이벤트로 알린다.
     * 거리 관리 스포너와 Disabled 스포너는 이 호출을 거절한다.
     */
    UFUNCTION(BlueprintCallable, Category = "Kata|Spawning")
    bool SpawnCharacters();

    /** 진행 중인 작업의 대기 요청을 취소한다. 진행 중일 때만 bCancelled=true인 완료 이벤트를 부른다. 거리 관리 스포너와 Disabled 스포너는 무시한다. */
    UFUNCTION(BlueprintCallable, Category = "Kata|Spawning")
    void CancelSpawning();

    /**
     * 대기 생성을 취소하고 이전 배치를 포함한 이 스포너의 생성 기록을 공용 예산으로 제거한다.
     * 제거 중·설정 준비 중·월드 종료 중이면 false다. 수락하면 다음 관리자 Tick부터 진행하고 완료 이벤트를 한 번 호출한다.
     * 제거 실패 기록은 보존하므로 완료 뒤 다시 요청할 수 있다. 거리 관리 스포너와 Disabled 스포너는 이 호출을 거절한다.
     */
    UFUNCTION(BlueprintCallable, Category = "Kata|Spawning")
    bool DespawnCharacters();

    /** 수동 제거 요청의 NPC와 소유 Controller 정리가 진행 중이면 true다. 거리 관리의 개체별 제거는 포함하지 않는다. */
    UFUNCTION(BlueprintPure, Category = "Kata|Spawning")
    bool IsDespawning() const { return ActiveDespawnBatch.IsValid(); }

    /** 아직 NPC·Controller 정리가 끝나지 않은 생성 기록 수. 이미 외부에서 제거한 개체의 기록도 포함할 수 있다. */
    UFUNCTION(BlueprintPure, Category = "Kata|Spawning")
    int32 GetPendingDespawnCount() const;

    /** Activation이 Player Distance이고 BeginPlay에서 유효한 거리로 관리를 시작했으면 true다. */
    UFUNCTION(BlueprintPure, Category = "Kata|Spawning")
    bool IsDistanceManaged() const { return bDistanceManaged; }

    /**
     * 거리 관리 스포너가 다음 원점 진입 때 생성할 수. 최초 진입 전에는 Spawn Area가 정할 수량이 아직 없으므로 0이다.
     * 거리로 제거를 제출한 NPC와 원점 이탈로 취소한 생성 대기분을 포함한다.
     */
    UFUNCTION(BlueprintPure, Category = "Kata|Spawning")
    int32 GetPendingRespawnCount() const { return PendingRespawnCount; }

    /** 이 스포너의 생성 작업이 진행 중이면 true다. */
    UFUNCTION(BlueprintPure, Category = "Kata|Spawning")
    bool IsSpawning() const { return bSpawnBatchActive; }

    /** 완료를 기다리는 개체 수. 요청을 제출하는 동안에는 아직 제출하지 않은 예약도 포함한다. */
    UFUNCTION(BlueprintPure, Category = "Kata|Spawning")
    int32 GetPendingSpawnCount() const { return ActiveBatch != nullptr ? ActiveBatch->RemainingCount : 0; }

    /**
     * 준비 또는 실행 중인 배치의 고정 정보를 제공한다. 배치가 없으면 null이다.
     * 반환 포인터는 현재 훅 호출 동안만 사용하며, 취소·종료 후를 위해 저장하지 않는다.
     * 기존 Blueprint 위치 보정 훅을 유지하면서 C++ 보정 구현이 시작 시점의 영역 기준을 사용할 때 쓴다.
     */
    const FKataSpawnBatchContext* GetSpawnBatchContext() const;

    /** 이 스포너가 생성한 캐릭터 중 아직 유효한 개체 수. 사망 여부를 뜻하지 않는다. */
    UFUNCTION(BlueprintPure, Category = "Kata|Spawning")
    int32 GetSpawnedCharacterCount() const;

    /** 이전 작업을 포함해 이 스포너가 생성한 유효 캐릭터를 돌려준다. 시체도 포함한다. 소유권을 이전하지 않는다. */
    UFUNCTION(BlueprintPure, Category = "Kata|Spawning")
    TArray<AKataCharacter*> GetSpawnedCharacters() const;

    /** 이 스포너가 생성한 캐릭터 중 지금까지 죽은 수. 시체를 제거한 뒤에도 줄지 않는다. 재생성 정책에는 쓰지 않는다. */
    UFUNCTION(BlueprintPure, Category = "Kata|Spawning")
    int32 GetDeadCharacterCount() const { return DeadCharacterCount; }

    /** 개체 생성 성공. SpawnIndex는 해당 작업 안에서 0부터 시작하며 완료 순서는 요청 순서와 다를 수 있다. */
    UPROPERTY(BlueprintAssignable, Category = "Kata|Spawning")
    FKataSpawnerCharacterSpawnedSignature OnCharacterSpawned;

    /** 로드·생성 실패. 설정 단계에서 거절된 작업은 이 이벤트 대신 SpawnCharacters가 false를 반환한다. */
    UPROPERTY(BlueprintAssignable, Category = "Kata|Spawning")
    FKataSpawnerCharacterFailedSignature OnCharacterSpawnFailed;

    /** 작업 완료 또는 명시적 취소. 취소된 대기 개체는 성공·실패 건수에 포함하지 않는다. 종료 중에는 알리지 않는다. */
    UPROPERTY(BlueprintAssignable, Category = "Kata|Spawning")
    FKataSpawnerBatchFinishedSignature OnBatchFinished;

    /** 제거 완료. 제거한 NPC 수와 NPC·Controller의 Destroy 실패 수이며, 종료 중에는 알리지 않는다. */
    UPROPERTY(BlueprintAssignable, Category = "Kata|Spawning")
    FKataSpawnerDespawnFinishedSignature OnDespawnFinished;

    /** 이 스포너가 생성한 캐릭터가 죽었다. 사망 정리가 끝난 뒤 사망 연출 전에 한 번 오며, 종료 중에는 알리지 않는다. */
    UPROPERTY(BlueprintAssignable, Category = "Kata|Spawning")
    FKataSpawnerCharacterDiedSignature OnCharacterDied;

protected:
    //~ Begin AActor Interface
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void OnConstruction(const FTransform& Transform) override;
    //~ End AActor Interface

    //~ Begin UObject Interface
    /** 이전 bSpawnOnBeginPlay=false를 Activation의 Manual로 옮긴다. */
    virtual void PostLoad() override;
    //~ End UObject Interface

private:
    friend class UKataSpawnerSubsystem;

    bool IsScheduledBatchActive(uint32 ExpectedBatchId) const;
    bool ProcessTimeSlicedStep(uint32 ExpectedBatchId, int32 GlobalRequestLimit, int32 SpawnerRequestLimit);
    void StopScheduledBatch(uint32 ExpectedBatchId);
    void FinishDespawnBatch(const TSharedPtr<FKataDespawnBatchState>& Batch, bool bBroadcast);

    /**
     * 월드 관리자가 주기적으로 호출하는 거리 판단. 원점 진입 시 생성을 시작하고, 원점 이탈 시 진행 중 생성을 취소한다.
     * 생성한 NPC는 각자 현재 위치로 판정해 DespawnDistance 밖이면 개체별 제거 작업으로 제출한다.
     */
    void EvaluateDistance(const FVector& PlayerLocation);

    /**
     * 원점이 범위 안이거나 생성 중이거나 정리할 생성 기록이 남았으면 true다.
     * 관리자는 이 스포너를 그리드 조회 범위 밖에서도 계속 평가해 원점 이탈과 NPC 이탈을 놓치지 않는다.
     */
    bool HasActiveDistanceState() const;

    /** BeginPlay에서 거리 값을 고정하고 관리자에 등록한다. 값이 잘못됐거나 관리자가 없으면 경고 후 생성하지 않는다. */
    void InitializeDistanceManagement();

    /** Player Distance의 거리 범위를 선택 시에만 보이는 에디터 미리보기에 반영한다. 다른 방식이면 숨긴다. */
    void UpdateDistancePreview();

    /**
     * 생성 작업 시작의 공용 경로. CountOverride가 0 이상이면 Spawn Area 수량 대신 그 수를 생성한다.
     * bForceTimeSlicing이면 bUseTimeSlicing과 무관하게 분산 경로를 쓴다.
     */
    bool StartSpawnBatch(int32 CountOverride, bool bForceTimeSlicing);

    /** 거리 관리에서 원점 이탈로 진행 중 생성을 취소하고 미완료 수를 재생성 수에 합한다. */
    void CancelDistanceSpawnBatch();
    /** 활성화된 첫 Spawn Area를 찾는다. 없으면 null이다. 둘 이상이면 SpawnCharacters가 거절하므로 미리보기는 첫 항목만 그린다. */
    const UKataSpawnerComponent_SpawnArea* FindEnabledSpawnArea() const;

    /** Spawn Area의 상대 영역과 모양(구 반지름 또는 Box Extent)을 에디터 미리보기에 반영한다. 고른 모양의 미리보기만 보인다. */
    void UpdateSpawnAreaPreview();

    /**
     * Character Id가 생성할 캐릭터의 메시를 에디터 미리보기 메시에 반영한다.
     * 영역 중심에 실제 생성과 같은 회전으로 놓고, 캐릭터 Blueprint의 Mesh 상대 Transform을 곱한다.
     * 에디터 월드에서만 동작하며 행의 에셋을 동기로 로드한다.
     */
    void UpdateCharacterPreview();

    void HandleSpawnCompleted(AKataCharacter* Character, uint32 RequestBatchId, int32 SpawnIndex);
    void FinishSpawnBatch(bool bCancelled, bool bBroadcast);

    /** 생성한 캐릭터의 사망 컴포넌트에 사망 기록과 시체 제거 처리를 연결한다. */
    void BindDeathComponent(AKataCharacter* Character);

    /** 생성 기록에 사망을 표시하고 OnCharacterDied를 알린다. */
    void HandleCharacterDied(UKataDeathComponent* DeathComponent);

    /** 시체 하나를 공용 제거 큐로 넘겨 NPC와 소유 Controller를 함께 정리한다. 처리하지 못하면 false를 돌려 사망 컴포넌트가 Destroy하게 한다. */
    bool HandleDeadCharacterRemoval(UKataDeathComponent* DeathComponent);

    /** 캐릭터의 생성 기록 위치. 없으면 INDEX_NONE이다. */
    int32 FindOwnershipRecordIndex(const AActor* Character) const;

    bool PrepareSpawnBatch(UKataSpawnBatchState& Batch, int32 CountOverride);
    bool AdvanceSpawnPlacement(UKataSpawnBatchState& Batch);
    void SubmitNextSpawnRequest(uint32 ExpectedBatchId);
    bool CanContinueSpawning() const;

#if WITH_EDITORONLY_DATA
    /** 상자 모양의 생성 후보 영역을 에디터 뷰포트에 보여 준다. 충돌이 없고 게임에서는 숨겨지며 쿠킹 빌드에는 포함되지 않는다. */
    UPROPERTY(VisibleAnywhere, Category = "Kata|Spawning")
    TObjectPtr<UBoxComponent> SpawnAreaPreview;

    /**
     * 구 모양의 생성 후보 영역을 에디터 뷰포트에 보여 준다. 충돌이 없고 게임에서는 숨겨지며 쿠킹 빌드에는 포함되지 않는다.
     * 엔진의 구 컴포넌트는 비균등 스케일을 가장 작은 축 값으로 그리므로, 영역 Transform의 스케일이 비균등하면 실제 후보 영역과 모양이 다르다.
     */
    UPROPERTY(VisibleAnywhere, Category = "Kata|Spawning")
    TObjectPtr<USphereComponent> SphereAreaPreview;

    /** 생성될 캐릭터의 외형을 영역 중심에 보여 주는 메시. 애니메이션 없이 기본 포즈로 그리며 게임에서는 숨겨진다. */
    UPROPERTY(VisibleAnywhere, Category = "Kata|Spawning")
    TObjectPtr<USkeletalMeshComponent> CharacterPreview;

    /** Player Distance의 SpawnDistance를 스포너를 선택했을 때만 보여 준다. 쿠킹 빌드에는 포함되지 않는다. */
    UPROPERTY(VisibleAnywhere, Category = "Kata|Spawning")
    TObjectPtr<USphereComponent> SpawnDistancePreview;

    /** Player Distance의 DespawnDistance를 스포너를 선택했을 때만 보여 준다. 쿠킹 빌드에는 포함되지 않는다. */
    UPROPERTY(VisibleAnywhere, Category = "Kata|Spawning")
    TObjectPtr<USphereComponent> DespawnDistancePreview;
#endif

    /** 이전 버전의 BeginPlay 자동 생성 여부. 저장 이름은 bSpawnOnBeginPlay이며 PostLoad에서 Activation으로 옮기고 다시 저장하지 않는다. */
    UPROPERTY()
    bool bSpawnOnBeginPlay_DEPRECATED = true;

    /** 요청 제출 전 후보 계산 중에도 설정과 행의 참조를 GC에서 보호한다. */
    UPROPERTY(Transient)
    TObjectPtr<UKataSpawnBatchState> PreparingBatch;

    /** 진행 중인 배치의 실행 데이터. 결과 콜백 안에서 정리되더라도 호출 중의 강한 참조가 수명을 유지한다. */
    UPROPERTY(Transient)
    TObjectPtr<UKataSpawnBatchState> ActiveBatch;

    TArray<TWeakObjectPtr<AKataCharacter>> SpawnedCharacters;
    TArray<TSharedPtr<FKataCharacterSpawnOwnership>> SpawnOwnershipRecords;
    TSharedPtr<FKataDespawnBatchState> ActiveDespawnBatch;
    uint32 BatchId = 0;
    bool bStartingBatch = false;
    bool bSpawnBatchActive = false;
    bool bEndingPlay = false;

    /** BeginPlay에서 고정한 거리 관리 상태. 설정 객체에는 실행 상태를 저장하지 않는다. */
    float DistanceSpawnRange = 0.f;
    float DistanceDespawnRange = 0.f;
    int32 PendingRespawnCount = 0;
    /** 관리자 큐에서 아직 끝나지 않은 거리 제거 배치 수. 완료 전까지 그리드 조회 밖에서도 평가 대상에 남긴다. */
    int32 PendingDistanceDespawnBatches = 0;
    /** 관리자 큐에서 아직 끝나지 않은 시체 제거 배치 수. 거리 제거와 같은 이유로 완료 전까지 평가 대상에 남긴다. */
    int32 PendingDeathRemovalBatches = 0;
    int32 DeadCharacterCount = 0;
    bool bDistanceManaged = false;
    bool bOriginInRange = false;
    bool bInitialSpawnPending = true;
    /** 원점 진입 후 아직 생성을 시작하지 못했으면 true다. 이탈하면 해제해 범위 안 체류만으로 다시 생성하지 않는다. */
    bool bEntrySpawnPending = false;
};
