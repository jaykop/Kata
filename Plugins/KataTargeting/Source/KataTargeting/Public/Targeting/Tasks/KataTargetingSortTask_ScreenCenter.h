#pragma once

#include "CoreMinimal.h"
#include "Targeting/Tasks/KataTargetingSortTask_Weighted.h"
#include "KataTargetingSortTask_ScreenCenter.generated.h"

/**
 * 실행 주체의 시선 중앙에 가까운 후보를 앞세우는 정렬. 원점수는 시선과 후보 방향 사이의 각도(도)다.
 * 플레이어가 조종하면 플레이어 카메라를, 그 밖에는 액터의 눈 시점을 기준으로 삼는다.
 */
UCLASS(meta = (DisplayName = "Kata Sort Screen Center"))
class KATATARGETING_API UKataTargetingSortTask_ScreenCenter : public UKataTargetingSortTask_Weighted
{
    GENERATED_BODY()

public:
    UKataTargetingSortTask_ScreenCenter(const FObjectInitializer& ObjectInitializer);

protected:
    virtual float GetRawScore(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const override;
};
