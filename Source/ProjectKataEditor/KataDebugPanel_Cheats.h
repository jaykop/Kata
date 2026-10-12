#pragma once

#include "CoreMinimal.h"
#include "KataDebugPanel.h"
#include "KataDebugPanel_Cheats.generated.h"

/**
 * 치트 패널이다. KataFramework·KataAI 치트 확장의 함수를 버튼과 체크박스로 부른다.
 * 콘솔 명령과 같은 함수를 쓰므로 콘솔에서 바꾼 토글도 이 패널에 그대로 보인다.
 * 플레이어 토글은 패널이 보이는 동안 UKataDebugUserSettings에 저장하고, 다음 PIE가 시작되면 다시 건다.
 */
UCLASS()
class UKataDebugPanel_Cheats : public UKataDebugPanel
{
    GENERATED_BODY()

public:
    virtual FString GetDisplayName() const override { return TEXT("Cheats"); }
    virtual int32 GetSortOrder() const override { return 0; }
    virtual bool UsesTargetSelection() const override { return true; }
    virtual void Draw(const FKataDebugPanelContext& Context) override;
    virtual void OnPlaySessionStarted(APlayerController* PlayerController) override;

private:
    /** 마지막 버튼 동작의 결과. 패널 아래에 한 줄로 보인다. */
    FString LastResult;
};
