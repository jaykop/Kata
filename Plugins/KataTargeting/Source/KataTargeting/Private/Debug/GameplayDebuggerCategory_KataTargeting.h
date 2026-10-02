#pragma once

#if WITH_GAMEPLAY_DEBUGGER

#include "CoreMinimal.h"
#include "GameplayDebuggerCategory.h"

/**
 * GameplayDebugger의 "KataTargeting" 카테고리.
 *
 * 디버그 대상 액터와 무관하게 로컬 플레이어 폰의 UKataPlayerTargetingComponent를 읽는다.
 * 락온 지점과 Lock On Preset이 지금 고를 수 있는 후보 지점을 우선순위와 함께 글로 보이고, 지점 위치에 구를 그린다.
 * 후보를 구하려고 수집 주기마다 Preset을 실행하므로 이 카테고리를 켠 동안에만 비용이 든다.
 */
class FGameplayDebuggerCategory_KataTargeting final : public FGameplayDebuggerCategory
{
public:
    FGameplayDebuggerCategory_KataTargeting();

    virtual void CollectData(APlayerController* OwnerPC, AActor* DebugActor) override;

    static TSharedRef<FGameplayDebuggerCategory> MakeInstance();
};

#endif // WITH_GAMEPLAY_DEBUGGER
