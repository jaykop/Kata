#pragma once

#include "CoreMinimal.h"
#include "Data/KataRowId.h"
#include "Engine/EngineTypes.h"
#include "StructUtils/InstancedStruct.h"
#include "Subsystems/WorldSubsystem.h"
#include "KataCharacterSpawnSubsystem.generated.h"

class AKataCharacter;
struct FStreamableHandle;

/** 생성 요청이 끝났을 때 불리는 콜백. 실패하면 Character가 null이다. */
DECLARE_DELEGATE_OneParam(FKataCharacterSpawnDelegate, AKataCharacter* /*Character*/);

/** 캐릭터 ID로 캐릭터가 생성됐을 때 알린다. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FKataOnCharacterSpawnedSignature, AKataCharacter*, Character, const FKataCharacterId&, CharacterId);

/** 진행 중인 생성 요청을 가리키는 핸들. 0은 무효 핸들이다. */
struct FKataCharacterSpawnHandle
{
    uint32 Id = 0;

    bool IsValid() const { return Id != 0; }
};

/**
 * 캐릭터 데이터 테이블 행으로 캐릭터를 비동기 생성하는 월드 서브시스템.
 *
 * 요청 시점에 행을 복사하고, 행이 참조하는 에셋을 비동기로 로드한 뒤 SpawnActorDeferred로 Character Class를 만든다.
 * FinishSpawning 안의 Construction Script가 끝난 뒤 OnConstruction에서 행을 적용하고, 생성이 끝나면 요청 콜백과 OnCharacterSpawned를 부른다.
 * 행은 캐릭터 ID로 프로젝트 설정의 데이터 컬렉션에서 찾는다.
 *
 * 로드 핸들은 적용이 끝나면 놓는다. 적용된 에셋은 캐릭터와 컴포넌트의 참조가 유지한다.
 * 요청이 취소되거나 월드가 정리되면 로드를 중단하고 콜백을 부르지 않는다. 행은 요청 시점에 복사하므로 원본 테이블의 수명과 무관하다.
 */
UCLASS()
class KATAFRAMEWORK_API UKataCharacterSpawnSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    //~ Begin USubsystem Interface
    virtual void Deinitialize() override;
    //~ End USubsystem Interface

    /**
     * 캐릭터 ID의 행으로 캐릭터 생성을 요청한다.
     *
     * 로드가 끝나면 다음 틱 이후에 OnComplete를 부른다. 에셋이 이미 로드되어 있어도 이 함수 안에서 부르지 않는다.
     * 행을 찾지 못하는 등 요청 단계에서 실패하면 OnComplete를 null로 즉시 부르고 무효 핸들을 돌려준다.
     *
     * @param CharacterId 데이터 컬렉션의 캐릭터 행 ID. 비었거나 행이 없으면 실패한다.
     * @param SpawnTransform 생성 위치와 회전.
     * @param OnComplete 생성된 캐릭터, 실패하면 null을 받는다. 취소되면 불리지 않는다.
     * @param CollisionHandling 생성 위치가 막혀 있을 때의 처리.
     * @return 취소와 조회에 쓰는 핸들. 요청 단계에서 실패하면 무효 핸들이다.
     */
    FKataCharacterSpawnHandle RequestSpawn(const FKataCharacterId& CharacterId, const FTransform& SpawnTransform,
        FKataCharacterSpawnDelegate OnComplete,
        ESpawnActorCollisionHandlingMethod CollisionHandling = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);

    /** 진행 중인 요청을 취소한다. 로드를 중단하고 콜백을 부르지 않는다. 이미 끝났거나 무효한 핸들이면 아무것도 하지 않는다. */
    void CancelSpawn(FKataCharacterSpawnHandle Handle);

    /** 요청이 아직 로드 중이면 true. */
    bool IsSpawnPending(FKataCharacterSpawnHandle Handle) const;

    /** 진행 중인 생성 요청 수. 늦게 시작한 시스템이 생성이 끝났는지 확인할 때 쓴다. */
    UFUNCTION(BlueprintPure, Category = "Kata|Character")
    int32 GetPendingSpawnCount() const { return PendingRequests.Num(); }

    /** 행으로 캐릭터가 생성될 때마다 요청 콜백 뒤에 알린다. 실패한 요청은 알리지 않는다. */
    UPROPERTY(BlueprintAssignable, Category = "Kata|Character")
    FKataOnCharacterSpawnedSignature OnCharacterSpawned;

private:
    struct FPendingRequest
    {
        FKataCharacterId CharacterId;
        FInstancedStruct RowData;
        FTransform SpawnTransform;
        ESpawnActorCollisionHandlingMethod CollisionHandling = ESpawnActorCollisionHandlingMethod::Undefined;
        FKataCharacterSpawnDelegate OnComplete;
        TSharedPtr<FStreamableHandle> LoadHandle;
    };

    void HandleAssetsLoaded(uint32 RequestId);
    AKataCharacter* SpawnFromRequest(const FPendingRequest& Request) const;

    TMap<uint32, FPendingRequest> PendingRequests;
    uint32 LastRequestId = 0;
};
