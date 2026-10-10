// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/KataCharacter.h"
#include "Character/KataCharacterSpawnOwnership.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"

#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/KataAnimLayerSetup.h"
#include "Animation/KataRootMotionCurveComponent.h"
#include "Character/KataCharacterRow.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Equipment/KataEquipmentComponent.h"
#include "Equipment/KataEquipmentSetup.h"
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
    EquipmentComponent = CreateDefaultSubobject<UKataEquipmentComponent>(TEXT("KataEquipment"));
    RootMotionCurveComponent = CreateDefaultSubobject<UKataRootMotionCurveComponent>(TEXT("KataRootMotionCurve"));

    // 런타임에 생성한 NPC도 레벨에 배치한 NPC처럼 AI 컨트롤러를 받게 한다.
    // 컨트롤러가 없으면 CharacterMovement가 중력을 포함한 이동 계산을 건너뛰어 생성 위치에 멈춘다.
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
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

void AKataCharacter::SetSpawnOwnership(const TSharedPtr<FKataCharacterSpawnOwnership>& Ownership)
{
    SpawnOwnership = Ownership;
}

void AKataCharacter::SpawnDefaultController()
{
    if (!SpawnOwnership.IsValid())
    {
        Super::SpawnDefaultController();
        return;
    }
    if (SpawnOwnership->IsDespawnRequested() || GetController() != nullptr || AIControllerClass == nullptr)
    {
        return;
    }
    // 엔진 함수는 만든 Controller를 반환하지 않는다. Possess의 확장 코드가 다른 Controller로 교체하기 전에 실제 생성 결과를 기록한다.
    FActorSpawnParameters SpawnInfo;
    SpawnInfo.Instigator = GetInstigator();
    SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    SpawnInfo.OverrideLevel = GetLevel();
    SpawnInfo.ObjectFlags |= RF_Transient;
    AController* NewController = GetWorld()->SpawnActor<AController>(AIControllerClass, GetActorLocation(), GetActorRotation(), SpawnInfo);
    if (NewController != nullptr)
    {
        SpawnOwnership->OwnedControllers.Add(NewController);
        NewController->OnPossessedPawnChanged.AddDynamic(this, &AKataCharacter::HandleSpawnOwnedControllerPawnChanged);
        NewController->Possess(this);
    }
}

void AKataCharacter::HandleSpawnOwnedControllerPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
    if (!SpawnOwnership.IsValid() || NewPawn == nullptr || NewPawn == this)
    {
        return;
    }
    for (TWeakObjectPtr<AController>& OwnedController : SpawnOwnership->OwnedControllers)
    {
        AController* OwnedControllerPtr = OwnedController.Get();
        if (OwnedControllerPtr != nullptr && OwnedControllerPtr->GetPawn() != nullptr && OwnedControllerPtr->GetPawn() != this)
        {
            // 다른 Pawn으로 이전한 Controller는 다시 대기 상태가 돼도 이 생성 세대의 제거 대상에 넣지 않는다.
            OwnedControllerPtr->OnPossessedPawnChanged.RemoveDynamic(this, &AKataCharacter::HandleSpawnOwnedControllerPawnChanged);
            OwnedController.Reset();
        }
    }
}

void AKataCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (SpawnOwnership.IsValid())
    {
        for (const TWeakObjectPtr<AController>& OwnedController : SpawnOwnership->OwnedControllers)
        {
            if (AController* OwnedControllerPtr = OwnedController.Get())
            {
                OwnedControllerPtr->OnPossessedPawnChanged.RemoveDynamic(this, &AKataCharacter::HandleSpawnOwnedControllerPawnChanged);
            }
        }
    }
    Super::EndPlay(EndPlayReason);
}

void AKataCharacter::DetachFromControllerPendingDestroy()
{
    if (SpawnOwnership.IsValid() && SpawnOwnership->IsDespawnRequested())
    {
        // PawnPendingDestroy는 소유 여부와 무관하게 Controller를 제거할 수 있다. 디스폰은 빙의만 풀고 기록한 Controller를 별도로 처리한다.
        AController* CurrentController = GetController();
        if (CurrentController != nullptr && CurrentController->GetPawn() == this)
        {
            CurrentController->UnPossess();
        }
        return;
    }
    Super::DetachFromControllerPendingDestroy();
}

void AKataCharacter::PostInitializeComponents()
{
    Super::PostInitializeComponents();

    if (AbilitySystem != nullptr)
    {
        // Gameplay Effect 적용과 태그 질의에 필요한 Owner/Avatar 정보를 채운다.
        // 프리뷰 월드에서는 컨트롤러 없이 스폰되므로 Owner와 Avatar를 모두 자기 자신으로 둔다.
        AbilitySystem->InitAbilityActorInfo(this, this);

        // 세트 추가와 초기값 설정은 Actor Info가 준비된 뒤에 해야 한다.
        if (!PendingGameplayData.IsEmpty() || !PendingIdentityTags.IsEmpty())
        {
            GameplayDataHandles = UKataGameplayData::ApplyAll(AbilitySystem, PendingGameplayData, PendingIdentityTags);
            PendingGameplayData.Reset();
            PendingIdentityTags.Reset();
        }
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

    // 행의 에셋은 생성 전에 로드되어 있다. 적용은 ASC가 준비되는 PostInitializeComponents에서 한다.
    PendingGameplayData.Reset();
    for (const TSoftObjectPtr<UKataGameplayData>& Data : Row->GameplayData)
    {
        if (UKataGameplayData* LoadedData = Data.Get())
        {
            PendingGameplayData.Add(LoadedData);
        }
    }
    PendingIdentityTags = Row->IdentityTags;

    // BeginPlay 전에 기록해야 AI Controller와 Perception이 처음부터 이 팩션의 팀 번호를 읽는다.
    if (TargetingComponent != nullptr && Row->Faction.IsValid())
    {
        TargetingComponent->Faction = Row->Faction;
    }

    // 메시·Anim Class를 바꾸면 Anim Instance가 다시 초기화되며 장착 컴포넌트가 레이어를 링크한다.
    // 그때 행의 설정을 쓰도록 먼저 넣어 두되, 바뀌기 전 메시와 비교해 경고가 나지 않게 여기서는 링크하지 않는다.
    UKataAnimLayerSetup* RowLayerSetup = Row->AnimLayerSetup.Get();
    if (EquipmentComponent != nullptr && RowLayerSetup != nullptr)
    {
        EquipmentComponent->AnimLayerSetup = RowLayerSetup;
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

    if (EquipmentComponent != nullptr)
    {
        if (UKataEquipmentSetup* RowSetup = Row->EquipmentSetup.Get())
        {
            EquipmentComponent->SetEquipmentSetup(RowSetup);
        }
        // 지금은 ASC Actor Info가 아직 준비되지 않았으므로 장착은 컴포넌트의 BeginPlay에 맡긴다.
        EquipmentComponent->SetStartingEquipment(Row->StartingEquipment);

        // 메시와 Anim Class가 행 기본값과 같아 다시 초기화되지 않은 경우에도 행의 레이어 설정을 반영한다.
        if (RowLayerSetup != nullptr)
        {
            EquipmentComponent->RefreshAnimLayers();
        }
    }
}

