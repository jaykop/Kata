#pragma once

#include "Action/KataTask.h"
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "KataRuntimeTypes.h"
#include "Runtime/KataTaskInstance.h"
#include "KataTask_ApplyLooseTag.generated.h"

class UAbilitySystemComponent;

/**
 * 타임라인의 한 구간 동안 Loose Gameplay Tag를 붙여 두는 태스크.
 *
 * "이 구간 동안 이 상태다"를 표시하기 위한 태스크이므로 구간이 끝나면 반드시 태그를 회수한다.
 * 태그 하나를 붙이려고 Gameplay Effect 에셋을 만들지 않아도 되도록 GE 적용과 분리해 둔다.
 * 지속되는 Attribute 변경이나 스택이 필요하면 이 태스크가 아니라 Apply Gameplay Effect를 쓴다.
 */
UCLASS(meta = (DisplayName = "Kata Task: Apply Loose Tag"))
class KATARUNTIME_API UKataTask_ApplyLooseTag : public UKataTask
{
    GENERATED_BODY()

public:
    UKataTask_ApplyLooseTag();

    /** 구간 동안 붙여 둘 태그. 비어 있으면 설정 오류다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tag")
    FGameplayTagContainer Tags;

    /** 태그를 받을 대상. 대상의 ASC에 붙인다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tag")
    EKataTaskTargetSource TagTarget = EKataTaskTargetSource::Avatar;

    virtual TSubclassOf<UKataTaskInstance> GetTaskInstanceClass_Implementation() const override;
    virtual FName GetConfigurationError() const override;
    virtual FString DescribeConfigurationError(FName ErrorCode) const override;
};

/**
 * 태그 부착의 실행별 상태.
 *
 * Loose Tag는 참조 카운트로 관리되므로 자신이 붙인 태그와 붙인 대상을 기억했다가
 * 같은 대상에서 같은 수만큼만 뺀다. 종료 사유와 무관하게 한 번만 회수한다.
 */
UCLASS()
class KATARUNTIME_API UKataTaskInstance_ApplyLooseTag : public UKataTaskInstance
{
    GENERATED_BODY()

protected:
    virtual void OnTaskStarted_Implementation() override;
    virtual void OnTaskEnded_Implementation(EKataTaskEndReason Reason) override;

private:
    /** 시작에서 실제로 붙인 태그. 설정이 아니라 이 값을 기준으로 회수한다. */
    FGameplayTagContainer AppliedTags;

    /** 태그를 붙인 ASC. 회수도 같은 대상에서 해야 한다. */
    TWeakObjectPtr<UAbilitySystemComponent> TargetAbilitySystem;
};
