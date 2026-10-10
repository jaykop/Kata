#pragma once

#include "CoreMinimal.h"
#include "Data/KataRowId.h"
#include "UObject/Object.h"
#include "KataSpawnerComponent.generated.h"

class AKataCharacter;
class AKataCharacterSpawner;
class UKataSpawnerComponent_SpawnArea;
struct FInstancedStruct;

/**
 * 스포너 Details에 인라인으로 추가하는 설정 컴포넌트의 기반 클래스.
 * GEComponent와 같은 Instanced UObject이며 스포너의 SpawnerComponents 배열이 소유한다.
 * 설정은 요청 시 복사하며, 대기 핸들·개체 목록 같은 실행 상태는 스포너가 관리한다.
 * 완료 훅은 캐릭터 BeginPlay 이후에 호출되므로 초기 AI 설정에 쓰지 않는다. BeginPlay 전에 적용할 값은 ModifySpawnRow로 행 사본에 기록한다.
 */
UCLASS(Abstract, BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced, CollapseCategories,
    meta = (DisplayName = "Kata Spawner Component"))
class KATAFRAMEWORK_API UKataSpawnerComponent : public UObject
{
    GENERATED_BODY()

public:
    /** 이 설정을 다음 생성 작업에 사용할지 여부. 진행 중인 작업의 설정 사본은 바뀌지 않는다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
    bool bEnabled = true;

    /** 생성 완료를 알린다. 설정을 변경하지 않으며, 실행 상태는 전달된 Spawner에 둔다. */
    UFUNCTION(BlueprintNativeEvent, Category = "Kata|Spawning")
    void OnCharacterSpawned(AKataCharacterSpawner* Spawner, AKataCharacter* Character, const FKataCharacterId& CharacterId) const;
    virtual void OnCharacterSpawned_Implementation(AKataCharacterSpawner* Spawner, AKataCharacter* Character,
        const FKataCharacterId& CharacterId) const;

    /**
     * Spawn Area가 고른 후보 위치를 보정한다. 스포너는 활성 설정을 배열 순서로 호출하고, 앞 설정의 결과를 다음 설정에 넘긴다.
     * false를 반환하면 이 후보를 버리고 Spawn Area에서 다시 뽑는다. 설정을 변경하지 않는다.
     * C++ 구현은 Spawner의 GetSpawnBatchContext로 시작 시 고정한 Transform을 조회할 수 있다.
     * 기존 Blueprint 훅의 매개변수는 유지하며, 액터의 현재 Transform을 직접 읽으면 시작 시점과 달라질 수 있다.
     *
     * @param SpawnArea 이번 요청의 Spawn Area 사본. 없으면 null이며 후보는 스포너 Transform이다.
     * @param OutTransform 보정한 Transform. 기본 구현은 CandidateTransform을 그대로 돌려준다.
     */
    UFUNCTION(BlueprintNativeEvent, Category = "Kata|Spawning")
    bool AdjustSpawnTransform(AKataCharacterSpawner* Spawner, const UKataSpawnerComponent_SpawnArea* SpawnArea, int32 SpawnIndex,
        const FTransform& CandidateTransform, FTransform& OutTransform) const;
    virtual bool AdjustSpawnTransform_Implementation(AKataCharacterSpawner* Spawner, const UKataSpawnerComponent_SpawnArea* SpawnArea,
        int32 SpawnIndex, const FTransform& CandidateTransform, FTransform& OutTransform) const;

    /**
     * 개체 하나의 위치를 정하려고 후보를 뽑는 최대 횟수. 스포너는 활성 설정 중 가장 큰 값을 쓰며, 모두 실패한 개체는 생성하지 않고 실패로 알린다.
     * 기본 구현은 1(다시 뽑지 않음)이다.
     */
    UFUNCTION(BlueprintNativeEvent, Category = "Kata|Spawning")
    int32 GetPlacementAttempts() const;
    virtual int32 GetPlacementAttempts_Implementation() const;

    /**
     * 배치를 시작할 때 이번 배치의 행 사본을 수정한다. 스포너는 Faction Override를 기록한 뒤 활성 설정을 배열 순서로 호출한다.
     * 행 사본은 생성 전 비동기 로드와 캐릭터의 행 적용(BeginPlay 전)에 그대로 쓰이며, 원본 테이블과 설정은 바뀌지 않는다.
     * 새로 지정한 소프트 참조는 행의 GatherAssetsToLoad가 수집하는 필드여야 로드된다. 기본 구현은 아무것도 하지 않는다.
     */
    virtual void ModifySpawnRow(FInstancedStruct& RowData) const;
};
