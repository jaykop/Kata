#pragma once

#include "Action/KataTask.h"
#include "CoreMinimal.h"
#include "Runtime/KataTaskInstance.h"
#include "KataTask_RotateToFacing.generated.h"

class AActor;
class UKataTargetingComponent;

/**
 * 타임라인 구간 동안 실행 주체를 공격 방향으로 일정한 속도로 돌리는 태스크.
 *
 * 방향은 실행 주체의 UKataTargetingComponent::ResolveFacingDirection이 이번 실행의 대상을 받아 정한다.
 * PC는 락온 대상 → 이동 입력 방향 → 대상 순서이고, 기반 컴포넌트는 대상 쪽이다.
 * 매 Tick Rotation Rate만큼만 Yaw를 더하므로 목표 방향에 도달하기 전에 구간이 끝나면 그 자리에서 멈춘다.
 * Pitch·Roll은 유지한다. 액션 시작 한 프레임에 바로 돌리려면 Resolve Facing Command를 쓴다.
 *
 * 컨트롤러 회전을 따르는 캐릭터(bUseControllerRotationYaw)는 컨트롤러가 매 프레임 회전을 덮어쓴다.
 */
UCLASS(meta = (DisplayName = "Kata Task: Rotate To Facing"))
class KATATARGETING_API UKataTask_RotateToFacing : public UKataTask
{
    GENERATED_BODY()

public:
    UKataTask_RotateToFacing();

    /** 초당 회전 각도. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rotation", meta = (ClampMin = "1.0", Units = "deg"))
    float RotationRate = 720.0f;

    /**
     * 켜면 매 Tick 방향을 다시 구해 움직이는 락온 대상과 바뀐 입력을 따라간다. 방향을 구하지 못한 Tick은 직전 목표를 유지한다.
     * 끄면 구간 시작에 한 번 구한 방향으로만 돌고, 도달하면 태스크를 끝낸다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rotation")
    bool bUpdateDirectionEveryTick = true;

    virtual TSubclassOf<UKataTaskInstance> GetTaskInstanceClass_Implementation() const override;
    virtual FName GetConfigurationError() const override;
    virtual FString DescribeConfigurationError(FName ErrorCode) const override;
};

/**
 * 방향 회전의 실행별 상태. 실행 주체와 컴포넌트, 현재 목표 Yaw를 보관한다.
 * 액터와 컴포넌트는 약한 참조로 두어 실행 중 파괴돼도 수명에 관여하지 않는다.
 */
UCLASS()
class KATATARGETING_API UKataTaskInstance_RotateToFacing : public UKataTaskInstance
{
    GENERATED_BODY()

protected:
    virtual void OnTaskStarted_Implementation() override;
    virtual void OnTaskTick_Implementation(float DeltaTime) override;
    virtual void OnTaskEnded_Implementation(EKataTaskEndReason Reason) override;

private:
    /** 컴포넌트에 방향을 물어 목표 Yaw를 갱신한다. 방향을 구하지 못하면 목표를 바꾸지 않고 false를 돌려준다. */
    bool UpdateTargetYaw();

    TWeakObjectPtr<AActor> RotatingActor;

    TWeakObjectPtr<const UKataTargetingComponent> Targeting;

    float TargetYaw = 0.0f;

    bool bHasTargetYaw = false;
};
