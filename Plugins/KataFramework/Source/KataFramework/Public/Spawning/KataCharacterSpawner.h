#pragma once

#include "CoreMinimal.h"
#include "Character/KataCharacterSpawnSubsystem.h"
#include "Engine/DataTable.h"
#include "GameFramework/Actor.h"
#include "KataCharacterSpawner.generated.h"

class AKataCharacter;
class UBoxComponent;
class USkeletalMeshComponent;
class UKataSpawnerComponent;
class UKataSpawnerComponent_SpawnSettings;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FKataSpawnerCharacterSpawnedSignature, AKataCharacter*, Character, int32, SpawnIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKataSpawnerCharacterFailedSignature, int32, SpawnIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FKataSpawnerBatchFinishedSignature, int32, SpawnedCount, int32, FailedCount, bool, bCancelled);

/**
 * NPC 테이블의 Row를 명시적으로 생성하는 스포너.
 * SpawnerComponents에 GEComponent 방식의 설정 UObject를 인라인으로 추가한다.
 * Spawn Settings가 개체 수와 영역을 제공하며, 없으면 액터 위치에서 1개를 생성한다.
 * 한 번에 하나의 생성 작업만 받는다. 설정과 후보 위치는 요청 전에 복사한다.
 * 취소나 스포너 종료는 대기 요청만 정리하며, 이미 생성한 NPC를 제거하거나 다시 생성하지 않는다.
 */
UCLASS(Blueprintable, meta = (DisplayName = "Kata Character Spawner"))
class KATAFRAMEWORK_API AKataCharacterSpawner : public AActor
{
    GENERATED_BODY()

public:
    AKataCharacterSpawner();

    /**
     * 생성할 NPC 테이블과 Row. 다음 요청부터 적용되며 진행 중인 요청은 바뀌지 않는다.
     * Details 선택기는 FKataNPCCharacterRow 테이블만 보여 준다. Blueprint에서 다른 테이블을 넣으면 생성 요청이 거부된다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kata|Spawning", meta = (RowType = "/Script/KataFramework.KataNPCCharacterRow"))
    FDataTableRowHandle CharacterRow;

    /**
     * 게임 시작 시 BeginPlay에서 SpawnCharacters를 한 번 호출할지 여부.
     * 끄면 Blueprint 이벤트 등에서 SpawnCharacters를 직접 호출해야 생성한다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Spawning")
    bool bSpawnOnBeginPlay = true;

    /** Details에서 타입을 선택해 추가하는 설정 객체 목록. ActorComponent가 아니며 각 스포너가 소유한다. */
    UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Kata|Spawning")
    TArray<TObjectPtr<UKataSpawnerComponent>> SpawnerComponents;

    /**
     * 설정을 복사하고 한 번의 생성 작업을 시작한다.
     * 성공적으로 요청을 받으면 true다. 설정 오류, 게임 월드 부재, 중복 요청은 false와 로그로 알린다.
     * 개체 수가 0이면 완료 이벤트를 즉시 부른다. 그 외에는 각 결과 뒤 완료 이벤트를 부른다.
     * 요청을 준비하는 동안 같은 스포너를 다시 호출하면 거절한다.
     */
    UFUNCTION(BlueprintCallable, Category = "Kata|Spawning")
    bool SpawnCharacters();

    /** 진행 중인 작업의 대기 요청을 취소한다. 진행 중일 때만 bCancelled=true인 완료 이벤트를 부른다. */
    UFUNCTION(BlueprintCallable, Category = "Kata|Spawning")
    void CancelSpawning();

    /** 이 스포너의 생성 작업이 진행 중이면 true다. */
    UFUNCTION(BlueprintPure, Category = "Kata|Spawning")
    bool IsSpawning() const { return bSpawnBatchActive; }

    /** 완료를 기다리는 개체 수. 요청을 제출하는 동안에는 아직 제출하지 않은 예약도 포함한다. */
    UFUNCTION(BlueprintPure, Category = "Kata|Spawning")
    int32 GetPendingSpawnCount() const { return PendingRequests.Num(); }

    /** 이 스포너가 생성한 캐릭터 중 아직 유효한 개체 수. 사망 여부를 뜻하지 않는다. */
    UFUNCTION(BlueprintPure, Category = "Kata|Spawning")
    int32 GetSpawnedCharacterCount() const;

    /** 이전 작업을 포함해 이 스포너가 생성한 유효 캐릭터를 돌려준다. 소유권을 이전하지 않는다. */
    UFUNCTION(BlueprintPure, Category = "Kata|Spawning")
    TArray<AKataCharacter*> GetSpawnedCharacters() const;

    /** 개체 생성 성공. SpawnIndex는 해당 작업 안에서 0부터 시작하며 완료 순서는 요청 순서와 다를 수 있다. */
    UPROPERTY(BlueprintAssignable, Category = "Kata|Spawning")
    FKataSpawnerCharacterSpawnedSignature OnCharacterSpawned;

    /** 로드·생성 실패. 설정 단계에서 거절된 작업은 이 이벤트 대신 SpawnCharacters가 false를 반환한다. */
    UPROPERTY(BlueprintAssignable, Category = "Kata|Spawning")
    FKataSpawnerCharacterFailedSignature OnCharacterSpawnFailed;

    /** 작업 완료 또는 명시적 취소. 취소된 대기 개체는 성공·실패 건수에 포함하지 않는다. 종료 중에는 알리지 않는다. */
    UPROPERTY(BlueprintAssignable, Category = "Kata|Spawning")
    FKataSpawnerBatchFinishedSignature OnBatchFinished;

protected:
    //~ Begin AActor Interface
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void OnConstruction(const FTransform& Transform) override;
    //~ End AActor Interface

private:
    /** 활성화된 첫 Spawn Settings를 찾는다. 없으면 null이다. 둘 이상이면 SpawnCharacters가 거절하므로 미리보기는 첫 항목만 그린다. */
    const UKataSpawnerComponent_SpawnSettings* FindEnabledSpawnSettings() const;

    /** Spawn Settings의 상대 영역과 Box Extent를 에디터 미리보기 상자에 반영한다. */
    void UpdateSpawnAreaPreview();

    /**
     * Character Row가 생성할 캐릭터의 메시를 에디터 미리보기 메시에 반영한다.
     * 영역 중심에 실제 생성과 같은 회전으로 놓고, 캐릭터 Blueprint의 Mesh 상대 Transform을 곱한다.
     * 에디터 월드에서만 동작하며 행의 에셋을 동기로 로드한다.
     */
    void UpdateCharacterPreview();

    void HandleSpawnCompleted(AKataCharacter* Character, uint32 RequestBatchId, int32 SpawnIndex);
    void FinishSpawnBatch(bool bCancelled, bool bBroadcast);

    UPROPERTY(Transient)
    FDataTableRowHandle ActiveRow;

#if WITH_EDITORONLY_DATA
    /** 생성 후보 영역을 에디터 뷰포트에 보여 주는 상자. 충돌이 없고 게임에서는 숨겨지며 쿠킹 빌드에는 포함되지 않는다. */
    UPROPERTY(VisibleAnywhere, Category = "Kata|Spawning")
    TObjectPtr<UBoxComponent> SpawnAreaPreview;

    /** 생성될 캐릭터의 외형을 영역 중심에 보여 주는 메시. 애니메이션 없이 기본 포즈로 그리며 게임에서는 숨겨진다. */
    UPROPERTY(VisibleAnywhere, Category = "Kata|Spawning")
    TObjectPtr<USkeletalMeshComponent> CharacterPreview;
#endif

    TMap<int32, FKataCharacterSpawnHandle> PendingRequests;
    /** 요청 당시 설정을 복사한 객체들. 원본을 편집해도 진행 중인 작업에 영향을 주지 않는다. */
    UPROPERTY(Transient)
    TArray<TObjectPtr<UKataSpawnerComponent>> ActiveComponents;
    TArray<TWeakObjectPtr<AKataCharacter>> SpawnedCharacters;
    TWeakObjectPtr<UKataCharacterSpawnSubsystem> ActiveSubsystem;
    uint32 BatchId = 0;
    int32 SucceededCount = 0;
    int32 FailedCount = 0;
    bool bStartingBatch = false;
    bool bSpawnBatchActive = false;
    bool bEndingPlay = false;
};
