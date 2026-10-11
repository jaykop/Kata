#pragma once

#include "CoreMinimal.h"
#include "Delegates/IDelegateInstance.h"
#include "GameFramework/CheatManager.h"
#include "KataAICheatExtension.generated.h"

class AActor;
class APawn;

/**
 * AI의 인지와 실행을 제어하는 치트 명령이다. 모듈이 시작될 때 모든 CheatManager에 붙는다.
 *
 * 토글 인자는 비우면 반전, 0·1이면 그 값으로 정한다. 대상 인자는 Self·Target·Nearest·All이다.
 * PC 인지 제외는 이 확장이 PlayerController별로 기억하고, Pawn이 바뀌면 새 Pawn에 다시 건다.
 * CheatManager는 Shipping 구성에서 만들어지지 않으므로 이 명령도 Shipping에서는 쓸 수 없다.
 */
UCLASS(NotBlueprintable)
class KATAAI_API UKataAICheatExtension : public UCheatManagerExtension
{
    GENERATED_BODY()

public:
    /**
     * 이미 있는 CheatManager와 이후 생성되는 CheatManager에 이 확장을 붙이도록 등록한다. 모듈 시작 시 호출한다.
     * @return UnregisterFromCheatManagers에 넘길 핸들. Shipping 구성에서는 등록하지 않고 빈 핸들을 돌려준다.
     */
    static FDelegateHandle RegisterForCheatManagers();

    /** RegisterForCheatManagers의 등록을 해제한다. 이미 붙은 확장은 CheatManager와 함께 사라진다. */
    static void UnregisterFromCheatManagers(FDelegateHandle Handle);

    /**
     * KataAI 타게팅이 PC Pawn을 보지 못하게 한다. 이미 인지한 AI는 시야 기록과 마지막 위치 기억을 바로 잃고,
     * 이후 스폰된 AI도 PC를 대상으로 삼지 않는다. 끄면 각 AI의 다음 인지 갱신에서 다시 보인다.
     * 엔진 Perception 자체는 그대로 동작하므로 KataAI 타게팅을 거치지 않는 인지 사용처에는 적용되지 않는다.
     */
    UFUNCTION(Exec)
    void KataHideFromAI(const FString& Enable);

    /**
     * 대상 AI의 StateTree 실행을 일시정지하거나 재개한다. 정지할 때 이동도 멈춘다. 대상 인자를 비우면 All이다.
     * 상태 인자를 비우면 대상 중 하나라도 실행 중일 때 모두 정지하고, 모두 정지해 있으면 모두 재개한다.
     * 이미 재생 중인 Kata 액션은 끝까지 진행된다.
     */
    UFUNCTION(Exec)
    void KataPauseAI(const FString& Target, const FString& Enable);

    /** PC 인지 제외를 정하고 현재 PC Pawn에 반영한다. 콘솔 명령과 디버그 메뉴가 함께 쓴다. */
    void SetHiddenFromAI(bool bHidden);

    /**
     * 액터들의 AI를 일시정지하거나 재개한다. AIController와 Brain이 없는 액터는 건너뛴다.
     * @return 상태를 바꾼 AI 수.
     */
    static int32 SetAIPaused(const TArray<AActor*>& Actors, bool bPause);

    /** 액터의 AI Brain이 일시정지 상태면 true. AI가 없으면 false. */
    static bool IsAIPaused(const AActor* Actor);

    bool IsHiddenFromAI() const { return bHiddenFromAI; }

    //~ Begin UCheatManagerExtension Interface
    virtual void AddedToCheatManager_Implementation() override;
    virtual void RemovedFromCheatManager_Implementation() override;
    //~ End UCheatManagerExtension Interface

private:
    UFUNCTION()
    void HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);

    /** Pawn의 숨김 상태를 바꾸고, 숨길 때는 월드의 모든 KataAI 타게팅에서 그 Pawn을 잊게 한다. */
    void SyncHidden(APawn* Pawn, bool bHidden) const;

    /** 콘솔과 로그에 결과를 한 줄 남긴다. */
    void Report(const FString& Message) const;

    bool bHiddenFromAI = false;
};
