// Copyright Epic Games, Inc. All Rights Reserved.

#include "Testing/KataTestActor.h"

#include "AbilitySystemComponent.h"
#include "Definition/KataResolvedDefinition.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "KataRuntimeLog.h"
#include "Runtime/KataComponent.h"
#include "Runtime/KataInstance.h"
#include "TimerManager.h"
#include "Testing/KataTestLogging.h"

AKataTestActor::AKataTestActor()
{
    PrimaryActorTick.bCanEverTick = false;

    AbilitySystem = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
    KataComponent = CreateDefaultSubobject<UKataComponent>(TEXT("KataComponent"));
}

UAbilitySystemComponent* AKataTestActor::GetAbilitySystemComponent() const
{
    return AbilitySystem;
}

void AKataTestActor::PostInitializeComponents()
{
    Super::PostInitializeComponents();

    if (AbilitySystem != nullptr)
    {
        // Gameplay Effect 적용에 필요한 Owner/Avatar 정보를 채운다.
        AbilitySystem->InitAbilityActorInfo(this, this);
    }
}

void AKataTestActor::BeginPlay()
{
    Super::BeginPlay();

    if (AbilitySystem != nullptr && !StartupLooseTags.IsEmpty())
    {
        AbilitySystem->AddLooseGameplayTags(StartupLooseTags, 1);
        UE_LOG(LogKata, Log, TEXT("KataTestActor: added startup loose tags %s"), *StartupLooseTags.ToStringSimple());
    }

    if (KataComponent != nullptr)
    {
        KataComponent->OnKataStarted.AddDynamic(this, &AKataTestActor::HandleKataStarted);
        KataComponent->OnKataEnded.AddDynamic(this, &AKataTestActor::HandleKataEnded);
    }

    if (!bPlayOnBeginPlay)
    {
        return;
    }

    if (PlayDelaySeconds <= 0.0f)
    {
        PlayTestKata();
        return;
    }

    GetWorldTimerManager().SetTimer(PlayTimerHandle, this, &AKataTestActor::PlayTestKata, PlayDelaySeconds, false);
}

void AKataTestActor::PlayTestKata()
{
    if (KataComponent == nullptr)
    {
        return;
    }
    if (DefinitionToPlay == nullptr)
    {
        UE_LOG(LogKata, Error, TEXT("KataTestActor: DefinitionToPlay is not set"));
        return;
    }

    // 에디터에서 부모 정의를 고친 뒤에도 최신 상태를 보도록 캐시를 비운다.
    KataComponent->ClearResolvedDefinitionCache();

    FKataContext Context;
    Context.OwnerActor = this;
    Context.AvatarActor = this;
    Context.TargetActor = TargetActor.Get();
    Context.AbilitySystem = AbilitySystem.Get();

    UKataInstance* Instance = nullptr;
    const EKataStartResult Result = KataComponent->PlayKata(DefinitionToPlay, Context, Instance);

    const FString ResultName = StaticEnum<EKataStartResult>()->GetNameStringByValue(static_cast<int64>(Result));
    const FString Message = FString::Printf(TEXT("[Kata] PlayKata(%s) -> %s"), *DefinitionToPlay->GetName(), *ResultName);

    if (Result == EKataStartResult::Started)
    {
        UE_LOG(LogKata, Log, TEXT("%s"), *Message);
    }
    else
    {
        UE_LOG(LogKata, Warning, TEXT("%s"), *Message);
    }

    if (GEngine != nullptr)
    {
        GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.0f, FColor::White, Message);
    }
}

void AKataTestActor::StopTestKata()
{
    if (KataComponent != nullptr)
    {
        KataComponent->StopKata(EKataEndReason::Cancelled);
    }
}

void AKataTestActor::DumpResolvedDefinition()
{
    KataTestLogging::DumpResolvedDefinition(DefinitionToPlay);
}

void AKataTestActor::HandleKataStarted(UKataInstance* Instance)
{
    UE_LOG(LogKata, Log, TEXT("[Kata] OnKataStarted duration=%.3fs"),
        Instance != nullptr ? Instance->GetTimelineDuration() : 0.0f);
}

void AKataTestActor::HandleKataEnded(UKataInstance* Instance, EKataEndReason EndReason)
{
    const FString ReasonName = StaticEnum<EKataEndReason>()->GetNameStringByValue(static_cast<int64>(EndReason));
    const FString Message = FString::Printf(TEXT("[Kata] OnKataEnded reason=%s loops=%d"),
        *ReasonName, Instance != nullptr ? Instance->GetLoopIteration() : 0);

    UE_LOG(LogKata, Log, TEXT("%s"), *Message);

    if (GEngine != nullptr)
    {
        GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.0f, FColor::White, Message);
    }
}
