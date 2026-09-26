// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/KataCharacter.h"

#include "AbilitySystemComponent.h"
#include "HitTrace/KataHitBoxComponent.h"
#include "KataGraphComponent.h"
#include "Runtime/KataActionComponent.h"
#include "Targeting/KataTargetingComponent.h"

const FName AKataCharacter::TargetingComponentName(TEXT("KataTargetingComponent"));

AKataCharacter::AKataCharacter(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    AbilitySystem = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
    ActionComponent = CreateDefaultSubobject<UKataActionComponent>(TEXT("KataActionComponent"));
    GraphComponent = CreateDefaultSubobject<UKataGraphComponent>(TEXT("KataGraphComponent"));
    TargetingComponent = CreateDefaultSubobject<UKataTargetingComponent>(TargetingComponentName);
    HitBoxComponent = CreateDefaultSubobject<UKataHitBoxComponent>(TEXT("KataHitBoxComponent"));
}

UAbilitySystemComponent* AKataCharacter::GetAbilitySystemComponent() const
{
    return AbilitySystem;
}

FGenericTeamId AKataCharacter::GetGenericTeamId() const
{
    return TargetingComponent != nullptr ? TargetingComponent->GetFactionTeamId() : FGenericTeamId::NoTeam;
}

void AKataCharacter::PostInitializeComponents()
{
    Super::PostInitializeComponents();

    if (AbilitySystem != nullptr)
    {
        // Gameplay Effect 적용과 태그 질의에 필요한 Owner/Avatar 정보를 채운다.
        // 프리뷰 월드에서는 컨트롤러 없이 스폰되므로 Owner와 Avatar를 모두 자기 자신으로 둔다.
        AbilitySystem->InitAbilityActorInfo(this, this);
    }
}
