#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManagerDefines.h"

class AActor;

/**
 * KataAI 치트가 켠 상태를 인지 처리 코드가 읽는 모듈 내부 창구다.
 * Shipping 구성에서는 항상 false를 돌려주는 빈 함수가 되어 인지 경로에 비용을 남기지 않는다.
 */
namespace KataAICheat
{
#if UE_WITH_CHEAT_MANAGER
    /** 액터를 AI 인지에서 숨기거나 되돌린다. 파괴된 액터의 기록은 다음 호출 때 정리된다. */
    void SetHiddenFromAI(const AActor* Actor, bool bHidden);

    /** 액터가 숨김 상태면 true. KataAI 타게팅이 시각 결과를 반영하기 전에 확인한다. */
    bool IsHiddenFromAI(const AActor* Actor);
#else
    inline bool IsHiddenFromAI(const AActor*)
    {
        return false;
    }
#endif
}
