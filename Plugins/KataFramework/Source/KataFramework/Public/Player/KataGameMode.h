#pragma once

#include "Character/KataCharacterSpawnSubsystem.h"
#include "CoreMinimal.h"
#include "Data/KataRowId.h"
#include "Engine/TimerHandle.h"
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
 * 플레이어 캐릭터가 죽으면 RequestPlayerRestart로 지연 뒤 같은 경로로 새 캐릭터를 만든다. 상태는 행에서 새로 조립되므로 초기화된다.
 */
UCLASS(Blueprintable, PrioritizeCategories = "Kata|Character", meta = (DisplayName = "Kata Game Mode"))
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

    /**
     * 플레이어를 Player Restart Delay 뒤 Player Start에서 다시 시작한다. AKataPlayerController가 빙의한 캐릭터의 사망 때 호출한다.
     * 죽은 Pawn은 새 캐릭터가 준비될 때까지 빙의를 유지해 카메라가 시체를 비추고, 새 캐릭터에 빙의할 때 풀린다.
     * 이미 예약됐거나, 예약 시간이 됐을 때 살아 있는 Pawn을 조종 중이면 아무것도 하지 않는다.
     */
    UFUNCTION(BlueprintCallable, Category = "Kata|Character")
    void RequestPlayerRestart(AController* Player);

protected:
    /** 플레이어 캐릭터가 죽은 뒤 다시 시작할 때까지의 시간(초). 0이면 사망 처리 직후 시작한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Character", meta = (ClampMin = "0.0", Units = "s"))
    float PlayerRestartDelay = 3.0f;

    /**
     * PC로 생성할 캐릭터 ID. 비워 두면 Default Pawn Class를 쓴다.
     * Details 드롭다운은 PC 테이블의 행만 보여 준다. PC 테이블의 행이 아니면 경고 후 Default Pawn Class로 시작한다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Character", meta = (RowType = "/Script/KataFramework.KataPlayerCharacterRow"))
    FKataCharacterId PlayerCharacterId;

private:
    void HandlePlayerCharacterSpawned(AKataCharacter* Character, TWeakObjectPtr<AController> Player, TWeakObjectPtr<AActor> StartSpot,
        FRotator StartRotation);

    void HandlePlayerRestartTimer(TWeakObjectPtr<AController> Player);

    /** Controller가 살아 있는 Pawn을 조종 중이면 true. 죽은 Pawn은 재시작에서 교체할 대상으로 본다. */
    static bool HasLivingPawn(const AController* Controller);

    TMap<TWeakObjectPtr<AController>, FKataCharacterSpawnHandle> PendingPlayerSpawns;
    TMap<TWeakObjectPtr<AController>, FTimerHandle> PendingPlayerRestarts;
};
