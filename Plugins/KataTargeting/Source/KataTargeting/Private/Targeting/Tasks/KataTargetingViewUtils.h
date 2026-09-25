#pragma once

#include "CoreMinimal.h"
#include "Types/TargetingSystemTypes.h"

class AActor;

namespace KataTargetingView
{
    /** 요청의 실행 주체. 소스 Context가 없으면 nullptr이다. */
    AActor* GetSourceActor(const FTargetingRequestHandle& TargetingHandle);

    /**
     * 실행 주체의 시점. 플레이어가 조종하는 폰이면 플레이어 카메라 시점을, 그 밖에는 액터의 눈 시점을 쓴다.
     * @return 시점을 구했으면 true.
     */
    bool GetViewPoint(const AActor* SourceActor, FVector& OutLocation, FRotator& OutRotation);
}
