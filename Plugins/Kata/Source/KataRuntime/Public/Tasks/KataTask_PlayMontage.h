#pragma once

#include "CoreMinimal.h"
#include "Action/KataTask.h"
#include "Runtime/KataTaskInstance.h"
#include "KataTask_PlayMontage.generated.h"

class UAnimInstance;
class UAnimMontage;

/** 몽타주가 먼저 끝났을 때 Kata 타임라인을 어떻게 다룰지 정한다. */
UENUM(BlueprintType)
enum class EKataMontageEndPolicy : uint8
{
    /** 몽타주 종료와 관계없이 타임라인 시각을 그대로 진행한다. */
    ContinueTimeline,
    /** 몽타주가 끝나면 이 태스크를 완료 처리한다. 타임라인은 계속 진행한다. */
    FinishTaskOnMontageEnd,
    /** 몽타주가 끝나면 Kata 인스턴스 전체를 중단한다. */
    EndKataOnMontageEnd
};

/**
 * 몽타주 재생 태스크 정의.
 *
 * 몽타주의 자연 종료 시점과 Kata의 정상 완료 시점은 구분한다.
 * 몽타주 시간을 두 번째 타임라인 시계로 사용하지 않는다.
 * 섹션 점프와 겹치는 재생의 자동 처리는 제공하지 않는다.
 */
UCLASS(meta = (DisplayName = "Kata Task: Play Montage"))
class KATARUNTIME_API UKataTask_PlayMontage : public UKataTask
{
    GENERATED_BODY()

public:
    UKataTask_PlayMontage();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Montage")
    TObjectPtr<UAnimMontage> Montage;

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

    /** 정지 시 사용할 블렌드 아웃 시간. 음수면 몽타주 설정을 따른다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Montage")
    float StopBlendOutTime = -1.0f;

    virtual TSubclassOf<UKataTaskInstance> GetTaskInstanceClass_Implementation() const override;
    virtual FName GetConfigurationError() const override;
    virtual FString DescribeConfigurationError(FName ErrorCode) const override;
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
    /** 재생이 끝나거나 블렌드 아웃할 때 호출한다. */
    void HandleMontageEnded(UAnimMontage* EndedMontage, bool bInterrupted);

    /** Context의 Avatar에서 애니메이션 인스턴스를 찾는다. */
    UAnimInstance* ResolveAnimInstance() const;

    UPROPERTY(Transient)
    TObjectPtr<UAnimMontage> PlayingMontage;

    /** 자신이 시작한 재생인지 확인할 몽타주 인스턴스 ID. */
    int32 PlayingMontageInstanceId = INDEX_NONE;

    bool bMontageEndDelegateBound = false;
};
