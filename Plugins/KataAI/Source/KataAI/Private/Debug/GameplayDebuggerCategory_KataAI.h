#pragma once

#if WITH_GAMEPLAY_DEBUGGER

#include "CoreMinimal.h"
#include "GameplayDebuggerCategory.h"

/**
 * GameplayDebugger의 "KataAI" 카테고리.
 *
 * 디버그 대상으로 고른 AI Pawn(또는 그 Controller)의 StateTree 실행 상태, 활성 상태 경로, 적용된 Linked 슬롯,
 * 현재 대상·기억 위치·Home·추격 한계, 이동 재시도 횟수, 실행 중인 Kata Action·Graph를 글로 보인다.
 * 대상선, 마지막 감지 위치, Home, 추격 한계 원은 월드 도형으로 그린다.
 * 값은 CollectData에서 읽기만 하며 AI 상태를 바꾸지 않는다.
 */
class FGameplayDebuggerCategory_KataAI final : public FGameplayDebuggerCategory
{
public:
    FGameplayDebuggerCategory_KataAI();

    virtual void CollectData(APlayerController* OwnerPC, AActor* DebugActor) override;

    static TSharedRef<FGameplayDebuggerCategory> MakeInstance();
};

#endif // WITH_GAMEPLAY_DEBUGGER
