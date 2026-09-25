#include "HitTrace/KataHitHandler.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Action/KataTask.h"
#include "GameFramework/Actor.h"
#include "GameplayEffect.h"
#include "KataFrameworkLog.h"

void UKataHitHandler::HandleHit_Implementation(AActor* InstigatorActor, AActor* TargetActor, const FHitResult& HitResult,
    UAbilitySystemComponent* SourceAbilitySystem, const UKataTask* SourceTask) const
{
}

FString UKataHitHandler::GetConfigurationError() const
{
    return FString();
}

void UKataHitHandler_SendGameplayEvent::HandleHit_Implementation(AActor* InstigatorActor, AActor* TargetActor, const FHitResult& HitResult,
    UAbilitySystemComponent* SourceAbilitySystem, const UKataTask* SourceTask) const
{
    if (!EventTag.IsValid())
    {
        return;
    }

    // UAbilitySystemBlueprintLibrary::SendGameplayEventToActor는 IAbilitySystemInterface가 필요하다.
    // 컴포넌트만 붙은 프리뷰 액터도 받도록 컴포넌트 탐색까지 하는 조회로 ASC를 찾아 직접 보낸다.
    UAbilitySystemComponent* RecipientAbilitySystem = Recipient == EKataHitEventRecipient::Instigator && SourceAbilitySystem != nullptr
        ? SourceAbilitySystem
        : UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Recipient == EKataHitEventRecipient::Target ? TargetActor : InstigatorActor);
    if (RecipientAbilitySystem == nullptr)
    {
        // ASC 없는 소품을 치는 일은 흔하므로 대상 쪽 누락은 경고로 남기지 않는다. 공격한 쪽 누락은 설정 문제다.
        if (Recipient == EKataHitEventRecipient::Instigator)
        {
            UE_LOG(LogKataFramework, Warning, TEXT("Kata hit event '%s' found no ability system on the instigator '%s'"),
                *EventTag.ToString(), *GetNameSafe(InstigatorActor));
        }
        else
        {
            UE_LOG(LogKataFramework, Verbose, TEXT("Kata hit event '%s' skipped target '%s' without an ability system"),
                *EventTag.ToString(), *GetNameSafe(TargetActor));
        }
        return;
    }

    FGameplayEventData Payload;
    Payload.EventTag = EventTag;
    Payload.Instigator = InstigatorActor;
    Payload.Target = TargetActor;
    Payload.OptionalObject = SourceTask;
    Payload.EventMagnitude = EventMagnitude;
    Payload.TargetData = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromHitResult(HitResult);
    RecipientAbilitySystem->HandleGameplayEvent(EventTag, &Payload);
}

FString UKataHitHandler_SendGameplayEvent::GetConfigurationError() const
{
    if (!EventTag.IsValid())
    {
        return TEXT("Send Gameplay Event handler needs an 'Event Tag'");
    }
    if (!FMath::IsFinite(EventMagnitude))
    {
        return TEXT("Send Gameplay Event handler 'Event Magnitude' must be a finite number");
    }
    return FString();
}

void UKataHitHandler_ApplyGameplayEffect::HandleHit_Implementation(AActor* InstigatorActor, AActor* TargetActor, const FHitResult& HitResult,
    UAbilitySystemComponent* SourceAbilitySystem, const UKataTask* SourceTask) const
{
    if (EffectClass == nullptr)
    {
        return;
    }

    UAbilitySystemComponent* TargetAbilitySystem = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor);
    if (SourceAbilitySystem == nullptr)
    {
        // 공격한 쪽에 ASC가 없으면 설정 문제이므로 조용히 넘기지 않는다.
        UE_LOG(LogKataFramework, Warning, TEXT("Kata hit effect '%s' has no ability system on the instigator '%s'"),
            *GetNameSafe(EffectClass), *GetNameSafe(InstigatorActor));
        return;
    }
    if (TargetAbilitySystem == nullptr)
    {
        // ASC 없는 소품을 치는 일은 흔하므로 경고로 남기지 않는다.
        UE_LOG(LogKataFramework, Verbose, TEXT("Kata hit effect '%s' skipped target '%s' without an ability system"),
            *GetNameSafe(EffectClass), *GetNameSafe(TargetActor));
        return;
    }

    FGameplayEffectContextHandle EffectContext = SourceAbilitySystem->MakeEffectContext();
    EffectContext.AddSourceObject(SourceTask);
    EffectContext.AddHitResult(HitResult);

    const FGameplayEffectSpecHandle SpecHandle = SourceAbilitySystem->MakeOutgoingSpec(EffectClass, EffectLevel, EffectContext);
    if (!SpecHandle.IsValid())
    {
        UE_LOG(LogKataFramework, Warning, TEXT("Kata hit effect handler failed to build a spec for '%s'"), *GetNameSafe(EffectClass));
        return;
    }
    SourceAbilitySystem->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetAbilitySystem);
}

FString UKataHitHandler_ApplyGameplayEffect::GetConfigurationError() const
{
    if (EffectClass == nullptr)
    {
        return TEXT("Apply Gameplay Effect handler needs an 'Effect Class'");
    }
    if (!FMath::IsFinite(EffectLevel))
    {
        return TEXT("Apply Gameplay Effect handler 'Effect Level' must be a finite number");
    }
    return FString();
}
