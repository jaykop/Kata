#pragma once

#include "CoreMinimal.h"
#include "DrawDebugHelpers.h"

#if ENABLE_DRAW_DEBUG
/**
 * 대상 방향 Tilt 디버그 콘솔 변수를 모듈 안에서 읽는 함수. 콘솔 변수는 KataTiltComponent.cpp에 정의한다.
 * Shipping처럼 디버그 그리기가 빠지는 빌드에서는 선언되지 않으므로 호출부도 ENABLE_DRAW_DEBUG로 감싼다.
 */
namespace KataTiltDebug
{
    /** Kata.Tilt.Debug가 켜져 있으면 true다. */
    bool IsDrawEnabled();

    /** Kata.Tilt.ForcePitch가 0이 아니면 그 값(도, 위쪽 양수)을 OutPitch에 넣고 true를 돌려준다. */
    bool GetForcedPitch(float& OutPitch);
}
#endif
