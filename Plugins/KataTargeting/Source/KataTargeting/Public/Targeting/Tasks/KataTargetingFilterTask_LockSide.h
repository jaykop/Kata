#pragma once

#include "CoreMinimal.h"
#include "Tasks/TargetingFilterTask_BasicFilterTemplate.h"
#include "KataTargetingFilterTask_LockSide.generated.h"

/** 락온 전환 방향. */
UENUM(BlueprintType)
enum class EKataLockSwitchSide : uint8
{
    Left,
    Right
};

/**
 * 현재 락온 대상을 기준으로 한쪽에 있는 후보만 남기는 Targeting 필터. 락온 전환 Preset에 넣는다.
 *
 * 기준은 실행 주체의 시점(플레이어 카메라)에서 락온 대상을 바라본 방향이며, 위에서 내려다본 수평면에서 좌우를 가린다.
 * 현재 락온 대상 자신은 항상 뺀다. 실행 주체에 UKataPlayerTargetingComponent가 없거나 락온 중이 아니면 방향으로 거르지 않는다.
 */
UCLASS(meta = (DisplayName = "Kata Filter Lock Side"))
class KATATARGETING_API UKataTargetingFilterTask_LockSide : public UTargetingFilterTask_BasicFilterTemplate
{
    GENERATED_BODY()

public:
    UKataTargetingFilterTask_LockSide(const FObjectInitializer& ObjectInitializer);

protected:
    virtual bool ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const override;

    /** 남길 방향. */
    UPROPERTY(EditAnywhere, Category = "Kata|Targeting")
    EKataLockSwitchSide Side = EKataLockSwitchSide::Right;
};
