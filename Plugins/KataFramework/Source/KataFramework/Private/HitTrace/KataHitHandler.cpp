#include "HitTrace/KataHitHandler.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Action/KataTask.h"
#include "Attributes/KataCombatSettings.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameplayCueManager.h"
#include "GameplayEffect.h"
#include "HitTrace/KataHurtBoxComponent.h"
#include "KataFrameworkLog.h"
#include "Time/KataTimeDilationSubsystem.h"

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
    // 받는 Ability가 약점 같은 피격 영역 속성으로 분기할 수 있도록 맞은 HurtBox의 태그를 넘긴다.
    if (const UKataHurtBoxComponent* HurtBox = Cast<UKataHurtBoxComponent>(HitResult.GetComponent()))
    {
        Payload.TargetTags = HurtBox->GetHurtBoxTags();
    }
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
    for (const TPair<FGameplayTag, float>& Pair : SetByCallerMagnitudes)
    {
        SpecHandle.Data->SetSetByCallerMagnitude(Pair.Key, Pair.Value);
    }
    if (ImpactTag.IsValid())
    {
        // GE 에셋 태그는 에셋마다 고정되므로 공격마다 다른 등급은 스펙 단위 동적 태그로 넣는다.
        SpecHandle.Data->AddDynamicAssetTag(ImpactTag);
    }
    const FActiveGameplayEffectHandle AppliedHandle = SourceAbilitySystem->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetAbilitySystem);

    // 적용 요건(무적 등)에 막히면 기본 핸들이 돌아와 false다. Instant GE도 실행되면 true인 핸들을 돌려준다.
    // Attribute 콜백은 바뀐 Attribute마다 불려 한 번 실행을 보장할 수 없으므로 적용 직후 이 값으로 판단한다.
    if (AppliedHandle.WasSuccessfullyApplied())
    {
        ExecuteHitCues(InstigatorActor, TargetActor, HitResult, EffectContext);
        RequestHitStop(InstigatorActor, TargetActor);
    }
}

void UKataHitHandler_ApplyGameplayEffect::ExecuteHitCues(AActor* InstigatorActor, AActor* TargetActor, const FHitResult& HitResult,
    const FGameplayEffectContextHandle& EffectContext) const
{
    const FKataImpactResponse* ImpactResponse = UKataCombatSettings::Get()->FindImpactResponse(ImpactTag);
    const FGameplayTag ResolvedHitCueTag = HitCueTag.IsValid() ? HitCueTag
        : ImpactResponse != nullptr ? ImpactResponse->HitCueTag : FGameplayTag();
    const FGameplayTag ResolvedAttackerCueTag = AttackerCueTag.IsValid() ? AttackerCueTag
        : ImpactResponse != nullptr ? ImpactResponse->AttackerCueTag : FGameplayTag();
    if (!ResolvedHitCueTag.IsValid() && !ResolvedAttackerCueTag.IsValid())
    {
        return;
    }

    FGameplayCueParameters CueParameters;
    UAbilitySystemGlobals::Get().InitGameplayCueParameters(CueParameters, EffectContext);
    // Burst Cue는 Spawn Condition의 Locally Controlled Source가 Instigator일 때 이 값으로 로컬 플레이어를 찾는다.
    // Effect Context의 Instigator는 ASC 소유자라 Avatar와 다를 수 있으므로 공격한 Avatar를 직접 넣는다.
    CueParameters.Instigator = InstigatorActor;
    CueParameters.EffectCauser = InstigatorActor;
    // Cue 배치는 막힌 히트의 접촉 지점을 먼저 쓰고, 그렇지 않으면 이 위치를 쓴다. 겹침으로 찾은 히트도 같은 지점에 나오게 채운다.
    CueParameters.Location = HitResult.ImpactPoint;
    CueParameters.Normal = HitResult.ImpactNormal;

    // 각 Cue를 연출의 주인에서 실행한다. Locally Controlled Source를 Target Actor로 두면 맞은 쪽 Cue는 피격자,
    // 때린 쪽 Cue는 공격자가 플레이어일 때만 흔들림·진동을 재생한다.
    if (ResolvedHitCueTag.IsValid())
    {
        UGameplayCueManager::ExecuteGameplayCue_NonReplicated(TargetActor, ResolvedHitCueTag, CueParameters);
    }
    if (ResolvedAttackerCueTag.IsValid() && InstigatorActor != nullptr)
    {
        UGameplayCueManager::ExecuteGameplayCue_NonReplicated(InstigatorActor, ResolvedAttackerCueTag, CueParameters);
    }
}

void UKataHitHandler_ApplyGameplayEffect::RequestHitStop(AActor* InstigatorActor, AActor* TargetActor) const
{
    if (HitStopDuration <= 0.0f)
    {
        return;
    }

    UWorld* World = TargetActor != nullptr ? TargetActor->GetWorld() : nullptr;
    UKataTimeDilationSubsystem* TimeDilation = World != nullptr ? World->GetSubsystem<UKataTimeDilationSubsystem>() : nullptr;
    if (TimeDilation == nullptr)
    {
        return;
    }

    // 요청은 Duration으로 스스로 풀리므로 핸들을 보관하지 않는다. 공유 처리기에 실행 상태를 남기지 않기 위해서다.
    // 한 공격이 같은 프레임에 여러 대상을 맞히면 공격자에게 같은 값의 요청이 여러 개 쌓이며, 가장 늦게 끝나는 요청까지 멈춘다.
    const UKataCombatSettings* Settings = UKataCombatSettings::Get();
    if (InstigatorActor != nullptr && InstigatorActor != TargetActor)
    {
        TimeDilation->RequestActorTimeDilation(InstigatorActor, Settings->HitStopTimeDilation, Settings->HitStopPriority, HitStopDuration);
    }
    TimeDilation->RequestActorTimeDilation(TargetActor, Settings->HitStopTimeDilation, Settings->HitStopPriority, HitStopDuration);
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
    if (!FMath::IsFinite(HitStopDuration) || HitStopDuration < 0.0f)
    {
        return TEXT("Apply Gameplay Effect handler 'Hit Stop Duration' must be a finite number of zero or more");
    }
    for (const TPair<FGameplayTag, float>& Pair : SetByCallerMagnitudes)
    {
        if (!Pair.Key.IsValid())
        {
            return TEXT("Apply Gameplay Effect handler has a 'Set By Caller Magnitudes' entry without a tag");
        }
        if (!FMath::IsFinite(Pair.Value))
        {
            return FString::Printf(TEXT("Apply Gameplay Effect handler Set By Caller '%s' must be a finite number"), *Pair.Key.ToString());
        }
    }
    return FString();
}
