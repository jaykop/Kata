#include "Death/KataDeathAbility.h"

#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Attributes/KataCombatSettings.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Death/AbilityTask_KataDimOut.h"
#include "Death/KataDeathComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GAS/AbilityTask_PlayKataAction.h"
#include "KataFrameworkLog.h"
#include "PhysicsEngine/PhysicsAsset.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

namespace KataDeath
{
    /** 고른 방향의 사망 Kata를 찾고, 비어 있으면 Front로 대신한다. */
    UKataAction* FindActionForDirection(const TMap<EKataHitDirection, TObjectPtr<UKataAction>>& Map, EKataHitDirection Direction)
    {
        if (const TObjectPtr<UKataAction>* Found = Map.Find(Direction); Found != nullptr && *Found != nullptr)
        {
            return Found->Get();
        }
        const TObjectPtr<UKataAction>* Front = Map.Find(EKataHitDirection::Front);
        return Front != nullptr ? Front->Get() : nullptr;
    }

    bool UsesAction(EKataDeathPresentation Presentation)
    {
        return Presentation == EKataDeathPresentation::Action || Presentation == EKataDeathPresentation::ActionThenRagdoll;
    }
}

UKataDeathAbility::UKataDeathAbility()
{
    // 연출·시체·DimOut 태스크와 진행 상태를 가지므로 액터별 인스턴스를 쓴다.
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

#if WITH_EDITOR
EDataValidationResult UKataDeathAbility::IsDataValid(FDataValidationContext& Context) const
{
    EDataValidationResult Result = Super::IsDataValid(Context);
    if (!ActivationBlockedTags.IsEmpty())
    {
        Context.AddError(FText::FromString(TEXT("Kata Death Ability must not have Activation Blocked Tags. A blocked death ability leaves the dead actor without presentation.")));
        Result = EDataValidationResult::Invalid;
    }
    if (KataDeath::UsesAction(Presentation) && DirectionalActions.IsEmpty())
    {
        Context.AddWarning(FText::FromString(TEXT("Kata Death Ability uses an action presentation but has no Directional Actions.")));
    }
    if (AbilityTriggers.IsEmpty())
    {
        Context.AddWarning(FText::FromString(TEXT("Kata Death Ability has no Ability Triggers. Add the Death Event Tag from the Kata Combat settings.")));
    }
    return Result;
}
#endif

void UKataDeathAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    // 비용·쿨다운으로 사망 연출이 거절되면 안 되므로 Commit하지 않는다.
    bPresentationFinished = false;
    bRemovalRequested = false;
    StartPresentation(TriggerEventData != nullptr ? *TriggerEventData : FGameplayEventData());
}

void UKataDeathAbility::StartPresentation_Implementation(const FGameplayEventData& EventData)
{
    if (Presentation == EKataDeathPresentation::None)
    {
        FinishPresentation();
        return;
    }

    ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    if (Character == nullptr)
    {
        UE_LOG(LogKataFramework, Warning, TEXT("Kata death ability '%s' needs a character for %s presentation on '%s'. Skipping presentation."),
            *GetName(), *UEnum::GetValueAsString(Presentation), *GetNameSafe(GetAvatarActorFromActorInfo()));
        FinishPresentation();
        return;
    }

    if (Presentation == EKataDeathPresentation::Ragdoll)
    {
        StartRagdoll(Character);
        FinishPresentation();
        return;
    }

    // FKataContext의 대상은 const가 아닌 액터를 받는다. 공격자를 대상으로 넘길 뿐 수정하지 않는다.
    AActor* Attacker = const_cast<AActor*>(EventData.Instigator.Get());
    FHitResult HitResult;
    if (const FHitResult* EventHitResult = EventData.ContextHandle.GetHitResult())
    {
        HitResult = *EventHitResult;
    }
    const EKataHitDirection Direction = UKataHitReactionAbility::ResolveHitDirection(Character, HitResult, Attacker,
        UKataCombatSettings::Get()->MinHorizontalDirectionRatio);

    UKataAction* Action = KataDeath::FindActionForDirection(DirectionalActions, Direction);
    if (Action == nullptr)
    {
        UE_LOG(LogKataFramework, Warning, TEXT("Kata death ability '%s' has no death action for %s or Front."),
            *GetName(), *UEnum::GetValueAsString(Direction));
        CompleteActionPresentation();
        return;
    }

    UAbilityTask_PlayKataAction* Task = UAbilityTask_PlayKataAction::PlayKataAction(this, Action, Attacker);
    Task->OnCompleted.AddDynamic(this, &UKataDeathAbility::HandleActionEnded);
    Task->OnInterrupted.AddDynamic(this, &UKataDeathAbility::HandleActionEnded);
    Task->OnBranched.AddDynamic(this, &UKataDeathAbility::HandleActionEnded);
    Task->OnFailed.AddDynamic(this, &UKataDeathAbility::HandleActionFailed);
    Task->ReadyForActivation();
}

void UKataDeathAbility::HandleActionEnded(EKataEndReason EndReason)
{
    CompleteActionPresentation();
}

void UKataDeathAbility::HandleActionFailed(EKataStartResult FailureReason)
{
    // 시작이 거절돼도 시체가 서 있는 채로 남지 않게 연출 방식대로 마무리한다.
    UE_LOG(LogKataFramework, Warning, TEXT("Kata death ability '%s' could not start its death action (result %d)."),
        *GetName(), static_cast<int32>(FailureReason));
    CompleteActionPresentation();
}

void UKataDeathAbility::CompleteActionPresentation()
{
    if (bPresentationFinished || !IsActive())
    {
        return;
    }
    if (ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
    {
        if (Presentation == EKataDeathPresentation::ActionThenRagdoll)
        {
            StartRagdoll(Character);
        }
        else
        {
            StopCharacterMovement(Character);
        }
    }
    FinishPresentation();
}

void UKataDeathAbility::FinishPresentation()
{
    if (bPresentationFinished || !IsActive())
    {
        return;
    }
    bPresentationFinished = true;

    if (CorpseLifetime > 0.0f)
    {
        UAbilityTask_WaitDelay* Wait = UAbilityTask_WaitDelay::WaitDelay(this, CorpseLifetime);
        Wait->OnFinish.AddDynamic(this, &UKataDeathAbility::HandleCorpseLifetimeEnded);
        Wait->ReadyForActivation();
        return;
    }
    StartDimOut();
}

void UKataDeathAbility::HandleCorpseLifetimeEnded()
{
    StartDimOut();
}

void UKataDeathAbility::StartDimOut()
{
    if (DimOutDuration > 0.0f && !DimOutParameterName.IsNone())
    {
        UAbilityTask_KataDimOut* DimOut = UAbilityTask_KataDimOut::KataDimOut(this, DimOutParameterName, DimOutDuration);
        DimOut->OnFinished.AddDynamic(this, &UKataDeathAbility::HandleDimOutFinished);
        DimOut->ReadyForActivation();
        return;
    }
    HandleDimOutFinished();
}

void UKataDeathAbility::HandleDimOutFinished()
{
    RequestRemovalOnce();
    EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UKataDeathAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    if (IsActive())
    {
        // 시체 단계 전에 취소돼도 죽은 대상이 남지 않게 제거는 반드시 요청한다.
        if (!bPresentationFinished || bWasCancelled)
        {
            UE_LOG(LogKataFramework, Verbose, TEXT("Kata death ability '%s' ended early on '%s'. Requesting removal now."),
                *GetName(), *GetNameSafe(GetAvatarActorFromActorInfo()));
        }
        RequestRemovalOnce();
    }
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UKataDeathAbility::RequestRemovalOnce()
{
    if (bRemovalRequested)
    {
        return;
    }
    bRemovalRequested = true;
    if (UKataDeathComponent* DeathComponent = GetDeathComponent())
    {
        DeathComponent->RequestRemoval();
        return;
    }
    UE_LOG(LogKataFramework, Warning, TEXT("Kata death ability '%s' found no Kata Death component on '%s'. The actor is not removed."),
        *GetName(), *GetNameSafe(GetAvatarActorFromActorInfo()));
}

UKataDeathComponent* UKataDeathAbility::GetDeathComponent() const
{
    const AActor* Avatar = GetAvatarActorFromActorInfo();
    return Avatar != nullptr ? Avatar->FindComponentByClass<UKataDeathComponent>() : nullptr;
}

void UKataDeathAbility::StopCharacterMovement(ACharacter* Character)
{
    if (Character == nullptr)
    {
        return;
    }
    if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
    {
        Movement->StopMovementImmediately();
        Movement->DisableMovement();
    }
}

void UKataDeathAbility::StartRagdoll(ACharacter* Character)
{
    if (Character == nullptr)
    {
        return;
    }
    StopCharacterMovement(Character);

    USkeletalMeshComponent* Mesh = Character->GetMesh();
    if (Mesh == nullptr || Mesh->GetPhysicsAsset() == nullptr)
    {
        // 캡슐까지 끄면 물리 바디 없이 바닥을 통과하므로 캡슐은 그대로 둔다.
        UE_LOG(LogKataFramework, Warning, TEXT("Kata ragdoll on '%s' skipped: the mesh has no Physics Asset."), *GetNameSafe(Character));
        return;
    }

    // 캡슐이 남아 있으면 물리 바디와 겹쳐 튀므로 먼저 끈다.
    if (UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
    {
        Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
    Mesh->SetCollisionProfileName(UKataCombatSettings::Get()->RagdollCollisionProfile);
    Mesh->SetSimulatePhysics(true);
    Mesh->WakeAllRigidBodies();
    Mesh->bBlendPhysics = true;
}
