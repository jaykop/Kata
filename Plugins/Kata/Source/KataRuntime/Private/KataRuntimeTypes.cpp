#include "KataRuntimeTypes.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/Actor.h"

namespace
{
    /** 진단 심각도를 로그와 에디터 메시지에 함께 쓸 영어 표기로 바꾼다. */
    const TCHAR* LexSeverity(EKataDiagnosticSeverity Severity)
    {
        switch (Severity)
        {
        case EKataDiagnosticSeverity::Error:
            return TEXT("Error");
        case EKataDiagnosticSeverity::Warning:
            return TEXT("Warning");
        default:
            return TEXT("Info");
        }
    }
}

FString FKataDiagnostic::ToDetailString() const
{
    FString TaskPart;
    if (!TaskLabel.IsEmpty())
    {
        TaskPart = FString::Printf(TEXT(" [Task %s]"), *TaskLabel);
    }
    else if (TaskId.IsValid())
    {
        TaskPart = FString::Printf(TEXT(" [Task %s]"), *TaskId.ToString());
    }
    return FString::Printf(TEXT("%s%s - %s"), *Code.ToString(), *TaskPart, *Detail);
}

FString FKataDiagnostic::ToDisplayString() const
{
    return FString::Printf(TEXT("%s: %s"), LexSeverity(Severity), *ToDetailString());
}

AActor* FKataContext::GetAvatarActor() const
{
    if (AActor* Avatar = AvatarActor.Get())
    {
        return Avatar;
    }
    return OwnerActor.Get();
}

UAbilitySystemComponent* FKataContext::ResolveAbilitySystem() const
{
    if (UAbilitySystemComponent* Explicit = AbilitySystem.Get())
    {
        return Explicit;
    }

    // 명시적 ASC가 없을 때만 액터에서 조회한다. 임의의 PlayerState 연결까지 추론하지는 않는다.
    if (AActor* Avatar = AvatarActor.Get())
    {
        if (UAbilitySystemComponent* Found = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Avatar))
        {
            return Found;
        }
    }
    if (AActor* Owner = OwnerActor.Get())
    {
        return UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Owner);
    }
    return nullptr;
}

FKataConditionContext FKataContext::ToConditionContext() const
{
    FKataConditionContext ConditionContext;
    ConditionContext.SelfActor = GetAvatarActor();
    ConditionContext.TargetActor = TargetActor;
    ConditionContext.SelfAbilitySystem = ResolveAbilitySystem();

    // Target 쪽 ASC는 조건 구현이 액터에서 직접 조회하므로 명시적으로 채우지 않는다.
    return ConditionContext;
}

bool FKataContext::HasValidOwner() const
{
    return GetAvatarActor() != nullptr;
}
