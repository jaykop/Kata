#pragma once

#if WITH_GAMEPLAY_DEBUGGER

#include "CoreMinimal.h"
#include "GameplayDebuggerCategory.h"

/**
 * GameplayDebugger의 "KataCamera" 카테고리.
 *
 * 디버그 대상 액터와 무관하게 로컬 플레이어의 AKataPlayerCameraManager를 읽는다.
 * 적용 중인 카메라 데이터, 배치 방식, 피벗·회전·FOV, Feature 실행 순서를 글로 보이고 피벗과 카메라 위치를 월드에 그린다.
 * 카메라 자신의 시점에서는 월드 표시가 가려지므로 GameplayDebugger 관전 모드에서 확인한다.
 */
class FGameplayDebuggerCategory_KataCamera final : public FGameplayDebuggerCategory
{
public:
    FGameplayDebuggerCategory_KataCamera();

    virtual void CollectData(APlayerController* OwnerPC, AActor* DebugActor) override;

    static TSharedRef<FGameplayDebuggerCategory> MakeInstance();
};

#endif // WITH_GAMEPLAY_DEBUGGER
