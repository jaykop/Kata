#pragma once

#if WITH_GAMEPLAY_DEBUGGER

#include "CoreMinimal.h"
#include "GameplayDebuggerCategory.h"
#include "KataCameraTypes.h"

/**
 * GameplayDebugger의 "KataCamera" 카테고리.
 *
 * 디버그 대상 액터와 무관하게 로컬 플레이어의 AKataPlayerCameraManager를 읽는다.
 * 적용 중인 카메라 데이터, 배치 방식, 회전·거리·FOV, Feature 실행 순서를 글로 보이고 피벗 위치에 점을 그린다.
 * Spline 레일과 배치 위치는 Canvas의 작은 XZ 투영으로 표시하므로 월드 DebugDraw 표시 여부에 의존하지 않는다.
 * 카메라 위치를 잇는 선은 플레이어 시야를 가리므로 그리지 않는다. Unpossess 후에는 GameplayDebugger의 월드 도형이 표시되지 않는 제한이 있다.
 */
class FGameplayDebuggerCategory_KataCamera final : public FGameplayDebuggerCategory
{
public:
    FGameplayDebuggerCategory_KataCamera();

    virtual void CollectData(APlayerController* OwnerPC, AActor* DebugActor) override;
    virtual void DrawData(APlayerController* OwnerPC, FGameplayDebuggerCanvasContext& CanvasContext) override;

    static TSharedRef<FGameplayDebuggerCategory> MakeInstance();

private:
    /** 수집한 포즈·곡선의 값 사본. DrawData는 UObject를 다시 조회하지 않는다. */
    FKataCameraDebugSnapshot CollectedSnapshot;
};

#endif // WITH_GAMEPLAY_DEBUGGER
