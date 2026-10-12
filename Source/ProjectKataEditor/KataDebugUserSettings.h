#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "KataDebugUserSettings.generated.h"

/**
 * Kata Debug 탭의 마지막 상태다. 사용자별 설정(EditorPerProjectUserSettings)에 저장해 에디터를 다시 열어도 유지한다.
 * 스포너 입력값은 UKataRuntimeSpawnerUserSettings에 따로 저장한다.
 */
UCLASS(config = EditorPerProjectUserSettings)
class UKataDebugUserSettings : public UObject
{
    GENERATED_BODY()

public:
    /** 마지막으로 고른 패널의 클래스 이름. 탭을 처음 열 때 이 패널을 고른다. */
    UPROPERTY(config)
    FName ActivePanelClass;

    /** 플레이어 치트 토글. 치트 패널에서 바꾸면 저장하고, PIE가 시작될 때 첫 PlayerController에 다시 건다. */
    UPROPERTY(config)
    bool bPlayerGod = false;

    UPROPERTY(config)
    bool bPlayerNoDamage = false;

    UPROPERTY(config)
    bool bPlayerInfiniteStamina = false;

    UPROPERTY(config)
    bool bPlayerHiddenFromAI = false;
};
