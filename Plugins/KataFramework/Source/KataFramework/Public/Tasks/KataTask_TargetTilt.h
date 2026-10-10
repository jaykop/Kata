#pragma once

#include "Action/KataTask.h"
#include "CoreMinimal.h"
#include "Runtime/KataTaskInstance.h"
#include "KataTask_TargetTilt.generated.h"

class AActor;
class UKataTargetingComponent;
class UKataTiltComponent;

/**
 * 타임라인 구간 동안 실행 주체의 상체를 대상 쪽으로 기울여 공격 궤적이 대상에 닿게 하는 태스크.
 *
 * 원래 애니메이션 위에 "애니메이션이 가정한 조준 방향"과 "실제 대상 방향"의 Pitch 차이만 더한다.
 * 가정한 조준점은 나와 같은 지면에 Expected Target Height 높이의 대상이 섰을 때의 조준점이다. 그래서 평지에서 같은 체격이면 기울지 않고,
 * 경사면·락온 부위·체격 차이만큼 기운다. 실제 조준점은 실행 주체의 UKataTargetingComponent::ResolveAimLocation이 정하며,
 * 부위 지점(PC 락온 지점)이 아니면 대상 캡슐의 세로 구간에서 대상 발 기준 Expected Target Height에 가장 가까운 점이다.
 * 원점 높이와 기준 키는 실행 주체 메시의 UKataAnimInstance Class Defaults에서 읽는다.
 *
 * 대상은 실행 Context의 대상(Resolve Target Command 결과)이다. 대상이 없거나 자신이면 아무것도 하지 않고 끝난다.
 * 계산한 각도는 실행 주체의 UKataTiltComponent에 요청으로 넘기고, 포즈 적용은 ABP의 Kata Tilt 노드가 한다.
 * 몸 정면에서 수평 90도 넘게 벗어난 대상에는 기울이지 않는다. 몸 전체의 방향은 Rotate To Facing이나 Resolve Facing이 맡는다.
 * 구간이 끝나거나 취소되면 컴포넌트가 마지막 각도를 유지한 채 Blend Out Time 동안 풀어 준다.
 */
UCLASS(meta = (DisplayName = "Kata Task: Target Tilt", KataTaskCategory = "Animation"))
class KATAFRAMEWORK_API UKataTask_TargetTilt : public UKataTask
{
    GENERATED_BODY()

public:
    UKataTask_TargetTilt();

    /** 구간 시작부터 Tilt가 다 적용되기까지의 시간. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tilt", meta = (ClampMin = "0.0", Units = "s"))
    float BlendInTime = 0.1f;

    /** 구간이 끝나거나 액션이 취소된 뒤 Tilt가 다 풀리기까지의 시간. 이 시간은 구간 밖으로 이어진다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tilt", meta = (ClampMin = "0.0", Units = "s"))
    float BlendOutTime = 0.2f;

    /** 켜면 구간 끝까지 매 Tick 각도를 다시 구해 움직이는 대상을 따라간다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tilt")
    bool bTrackUntilEnd = true;

    /**
     * Track Until End를 끄면 구간 시작부터 이 시간 동안만 각도를 다시 구하고, 이후에는 마지막 각도로 고정한다.
     * 휘두르는 동안에도 대상을 따라가면 회피가 성립하지 않으므로, 공격이 닿기 전에 추적을 끝내는 데 쓴다. 0이면 시작 때 한 번만 구한다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tilt", meta = (ClampMin = "0.0", Units = "s", EditCondition = "!bTrackUntilEnd"))
    float TrackDuration = 0.2f;

    /** 적용 각도가 목표 각도로 다가가는 초당 최대 각도. 대상이 갑자기 움직여도 자세가 튀지 않게 한다. 0이면 제한하지 않는다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tilt", meta = (ClampMin = "0.0", Units = "deg"))
    float MaxPitchSpeed = 360.0f;

    /** 위로 기울일 수 있는 최대 각도. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tilt", meta = (ClampMin = "0.0", ClampMax = "90.0", Units = "deg"))
    float MaxPitchUp = 30.0f;

    /** 아래로 기울일 수 있는 최대 각도. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tilt", meta = (ClampMin = "0.0", ClampMax = "90.0", Units = "deg"))
    float MaxPitchDown = 30.0f;

    /** 각도 계산에 쓰는 최소 수평 거리. 대상이 바로 위나 아래에 있을 때 작은 높이 차이로 각도가 커지는 것을 막는다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tilt", meta = (ClampMin = "0.0", Units = "cm"))
    float MinHorizontalDistance = 50.0f;

    /** 켜면 이 액션에서만 Anim Instance의 Expected Target Height 대신 Expected Target Height 값을 쓴다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tilt")
    bool bOverrideExpectedTargetHeight = false;

    /** Override Expected Target Height를 켰을 때 쓸 기준 키(대상 발 기준). 내려찍기처럼 가정한 조준점이 다른 공격에 쓴다. 0이면 자기 캡슐 절반 높이다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tilt", meta = (ClampMin = "0.0", Units = "cm", EditCondition = "bOverrideExpectedTargetHeight"))
    float ExpectedTargetHeight = 0.0f;

    virtual TSubclassOf<UKataTaskInstance> GetTaskInstanceClass_Implementation() const override;
    virtual FName GetConfigurationError() const override;
    virtual FString DescribeConfigurationError(FName ErrorCode) const override;
};

/**
 * 대상 방향 Tilt의 실행별 상태. 실행 주체·대상·컴포넌트와 Tilt 컴포넌트에 등록한 요청 핸들을 가진다.
 * 액터와 컴포넌트는 약한 참조로 두어 실행 중 파괴돼도 수명에 관여하지 않는다.
 */
UCLASS()
class KATAFRAMEWORK_API UKataTaskInstance_TargetTilt : public UKataTaskInstance
{
    GENERATED_BODY()

protected:
    virtual void OnTaskStarted_Implementation() override;
    virtual void OnTaskTick_Implementation(float DeltaTime) override;
    virtual void OnTaskEnded_Implementation(EKataTaskEndReason Reason) override;

private:
    /**
     * 지금 위치로 목표 Pitch(도, 위쪽 양수)를 구한다. 실행 주체나 대상이 없으면 false다.
     * bDrawDebug가 true면 Kata.Tilt.Debug 표시(조준 원점, 가정·실제 조준점, 목표 각도)를 이번 프레임에 그린다.
     */
    bool ComputeTargetPitch(float& OutPitch, bool bDrawDebug = false) const;

    TWeakObjectPtr<AActor> TiltActor;

    TWeakObjectPtr<AActor> TargetActor;

    TWeakObjectPtr<const UKataTargetingComponent> Targeting;

    TWeakObjectPtr<UKataTiltComponent> TiltComponent;

    int32 TiltHandle = INDEX_NONE;

    /** 발에서 조준 원점까지의 높이. 0이면 자기 캡슐 절반 높이다. */
    float AimOriginHeight = 0.0f;

    /** 애니메이션이 가정한 대상의 조준 높이. 0이면 자기 캡슐 절반 높이다. */
    float ExpectedTargetHeight = 0.0f;

    /** 구간 시작부터 지난 시간. Track Duration과 비교한다. */
    float TrackElapsed = 0.0f;
};
