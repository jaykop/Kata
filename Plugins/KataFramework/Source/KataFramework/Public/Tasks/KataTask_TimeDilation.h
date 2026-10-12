#pragma once

#include "Action/KataTask.h"
#include "CoreMinimal.h"
#include "Runtime/KataTaskInstance.h"
#include "KataTask_TimeDilation.generated.h"

class UKataTimeDilationSubsystem;

/** Time Dilation 태스크가 배율을 요청할 층. */
UENUM(BlueprintType)
enum class EKataTimeDilationTarget : uint8
{
    /** 이 액션을 실행하는 액터(실행 컴포넌트의 소유 액터). 액션 타임라인도 같은 시간 배율로 진행한다. */
    Self,
    /** 월드 전체. 월드 시간 배율을 따르는 액터와 이 액션의 타임라인에 적용한다. */
    World
};

/**
 * 타임라인 구간 동안 시간 배율을 요청하는 태스크.
 *
 * 구간이 시작될 때 UKataTimeDilationSubsystem에 요청하고, 완료·취소·중단·소유자 파괴 등 어떤 이유로 끝나도 요청을 푼다.
 * 구간 길이는 Kata 시각으로 잰다. 대상이 Self나 World이면 액션 타임라인도 함께 느려지므로,
 * 배율 0.5에 Duration 0.2초인 구간은 실제로 약 0.4초 동안 유지된다.
 * 같은 층의 다른 요청과는 Priority로 겨루며, 진 동안에는 이 태스크의 배율이 적용되지 않는다.
 */
UCLASS(meta = (DisplayName = "Kata Task: Time Dilation", KataTaskCategory = "Time"))
class KATAFRAMEWORK_API UKataTask_TimeDilation : public UKataTask
{
    GENERATED_BODY()

public:
    UKataTask_TimeDilation();

    /** 배율을 바꿀 층. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Time Dilation")
    EKataTimeDilationTarget Target = EKataTimeDilationTarget::Self;

    /**
     * 요청할 시간 배율. 1보다 작으면 느려지고 크면 빨라진다.
     * 에디터 입력 하한은 0.01이며 런타임에서 이 하한을 강제하지 않는다.
     * 구간 끝을 Kata 시각으로 재므로 Self 배율이 0이면 타임라인이 멈춰 구간도 끝나지 않는다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Time Dilation", meta = (ClampMin = "0.01", UIMin = "0.01", UIMax = "2.0"))
    float TimeDilation = 0.5f;

    /** 같은 층의 다른 요청과 겨룰 우선순위. 값이 클수록 우선한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Time Dilation")
    int32 Priority = 0;

    virtual TSubclassOf<UKataTaskInstance> GetTaskInstanceClass_Implementation() const override;
    virtual FName GetConfigurationError() const override;
    virtual FString DescribeConfigurationError(FName ErrorCode) const override;
};

/** 시간 배율 태스크의 실행별 상태. 요청을 받은 Subsystem과 핸들만 가진다. */
UCLASS()
class KATAFRAMEWORK_API UKataTaskInstance_TimeDilation : public UKataTaskInstance
{
    GENERATED_BODY()

protected:
    virtual void OnTaskStarted_Implementation() override;
    virtual void OnTaskEnded_Implementation(EKataTaskEndReason Reason) override;

private:
    TWeakObjectPtr<UKataTimeDilationSubsystem> TimeDilationSubsystem;

    int32 RequestHandle = INDEX_NONE;
};
