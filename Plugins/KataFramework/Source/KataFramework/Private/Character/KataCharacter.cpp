// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/KataCharacter.h"

#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Character/KataCharacterRow.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "HitTrace/KataHitBoxComponent.h"
#include "KataGraphComponent.h"
#include "Runtime/KataActionComponent.h"
#include "StructUtils/InstancedStruct.h"
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

void AKataCharacter::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    if (PendingCharacterRow.IsValid())
    {
        ApplyCharacterRow(PendingCharacterRow);
    }
}

void AKataCharacter::FinishSpawningWithCharacterRow(const FTransform& SpawnTransform, const FInstancedStruct& RowData)
{
    PendingCharacterRow = RowData;
    FinishSpawning(SpawnTransform);
    PendingCharacterRow.Reset();
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

void AKataCharacter::ApplyCharacterRow(const FInstancedStruct& RowData)
{
    const FKataCharacterRow* Row = RowData.GetPtr<FKataCharacterRow>();
    USkeletalMeshComponent* MeshComponent = GetMesh();
    if (Row == nullptr || MeshComponent == nullptr)
    {
        return;
    }

    // Construction Script가 메시를 다시 설정할 수 있으므로 스크립트가 끝난 OnConstruction에서 행 값을 우선 적용한다.
    // 비어 있는 항목은 Blueprint 기본값을 유지한다.
    if (USkeletalMesh* RowMesh = Row->SkeletalMesh.Get())
    {
        MeshComponent->SetSkeletalMeshAsset(RowMesh);
    }
    if (UClass* RowAnimClass = Row->AnimClass.Get())
    {
        MeshComponent->SetAnimInstanceClass(RowAnimClass);
    }
}

