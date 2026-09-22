#include "Tasks/KataTask_ApplyLooseTag.h"

#include "AbilitySystemComponent.h"
#include "KataRuntimeLog.h"

UKataTask_ApplyLooseTag::UKataTask_ApplyLooseTag()
{
    // 태그는 구간을 차지해야 의미가 있다. 기본값으로 짧은 구간을 준다.
    Duration = 0.2f;
}

TSubclassOf<UKataTaskInstance> UKataTask_ApplyLooseTag::GetTaskInstanceClass_Implementation() const
{
    return UKataTaskInstance_ApplyLooseTag::StaticClass();
}

FName UKataTask_ApplyLooseTag::GetConfigurationError() const
{
    const FName SuperError = Super::GetConfigurationError();
    if (!SuperError.IsNone())
    {
        return SuperError;
    }
    if (Tags.IsEmpty())
    {
        return TEXT("MissingTags");
    }
    if (bSingleFrame)
    {
        // 한 프레임 태그는 붙자마자 떨어져 다른 어떤 처리도 관측하지 못한다.
        return TEXT("SingleFrameTag");
    }
    if (!(Duration > 0.0f))
    {
        return TEXT("ZeroLengthTag");
    }
    return NAME_None;
}

FString UKataTask_ApplyLooseTag::DescribeConfigurationError(FName ErrorCode) const
{
    if (ErrorCode == TEXT("MissingTags"))
    {
        return TEXT("'Tags' must contain at least one tag");
    }
    if (ErrorCode == TEXT("SingleFrameTag"))
    {
        return TEXT("'Single Frame' cannot be used for a loose tag; give it a duration instead");
    }
    if (ErrorCode == TEXT("ZeroLengthTag"))
    {
        return TEXT("'Duration' must be greater than zero or the tag is removed as soon as it is added");
    }
    return Super::DescribeConfigurationError(ErrorCode);
}

void UKataTaskInstance_ApplyLooseTag::OnTaskStarted_Implementation()
{
    const UKataTask_ApplyLooseTag* Definition = Cast<UKataTask_ApplyLooseTag>(GetTaskDefinition());
    if (Definition == nullptr || Definition->Tags.IsEmpty())
    {
        FinishTask();
        return;
    }

    const FKataContext Context = GetKataContext();
    UAbilitySystemComponent* ReceivingAbilitySystem = Context.ResolveAbilitySystemFor(Definition->TagTarget);
    if (ReceivingAbilitySystem == nullptr)
    {
        // 부착 실패를 성공으로 감추지 않는다. 태스크만 완료하고 타임라인은 계속 진행한다.
        UE_LOG(LogKata, Warning, TEXT("Kata loose tag task '%s' found no ability system for the selected target"),
            *GetDisplayName());
        FinishTask();
        return;
    }

    // 회수 기준은 설정이 아니라 실제로 붙인 값이다. 대상도 함께 기억해 다른 ASC에서 빼지 않는다.
    AppliedTags = Definition->Tags;
    TargetAbilitySystem = ReceivingAbilitySystem;
    ReceivingAbilitySystem->AddLooseGameplayTags(AppliedTags);
}

void UKataTaskInstance_ApplyLooseTag::OnTaskEnded_Implementation(EKataTaskEndReason Reason)
{
    if (!AppliedTags.IsEmpty())
    {
        // 어떤 사유로 끝나든 자신이 올린 참조 카운트만 되돌린다.
        if (UAbilitySystemComponent* ReceivingAbilitySystem = TargetAbilitySystem.Get())
        {
            ReceivingAbilitySystem->RemoveLooseGameplayTags(AppliedTags);
        }
        AppliedTags.Reset();
    }
    TargetAbilitySystem.Reset();

    Super::OnTaskEnded_Implementation(Reason);
}
