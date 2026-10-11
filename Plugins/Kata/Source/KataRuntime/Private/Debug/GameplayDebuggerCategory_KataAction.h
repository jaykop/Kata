#pragma once

#include "Runtime/KataActionTrace.h"

#if WITH_GAMEPLAY_DEBUGGER && KATA_WITH_ACTION_TRACE

#include "CoreMinimal.h"
#include "GameplayDebuggerCategory.h"

/**
 * GameplayDebugger의 "KataAction" 카테고리.
 *
 * 디버그 대상 액터의 UKataActionComponent에서 현재 액션과 최근 실행 이력을 글로 보인다.
 * 대상을 고르지 않았으면 로컬 플레이어 Pawn을 쓴다. 화면에는 최근 기록 일부만 보이며 전체는 Kata.ActionTrace.Dump로 로그에 꺼낸다.
 * 그래프의 현재 노드처럼 KataGraph만 아는 상태는 보이지 않고, 기록에 남은 그래프 이벤트만 보인다.
 */
class FGameplayDebuggerCategory_KataAction final : public FGameplayDebuggerCategory
{
public:
    FGameplayDebuggerCategory_KataAction();

    virtual void CollectData(APlayerController* OwnerPC, AActor* DebugActor) override;

    static TSharedRef<FGameplayDebuggerCategory> MakeInstance();

private:
    /** 매 수집마다 저장 공간을 다시 잡지 않도록 재사용하는 기록 사본. */
    TArray<FKataActionTraceEntry> CollectedEntries;
};

#endif // WITH_GAMEPLAY_DEBUGGER && KATA_WITH_ACTION_TRACE
