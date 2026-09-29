#pragma once

#include "Character/KataCharacterSpawnSubsystem.h"
#include "CoreMinimal.h"
#include "Engine/CancellableAsyncAction.h"
#include "KataAsyncAction_SpawnCharacter.generated.h"

class AKataCharacter;
class UWorld;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKataSpawnCharacterPin, AKataCharacter*, Character);

/**
 * 캐릭터 데이터 테이블 행으로 캐릭터를 비동기 생성하는 Blueprint 노드.
 *
 * UKataCharacterSpawnSubsystem에 요청을 넘기고, 생성되면 On Spawned, 실패하면 On Failed를 실행한다.
 * 노드가 돌려주는 Async Action으로 Cancel을 호출하면 로드를 중단하며 어느 핀도 실행하지 않는다.
 */
UCLASS(meta = (DisplayName = "Kata Spawn Character Async Action"))
class KATAFRAMEWORK_API UKataAsyncAction_SpawnCharacter : public UCancellableAsyncAction
{
    GENERATED_BODY()

public:
    /**
     * 행의 에셋을 비동기로 로드한 뒤 캐릭터를 생성한다.
     *
     * @param Row 캐릭터 테이블과 행 이름. 행 구조가 FKataCharacterRow 계열이어야 한다.
     * @param SpawnTransform 생성 위치와 회전.
     */
    UFUNCTION(BlueprintCallable, Category = "Kata|Character",
        meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Spawn Kata Character"))
    static UKataAsyncAction_SpawnCharacter* SpawnKataCharacter(UObject* WorldContextObject, FDataTableRowHandle Row, FTransform SpawnTransform);

    /** 캐릭터가 생성되면 실행된다. */
    UPROPERTY(BlueprintAssignable)
    FKataSpawnCharacterPin OnSpawned;

    /** 행을 찾지 못하거나 로드·생성에 실패하면 실행된다. Character는 null이다. */
    UPROPERTY(BlueprintAssignable)
    FKataSpawnCharacterPin OnFailed;

    //~ Begin UBlueprintAsyncActionBase Interface
    virtual void Activate() override;
    //~ End UBlueprintAsyncActionBase Interface

    //~ Begin UCancellableAsyncAction Interface
    virtual void Cancel() override;
    //~ End UCancellableAsyncAction Interface

private:
    void HandleSpawnCompleted(AKataCharacter* Character);
    void HandleWorldBeginTearDown(UWorld* TearingDownWorld);
    void UnbindWorldTearDown();

    TWeakObjectPtr<UWorld> World;
    UPROPERTY(Transient)
    FDataTableRowHandle Row;
    FTransform SpawnTransform;
    FKataCharacterSpawnHandle SpawnHandle;
    FDelegateHandle WorldTearDownHandle;
};
