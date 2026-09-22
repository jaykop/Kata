#include "Tasks/KataTask_ApplyGameplayEffect.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "KataRuntimeLog.h"

TSubclassOf<UKataTaskInstance> UKataTask_ApplyGameplayEffect::GetTaskInstanceClass_Implementation() const
{
    return UKataTaskInstance_ApplyGameplayEffect::StaticClass();
}

FName UKataTask_ApplyGameplayEffect::GetConfigurationError() const
{
    const FName SuperError = Super::GetConfigurationError();
    if (!SuperError.IsNone())
    {
        return SuperError;
    }
    if (EffectClass == nullptr)
    {
        return TEXT("MissingEffectClass");
    }
    if (!FMath::IsFinite(EffectLevel))
    {
        return TEXT("InvalidEffectLevel");
    }
    return NAME_None;
}

FString UKataTask_ApplyGameplayEffect::DescribeConfigurationError(FName ErrorCode) const
{
    if (ErrorCode == TEXT("MissingEffectClass"))
    {
        return TEXT("'Effect Class' is not set");
    }
    if (ErrorCode == TEXT("InvalidEffectLevel"))
    {
        return TEXT("'Effect Level' must be a finite number");
    }
    return Super::DescribeConfigurationError(ErrorCode);
}

void UKataTaskInstance_ApplyGameplayEffect::OnTaskStarted_Implementation()
{
    const UKataTask_ApplyGameplayEffect* Definition = Cast<UKataTask_ApplyGameplayEffect>(GetTaskDefinition());
    if (Definition == nullptr || Definition->EffectClass == nullptr)
    {
        FinishTask();
        return;
    }

    const FKataContext Context = GetKataContext();

    // 적용 주체는 언제나 Kata를 실행하는 쪽이다. 대상만 설정에 따라 달라진다.
    UAbilitySystemComponent* SourceAbilitySystem = Context.ResolveAbilitySystem();
    UAbilitySystemComponent* ReceivingAbilitySystem = Context.ResolveAbilitySystemFor(Definition->EffectTarget);
    if (SourceAbilitySystem == nullptr || ReceivingAbilitySystem == nullptr)
    {
        // 적용 실패를 성공으로 감추지 않는다. 태스크만 완료하고 타임라인은 계속 진행한다.
        UE_LOG(LogKata, Warning, TEXT("Kata gameplay effect task '%s' found no ability system for the source or the selected target"),
            *GetDisplayName());
        FinishTask();
        return;
    }

    FGameplayEffectContextHandle EffectContext = SourceAbilitySystem->MakeEffectContext();
    EffectContext.AddSourceObject(Definition);

    const FGameplayEffectSpecHandle SpecHandle =
        SourceAbilitySystem->MakeOutgoingSpec(Definition->EffectClass, Definition->EffectLevel, EffectContext);
    if (!SpecHandle.IsValid())
    {
        UE_LOG(LogKata, Warning, TEXT("Kata gameplay effect task '%s' failed to build a spec for effect '%s'"),
            *GetDisplayName(), *GetNameSafe(Definition->EffectClass));
        FinishTask();
        return;
    }

    AppliedHandle = SourceAbilitySystem->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), ReceivingAbilitySystem);

    // 핸들은 받은 쪽의 활성 효과 목록을 가리키므로 제거도 같은 ASC에서 해야 한다.
    TargetAbilitySystem = ReceivingAbilitySystem;

    if (Definition->RemovePolicy == EKataEffectRemovePolicy::RemoveOnTaskEnd && !AppliedHandle.IsValid())
    {
        // Instant GE처럼 남는 활성 효과가 없으면 구간 종료에 제거할 대상도 없다.
        UE_LOG(LogKata, Warning, TEXT("Kata gameplay effect task '%s' asked to remove effect '%s' on task end, but the effect left no active handle"),
            *GetDisplayName(), *GetNameSafe(Definition->EffectClass));
    }
}

void UKataTaskInstance_ApplyGameplayEffect::OnTaskEnded_Implementation(EKataTaskEndReason Reason)
{
    const UKataTask_ApplyGameplayEffect* Definition = Cast<UKataTask_ApplyGameplayEffect>(GetTaskDefinition());
    const EKataEffectRemovePolicy Policy = Definition != nullptr
        ? Definition->RemovePolicy
        : EKataEffectRemovePolicy::UseEffectDuration;

    if (Policy == EKataEffectRemovePolicy::RemoveOnTaskEnd && AppliedHandle.IsValid())
    {
        // 어떤 사유로 끝나든 자신이 적용한 효과만 회수한다.
        if (UAbilitySystemComponent* ReceivingAbilitySystem = TargetAbilitySystem.Get())
        {
            ReceivingAbilitySystem->RemoveActiveGameplayEffect(AppliedHandle);
        }
    }

    AppliedHandle = FActiveGameplayEffectHandle();
    TargetAbilitySystem.Reset();

    Super::OnTaskEnded_Implementation(Reason);
}
