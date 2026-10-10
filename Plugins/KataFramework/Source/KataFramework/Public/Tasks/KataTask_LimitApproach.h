#pragma once

#include "Action/KataTask.h"
#include "CoreMinimal.h"
#include "Runtime/KataTaskInstance.h"
#include "KataTask_LimitApproach.generated.h"

class UKataRootMotionCurveComponent;
class UTargetingPreset;

/**
 * 타임라인 구간 동안 루트 모션이 대상 HurtBox 표면에서 Limit Distance 안으로 들어가지 못하게 하는 태스크.
 *
 * 기준은 후보 HurtBox 중 표면이 자신 캡슐 축에 가장 가까운 점이다. 후보는 실행 주체가 PC이고 락온 중이 아니면(소프트락)
 * Soft Lock Hurt Box Preset으로 고르고, 락온 중이거나 AI이면 실행 Context 대상의 HurtBox를 쓴다.
 * Preset이 비었거나 후보를 내지 못하면 대상의 HurtBox, 그것도 없으면 대상의 캡슐 표면을 쓴다. 대상도 없으면 아무것도 하지 않는다.
 * 후보는 구간 시작에 정하고, 표면 점은 매 이동 갱신 다시 구한다.
 *
 * 애니메이션 루트 모션(커브 대체·오토 대시 결과 포함)만 제한한다. 기준점 쪽 성분만 자르고 옆으로 도는 이동은 남기며,
 * 이미 간격 안이면 기준점 쪽 전진만 막고 밀어내지 않는다. 실제 처리는 실행 주체의 UKataRootMotionCurveComponent가 3단계에서 한다.
 * 오토 대시와 같은 기준에서 재므로, 같은 공격에 쓰면 실제로 멈추는 간격은 Stop Distance와 Limit Distance 중 큰 값이다.
 */
UCLASS(meta = (DisplayName = "Kata Task: Limit Approach", KataTaskCategory = "Movement"))
class KATAFRAMEWORK_API UKataTask_LimitApproach : public UKataTask
{
    GENERATED_BODY()

public:
    UKataTask_LimitApproach();

    /** 자신의 캡슐 표면과 대상 HurtBox 표면 사이에 남길 최소 간격. 오토 대시의 Stop Distance보다 작게 두면 대시는 Stop Distance에서 멈추고, 이 값은 그 뒤의 밀림만 막는다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Limit Approach", meta = (ClampMin = "0.0", Units = "cm"))
    float LimitDistance = 10.0f;

    /**
     * 소프트락(PC가 락온 중이 아님)일 때 후보 HurtBox를 고를 Targeting Preset. 구간 시작에 한 번 실행한다.
     * 결과가 HurtBox이면 그대로, 액터이면 그 액터의 HurtBox 전부를 후보로 쓴다. 비우면 대상의 HurtBox를 쓴다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Limit Approach")
    TObjectPtr<UTargetingPreset> SoftLockHurtBoxPreset;

    virtual TSubclassOf<UKataTaskInstance> GetTaskInstanceClass_Implementation() const override;
    virtual FName GetConfigurationError() const override;
    virtual FString DescribeConfigurationError(FName ErrorCode) const override;
};

/**
 * 전진 제한의 실행별 상태. 루트 모션 컴포넌트에 등록한 요청의 핸들만 가진다.
 * 후보 HurtBox와 대상은 요청이 약한 참조로 갖는다.
 */
UCLASS()
class KATAFRAMEWORK_API UKataTaskInstance_LimitApproach : public UKataTaskInstance
{
    GENERATED_BODY()

protected:
    virtual void OnTaskStarted_Implementation() override;
    virtual void OnTaskEnded_Implementation(EKataTaskEndReason Reason) override;

private:
    TWeakObjectPtr<UKataRootMotionCurveComponent> RootMotionComponent;

    int32 LimitHandle = INDEX_NONE;
};
