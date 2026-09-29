#pragma once

#if WITH_GAMEPLAY_DEBUGGER

#include "CoreMinimal.h"
#include "GameplayDebuggerCategory.h"

/**
 * GameplayDebugger의 "KataCamera" 카테고리.
 *
 * 디버그 대상 액터와 무관하게 로컬 플레이어의 AKataPlayerCameraManager를 읽는다.
 * 적용 중인 카메라 데이터, 배치 방식, 회전·거리·FOV, Feature 실행 순서를 글로만 보인다.
 * 월드 도형은 그리지 않는다. 카메라 위치를 잇는 선은 카메라 시점에서 시야를 가리고, 관전 모드에서는 표시되지 않아 확인 수단이 되지 못한다.
 */
class FGameplayDebuggerCategory_KataCamera final : public FGameplayDebuggerCategory
{
public:
    FGameplayDebuggerCategory_KataCamera();

    virtual void CollectData(APlayerController* OwnerPC, AActor* DebugActor) override;

    static TSharedRef<FGameplayDebuggerCategory> MakeInstance();
};

#endif // WITH_GAMEPLAY_DEBUGGER
