#include "Tasks/KataTask_SendGameplayEvent.h"

#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GameFramework/Actor.h"
#include "KataRuntimeLog.h"

TSubclassOf<UKataTaskInstance> UKataTask_SendGameplayEvent::GetTaskInstanceClass_Implementation() const
{
    return UKataTaskInstance_SendGameplayEvent::StaticClass();
}

FName UKataTask_SendGameplayEvent::GetConfigurationError() const
{
    const FName SuperError = Super::GetConfigurationError();
    if (!SuperError.IsNone())
    {
        return SuperError;
    }
    if (!EventTag.IsValid())
    {
        return TEXT("MissingEventTag");
    }
    if (!FMath::IsFinite(EventMagnitude))
    {
        return TEXT("InvalidEventMagnitude");
    }
    return NAME_None;
}

FString UKataTask_SendGameplayEvent::DescribeConfigurationError(FName ErrorCode) const
{
    if (ErrorCode == TEXT("MissingEventTag"))
    {
        return TEXT("'Event Tag' must be set so a listener can match this event");
    }
    if (ErrorCode == TEXT("InvalidEventMagnitude"))
    {
        return TEXT("'Event Magnitude' must be a finite number");
    }
    return Super::DescribeConfigurationError(ErrorCode);
}

void UKataTaskInstance_SendGameplayEvent::OnTaskStarted_Implementation()
{
    const UKataTask_SendGameplayEvent* Definition = Cast<UKataTask_SendGameplayEvent>(GetTaskDefinition());
    if (Definition == nullptr || !Definition->EventTag.IsValid())
    {
        FinishTask();
        return;
    }

    const FKataContext Context = GetKataContext();
    UAbilitySystemComponent* TargetAbilitySystem = Context.ResolveAbilitySystemFor(Definition->EventTarget);
    if (TargetAbilitySystem == nullptr)
    {
        // 발송 실패를 성공으로 감추지 않는다. 태스크만 완료하고 타임라인은 계속 진행한다.
        UE_LOG(LogKata, Warning, TEXT("Kata gameplay event task '%s' found no ability system for the selected target"),
            *GetDisplayName());
        FinishTask();
        return;
    }

    FGameplayEventData Payload;
    Payload.EventTag = Definition->EventTag;
    Payload.Instigator = Context.GetAvatarActor();
    Payload.Target = Context.ResolveActor(Definition->EventTarget);
    Payload.OptionalObject = Definition->OptionalObject.Get();
    Payload.EventMagnitude = Definition->EventMagnitude;

    // UAbilitySystemBlueprintLibrary::SendGameplayEventToActor는 대상이 IAbilitySystemInterface를
    // 구현해야만 동작한다. 컴포넌트만 붙어 있는 프리뷰 액터까지 받아들이도록 ASC로 직접 보낸다.
    TargetAbilitySystem->HandleGameplayEvent(Definition->EventTag, &Payload);

    // 보낸 뒤에는 더 할 일이 없다. 지속 시간이 있어도 여기서 완료로 표시한다.
    FinishTask();
}
