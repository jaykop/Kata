#pragma once

#include "CoreMinimal.h"
#include "Delegates/IDelegateInstance.h"
#include "GameFramework/CheatManager.h"
#include "KataWorldCheatExtension.generated.h"

class UKataWorldTimeSubsystem;

/**
 * 월드 시각을 조작하는 치트 명령이다. 모듈이 시작될 때 모든 CheatManager에 붙는다.
 *
 * 명령은 플레이어 월드의 UKataWorldTimeSubsystem을 바꾸며, 결과를 콘솔과 로그에 한 줄로 남긴다.
 * CheatManager는 Shipping 구성에서 만들어지지 않으므로 이 명령도 Shipping에서는 쓸 수 없다.
 */
UCLASS(NotBlueprintable)
class KATAWORLD_API UKataWorldCheatExtension : public UCheatManagerExtension
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

    /** 인자를 비우면 현재 시각, 시간대, 모드, 하루 길이를 보여 준다. 시각(시)을 주면 그 시각으로 바꾼다. 예: KataTime 18.5 */
    UFUNCTION(Exec)
    void KataTime(const FString& Hour);

    /** 시각 흐름을 켜거나 끈다. 비우면 반전하고, 1·true·on이면 Flowing, 0·false·off면 Fixed로 정한다. */
    UFUNCTION(Exec)
    void KataTimeFlow(const FString& Enable);

    /** 게임 하루가 지나는 실제 시간(분)을 바꾼다. 0.1분보다 짧은 값은 0.1분이 된다. 예: KataDayLength 2 */
    UFUNCTION(Exec)
    void KataDayLength(const FString& Minutes);

private:
    /** 플레이어 월드의 시간 Subsystem. 없으면 결과를 남기고 nullptr을 돌려준다. */
    UKataWorldTimeSubsystem* FindTimeSubsystem(const TCHAR* CommandName) const;

    /** 현재 시간 상태를 한 줄 문자열로 만든다. */
    static FString DescribeTime(const UKataWorldTimeSubsystem& TimeSubsystem);

    /** 콘솔과 로그에 결과를 한 줄 남긴다. */
    void Report(const FString& Message) const;
};
