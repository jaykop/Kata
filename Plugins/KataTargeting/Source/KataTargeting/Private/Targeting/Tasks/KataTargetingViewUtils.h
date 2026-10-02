#pragma once

#include "CoreMinimal.h"
#include "Types/TargetingSystemTypes.h"

class AActor;
class UKataTargetPointComponent;

namespace KataTargetingView
{
    /** 결과가 타겟 지점이면 그 지점. Kata Expand Target Points를 거치지 않은 액터 결과면 nullptr이다. */
    UKataTargetPointComponent* GetTargetPoint(const FTargetingDefaultResultData& TargetData);

    /** 판정에 쓸 결과 위치. 타겟 지점이면 지점 위치, 아니면 액터 위치다. 둘 다 없으면 false다. */
    bool GetTargetLocation(const FTargetingDefaultResultData& TargetData, FVector& OutLocation);

    /** 요청의 실행 주체. 소스 Context가 없으면 nullptr이다. */
    AActor* GetSourceActor(const FTargetingRequestHandle& TargetingHandle);

    /**
     * 실행 주체의 시점. 플레이어가 조종하는 폰이면 플레이어 카메라 시점을, 그 밖에는 액터의 눈 시점을 쓴다.
     * @return 시점을 구했으면 true.
     */
    bool GetViewPoint(const AActor* SourceActor, FVector& OutLocation, FRotator& OutRotation);
}
