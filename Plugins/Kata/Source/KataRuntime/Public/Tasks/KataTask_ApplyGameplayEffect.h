#pragma once

#include "Action/KataTask.h"
#include "ActiveGameplayEffectHandle.h"
#include "CoreMinimal.h"
#include "KataRuntimeTypes.h"
#include "Runtime/KataTaskInstance.h"
#include "Templates/SubclassOf.h"
#include "KataTask_ApplyGameplayEffect.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;

/** 태스크가 끝날 때 적용한 Gameplay Effect를 어떻게 다룰지 정한다. */
UENUM(BlueprintType)
enum class EKataEffectRemovePolicy : uint8
{
    /** GE 자체의 지속 시간을 따른다. 태스크가 끝나도 제거하지 않는다. */
    UseEffectDuration,
    /** 태스크 구간이 끝나면 적용한 GE를 제거한다. Instant GE에는 제거할 대상이 없다. */
    RemoveOnTaskEnd
};

/**
 * 타임라인의 한 구간에 Gameplay Effect를 적용하는 태스크.
 *
 * 비용·쿨다운·Attribute 계산은 GAS가 담당하며 이 태스크는 적용 시점과 회수 시점만 정한다.
 * GE 자체의 지속 시간과 태스크 구간은 서로 다른 시계이므로 Remove Policy로 둘 중 무엇을 따를지 고른다.
 */
UCLASS(meta = (DisplayName = "Kata Task: Apply Gameplay Effect"))
class KATARUNTIME_API UKataTask_ApplyGameplayEffect : public UKataTask
{
    GENERATED_BODY()

public:
    /** 적용할 Gameplay Effect 클래스. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
    TSubclassOf<UGameplayEffect> EffectClass;

    /** 효과를 받을 대상. 대상의 ASC에 적용한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
    EKataTaskTargetSource EffectTarget = EKataTaskTargetSource::Avatar;

    /** 적용할 효과 레벨. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
    float EffectLevel = 1.0f;

    /** 태스크 구간이 끝났을 때의 처리 방식. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
    EKataEffectRemovePolicy RemovePolicy = EKataEffectRemovePolicy::UseEffectDuration;

    virtual TSubclassOf<UKataTaskInstance> GetTaskInstanceClass_Implementation() const override;
    virtual FName GetConfigurationError() const override;
    virtual FString DescribeConfigurationError(FName ErrorCode) const override;
};

/**
 * 효과 적용의 실행별 상태.
 * 자신이 적용한 핸들만 기억했다가 종료에서 한 번만 제거한다.
 */
UCLASS()
class KATARUNTIME_API UKataTaskInstance_ApplyGameplayEffect : public UKataTaskInstance
{
    GENERATED_BODY()

protected:
    virtual void OnTaskStarted_Implementation() override;
    virtual void OnTaskEnded_Implementation(EKataTaskEndReason Reason) override;

private:
    /** 자신이 적용한 효과의 핸들. Instant GE는 남는 핸들이 없어 무효로 남는다. */
    FActiveGameplayEffectHandle AppliedHandle;

    /** 핸들이 살아 있는 ASC. 적용한 쪽이 아니라 받은 쪽이므로 따로 보관한다. */
    TWeakObjectPtr<UAbilitySystemComponent> TargetAbilitySystem;
};
