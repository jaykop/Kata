// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/KataCharacter.h"

#include "AbilitySystemComponent.h"
#include "Runtime/KataActionComponent.h"

AKataCharacter::AKataCharacter()
{
    AbilitySystem = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
    ActionComponent = CreateDefaultSubobject<UKataActionComponent>(TEXT("KataActionComponent"));
}

UAbilitySystemComponent* AKataCharacter::GetAbilitySystemComponent() const
{
    return AbilitySystem;
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
