#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Tasks/TargetingFilterTask_BasicFilterTemplate.h"
#include "KataTargetingFilterTask_LineOfSight.generated.h"

/**
 * 실행 주체의 시점에서 후보 위치까지 막는 물체가 있으면 거르는 Targeting 필터.
 * 시점은 플레이어가 조종하는 폰이면 플레이어 카메라, 그 밖에는 액터의 눈 시점이다.
 * 후보 위치는 Kata Expand Target Points를 거친 결과면 지점 위치, 아니면 액터 위치다.
 * 실행 주체와 후보 액터, 두 액터에 붙은 액터는 가림으로 보지 않는다.
 * 후보마다 라인 트레이스를 한 번 하므로 비용이 큰 다른 필터보다 뒤에 둔다.
 */
UCLASS(meta = (DisplayName = "Kata Filter Line Of Sight"))
class KATATARGETING_API UKataTargetingFilterTask_LineOfSight : public UTargetingFilterTask_BasicFilterTemplate
{
    GENERATED_BODY()

public:
    UKataTargetingFilterTask_LineOfSight(const FObjectInitializer& ObjectInitializer);

protected:
    virtual bool ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const override;

    /** 가림을 판정할 트레이스 채널. 이 채널을 Block하는 물체가 시점과 후보 사이에 있으면 후보에서 뺀다. */
    UPROPERTY(EditAnywhere, Category = "Kata|Line Of Sight")
    TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;
};
