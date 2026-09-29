#pragma once

#include "Character/KataCharacterSpawnSubsystem.h"
#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameFramework/GameModeBase.h"
#include "KataGameMode.generated.h"

class AKataCharacter;

/**
 * PC를 캐릭터 데이터 테이블 행으로 생성하는 GameMode.
 *
 * 플레이어를 시작할 때 기본 폰을 동기로 만드는 대신 Player Character Row로 UKataCharacterSpawnSubsystem에 비동기 생성을 요청하고,
 * 생성되면 그 캐릭터에 빙의시킨다. 로드하는 동안 플레이어에게는 폰이 없다.
 * 준비 완료 시점은 컨트롤러의 OnPossessedPawnChanged나 서브시스템의 OnCharacterSpawned로 받는다.
 *
 * Player Character Row를 비워 두면 AGameModeBase처럼 Default Pawn Class를 동기로 생성한다.
 * 생성에 실패하면 로그를 남기고 플레이어를 폰 없이 둔다. 행 기반 생성은 Player Start에서 시작하는 경로에만 적용한다.
 */
UCLASS(Blueprintable, meta = (DisplayName = "Kata Game Mode"))
class KATAFRAMEWORK_API AKataGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AKataGameMode(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    //~ Begin AGameModeBase Interface
    virtual void RestartPlayerAtPlayerStart(AController* NewPlayer, AActor* StartSpot) override;
    virtual void Logout(AController* Exiting) override;
    //~ End AGameModeBase Interface

    /** 플레이어의 캐릭터를 아직 로드하고 있으면 true. 로딩 화면처럼 늦게 시작한 시스템이 대기 여부를 판단할 때 쓴다. */
    UFUNCTION(BlueprintPure, Category = "Kata|Character")
    bool IsPlayerCharacterPending(AController* Player) const;

protected:
    /** PC로 생성할 캐릭터 행. 비워 두면 Default Pawn Class를 쓴다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Character", meta = (RowType = "/Script/KataFramework.KataPlayerCharacterRow"))
    FDataTableRowHandle PlayerCharacterRow;

private:
    void HandlePlayerCharacterSpawned(AKataCharacter* Character, TWeakObjectPtr<AController> Player, TWeakObjectPtr<AActor> StartSpot,
        FRotator StartRotation);

    TMap<TWeakObjectPtr<AController>, FKataCharacterSpawnHandle> PendingPlayerSpawns;
};
