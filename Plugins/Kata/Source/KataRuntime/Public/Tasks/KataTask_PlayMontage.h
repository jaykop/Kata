#pragma once

#include "CoreMinimal.h"
#include "Action/KataTask.h"
#include "Runtime/KataTaskInstance.h"
#include "KataTask_PlayMontage.generated.h"

class UAnimInstance;
class UAnimMontage;

/** 태스크 실행 중 몽타주가 종료됐을 때 액션과 태스크를 어떻게 처리할지 정한다. */
UENUM(BlueprintType)
enum class EKataMontageEndPolicy : uint8
{
    /** 몽타주 종료와 관계없이 타임라인 시각을 그대로 진행한다. */
    ContinueTimeline,
    /** 몽타주가 끝나면 이 태스크를 완료 처리한다. 타임라인은 계속 진행한다. */
    FinishTaskOnMontageEnd,
    /** 몽타주가 자연 종료되면 액션을 Completed로, 재생이 중단되면 Interrupted로 종료한다. */
    EndKataOnMontageEnd
};

/**
 * 태스크가 시작될 때 몽타주를 재생한다.
 *
 * 몽타주의 자연 종료 시점과 Kata의 정상 완료 시점은 구분한다.
 * 몽타주 시간을 두 번째 타임라인 시계로 사용하지 않는다.
 * Start Section으로 시작 섹션을 지정할 수 있다. 재생 중 섹션 이동과 겹치는 재생을 자동으로 조정하지 않는다.
 */
UCLASS(meta = (DisplayName = "Kata Task: Play Montage", KataTaskCategory = "Animation"))
class KATARUNTIME_API UKataTask_PlayMontage : public UKataTask
{
    GENERATED_BODY()

public:
    UKataTask_PlayMontage();

    /** 재생할 몽타주. 실행 주체의 Avatar에 있는 스켈레탈 메시의 Anim Instance를 사용한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Montage")
    TObjectPtr<UAnimMontage> Montage;

    /** 재생 속도 배율. 1이면 원래 속도이며, 0보다 커야 한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Montage", meta = (ClampMin = "0.01"))
    float PlayRate = 1.0f;

    /** 비워 두면 몽타주의 기본 시작 지점에서 재생한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Montage")
    FName StartSection;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Montage")
    EKataMontageEndPolicy MontageEndPolicy = EKataMontageEndPolicy::ContinueTimeline;

    /** 태스크가 끝날 때 자신이 시작한 재생만 정지한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Montage")
    bool bStopMontageWhenTaskEnds = true;

    /** 태스크 종료로 재생을 정지할 때 사용할 블렌드 아웃 시간(초). 음수면 몽타주 설정을 따른다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Montage")
    float StopBlendOutTime = -1.0f;

    virtual TSubclassOf<UKataTaskInstance> GetTaskInstanceClass_Implementation() const override;
    virtual FName GetConfigurationError() const override;
    virtual FString DescribeConfigurationError(FName ErrorCode) const override;

#if WITH_EDITOR
    /** Montage를 변경하면 전체 길이를 PlayRate로 나눈 값을 Duration으로 제안한다. StartSection은 고려하지 않으며, Montage를 비우면 기존 Duration을 유지한다. */
    virtual bool GetAutoDuration(FName ChangedProperty, float& OutDuration) const override;
#endif
};

/**
 * 몽타주 재생의 실행별 상태.
 * 자신이 시작한 재생 인스턴스만 추적하고 정리한다.
 */
UCLASS()
class KATARUNTIME_API UKataTaskInstance_PlayMontage : public UKataTaskInstance
{
    GENERATED_BODY()

protected:
    virtual void OnTaskStarted_Implementation() override;
    virtual void OnTaskEnded_Implementation(EKataTaskEndReason Reason) override;

private:
    /** 몽타주 종료 델리게이트에서 호출하며, 설정한 종료 정책을 적용한다. */
    void HandleMontageEnded(UAnimMontage* EndedMontage, bool bInterrupted);

    /** Context의 Avatar에서 애니메이션 인스턴스를 찾는다. */
    UAnimInstance* ResolveAnimInstance() const;

    UPROPERTY(Transient)
    TObjectPtr<UAnimMontage> PlayingMontage;

    /** 자신이 시작한 재생인지 확인할 몽타주 인스턴스 ID. */
    int32 PlayingMontageInstanceId = INDEX_NONE;

    bool bMontageEndDelegateBound = false;
};
