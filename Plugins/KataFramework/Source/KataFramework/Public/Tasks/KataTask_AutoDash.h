#pragma once

#include "Action/KataTask.h"
#include "CoreMinimal.h"
#include "Runtime/KataTaskInstance.h"
#include "KataTask_AutoDash.generated.h"

class UKataRootMotionCurveComponent;

/**
 * 타임라인 구간 동안 루트 모션 전진 거리를 대상까지의 거리에 맞춰 늘이거나 줄이는 태스크.
 *
 * 대상은 실행 Context의 대상(Resolve Target Command 결과)이다. 거리는 전진 제한(Limit Approach)과 같은 기준으로,
 * 대상 HurtBox 중 표면이 자신 캡슐 축에 가장 가까운 점까지 잰다. 대상에 HurtBox가 없으면 실행 주체의
 * UKataTargetingComponent::ResolveApproachLocation이 정한 위치를 쓴다. PC는 그 대상에 락온 중이면 락온 지점, 아니면 대상 위치다.
 * 실제 보정은 실행 주체의 UKataRootMotionCurveComponent가 루트 모션 처리 2단계에서 한다.
 *
 * 구간 동안의 전진 거리가 "자신의 캡슐 표면에서 대상 기준까지 Stop Distance를 남긴 거리"가 되도록,
 * 원래 루트 모션의 수평 이동을 늘이거나 줄인다. 총 거리는 Min Dash Distance~Max Dash Distance로 제한한다.
 * 몸이 대상에서 틀어져 있으면 지금 향한 경로 위에서 대상에 가장 가까워지는 지점까지만 가고, 90도 넘게 틀어져 있으면 보정하지 않는다.
 * 이동 속도의 분포는 애니메이션을 따른다. 방향은 바꾸지 않으므로 대상을 향하게 하려면 Rotate To Facing이나 Resolve Facing을 함께 쓴다.
 * 구간 길이는 처음 만난 루트 모션 몽타주의 재생 속도로 몽타주 시간으로 바뀐다.
 * 대상이나 컴포넌트가 없으면 아무것도 하지 않고 끝나며, 전진 루트 모션이 없는 구간은 보정하지 않는다.
 */
UCLASS(meta = (DisplayName = "Kata Task: Auto Dash", KataTaskCategory = "Movement"))
class KATAFRAMEWORK_API UKataTask_AutoDash : public UKataTask
{
    GENERATED_BODY()

public:
    UKataTask_AutoDash();

    /**
     * 자신의 캡슐 표면과 대상 기준 사이에 남길 간격. 대상 HurtBox가 있으면 그 표면까지의 간격이다.
     * HurtBox가 없으면 대상 위치 기준일 때 대상 캡슐 표면까지, 락온 지점 기준일 때 지점까지의 간격이다.
     * 같은 공격에 Limit Approach를 쓰면 실제로 멈추는 간격은 이 값과 Limit Distance 중 큰 값이다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Auto Dash", meta = (ClampMin = "0.0", Units = "cm"))
    float StopDistance = 50.0f;

    /**
     * 구간 동안 보정해 이동할 수평 거리 총량의 하한. 대상이 더 가까워도 이만큼은 나아간다.
     * 0이면 이미 대상 앞에 있을 때 그 자리에서 휘두른다. 대상보다 멀리 잡으면 대상에게 파고들 수 있다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Auto Dash", meta = (ClampMin = "0.0", Units = "cm"))
    float MinDashDistance = 0.0f;

    /** 구간 동안 보정해 이동할 수평 거리 총량의 상한. 대상이 더 멀면 이 거리만 가고 멈춘다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Auto Dash", meta = (ClampMin = "0.0", Units = "cm"))
    float MaxDashDistance = 500.0f;

    /** 켜면 매 이동 갱신 대상 위치를 다시 구해 움직이는 대상을 따라간다. 끄면 처음 구한 위치로 고정한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Auto Dash")
    bool bTrackTarget = true;

    virtual TSubclassOf<UKataTaskInstance> GetTaskInstanceClass_Implementation() const override;
    virtual FName GetConfigurationError() const override;
    virtual FString DescribeConfigurationError(FName ErrorCode) const override;
};

/**
 * 오토 대시의 실행별 상태. 루트 모션 컴포넌트에 등록한 거리 보정 요청의 핸들만 가진다.
 * 컴포넌트는 약한 참조로 두어 실행 중 파괴돼도 수명에 관여하지 않는다.
 */
UCLASS()
class KATAFRAMEWORK_API UKataTaskInstance_AutoDash : public UKataTaskInstance
{
    GENERATED_BODY()

protected:
    virtual void OnTaskStarted_Implementation() override;
    virtual void OnTaskEnded_Implementation(EKataTaskEndReason Reason) override;

private:
    TWeakObjectPtr<UKataRootMotionCurveComponent> RootMotionComponent;

    int32 CorrectionHandle = INDEX_NONE;
};
