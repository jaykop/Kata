// Copyright Epic Games, Inc. All Rights Reserved.

#include "Testing/KataTestActor.h"

#include "AbilitySystemComponent.h"
#include "Action/KataAction.h"
#include "Action/KataResolvedAction.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "KataRuntimeLog.h"
#include "Runtime/KataActionComponent.h"
#include "Runtime/KataActionInstance.h"
#include "TimerManager.h"
#include "Testing/KataTestLogging.h"

AKataTestActor::AKataTestActor()
{
    PrimaryActorTick.bCanEverTick = false;

    AbilitySystem = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
    ActionComponent = CreateDefaultSubobject<UKataActionComponent>(TEXT("KataActionComponent"));
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

    if (ActionComponent != nullptr)
    {
        ActionComponent->OnKataStarted.AddDynamic(this, &AKataTestActor::HandleKataStarted);
        ActionComponent->OnKataEnded.AddDynamic(this, &AKataTestActor::HandleKataEnded);
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

UKataAction* AKataTestActor::ResolveActionToPlay()
{
    if (BuiltInAction != EKataTestAction::None)
    {
        // 코드 하네스는 매번 새로 만든다. 팩토리를 고친 뒤에도 최신 값이 반영된다.
        BuiltInActionObject = KataTestActions::Make(BuiltInAction, this);
        return BuiltInActionObject;
    }
    BuiltInActionObject = nullptr;
    return ActionToPlay;
}

void AKataTestActor::PlayTestKata()
{
    if (ActionComponent == nullptr)
    {
        return;
    }

    UKataAction* Action = ResolveActionToPlay();
    if (Action == nullptr)
    {
        UE_LOG(LogKata, Error, TEXT("KataTestActor: no action to play; set Built In Action or Action To Play"));
        return;
    }

    FKataContext Context;
    Context.OwnerActor = this;
    Context.AvatarActor = this;
    Context.TargetActor = TargetActor.Get();
    Context.AbilitySystem = AbilitySystem.Get();

    UKataActionInstance* Instance = nullptr;
    const EKataStartResult Result = ActionComponent->PlayKataAction(Action, Context, Instance);

    const FString ResultName = StaticEnum<EKataStartResult>()->GetNameStringByValue(static_cast<int64>(Result));
    const FString Message = FString::Printf(TEXT("[Kata] PlayKataAction(%s) -> %s"), *Action->GetName(), *ResultName);

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
    if (ActionComponent != nullptr)
    {
        ActionComponent->StopKata(EKataEndReason::Cancelled);
    }
}

void AKataTestActor::DumpResolvedAction()
{
    KataTestLogging::DumpResolvedAction(ResolveActionToPlay());
}

void AKataTestActor::HandleKataStarted(UKataActionInstance* Instance)
{
    UE_LOG(LogKata, Log, TEXT("[Kata] OnKataStarted duration=%.3fs"),
        Instance != nullptr ? Instance->GetTimelineDuration() : 0.0f);
}

void AKataTestActor::HandleKataEnded(UKataActionInstance* Instance, EKataEndReason EndReason)
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
