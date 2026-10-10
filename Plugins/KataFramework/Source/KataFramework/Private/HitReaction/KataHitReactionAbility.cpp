#include "HitReaction/KataHitReactionAbility.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "Attributes/KataAttributeSet_Stance.h"
#include "Attributes/KataCombatSettings.h"
#include "GameFramework/Actor.h"
#include "GAS/AbilityTask_PlayKataAction.h"
#include "KataFrameworkLog.h"

namespace KataHitReaction
{
    /** 고른 방향의 데이터를 찾고, 비어 있으면 Front로 대신한다. */
    template <typename T>
    T* FindForDirection(const TMap<EKataHitDirection, TObjectPtr<T>>& Map, EKataHitDirection Direction)
    {
        if (const TObjectPtr<T>* Found = Map.Find(Direction); Found != nullptr && *Found != nullptr)
        {
            return Found->Get();
        }
        const TObjectPtr<T>* Front = Map.Find(EKataHitDirection::Front);
        return Front != nullptr ? Front->Get() : nullptr;
    }
}

UKataHitReactionAbility::UKataHitReactionAbility()
{
    // 반응마다 태스크와 콜백 상태를 가지므로 액터별 인스턴스를 쓴다.
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    // 경직 중 다시 맞으면 진행 중인 반응을 끝내고 새 방향으로 다시 시작한다.
    bRetriggerInstancedAbility = true;
}

EKataHitDirection UKataHitReactionAbility::ResolveHitDirection(const AActor* Victim, const FHitResult& HitResult, const AActor* Attacker,
    float MinHorizontalRatio)
{
    if (Victim == nullptr)
    {
        return EKataHitDirection::Front;
    }

    // 공격이 들어온 쪽을 가리키는 수평 방향. 맞은 쪽 위치 기준이다.
    FVector FromDirection = FVector::ZeroVector;
    const FVector Motion = HitResult.TraceEnd - HitResult.TraceStart;
    const FVector HorizontalMotion(Motion.X, Motion.Y, 0.0);
    const double MotionLength = Motion.Size();
    if (MotionLength > UE_KINDA_SMALL_NUMBER && HorizontalMotion.Size() >= MotionLength * MinHorizontalRatio)
    {
        // 무기는 공격이 들어온 쪽에서 반대쪽으로 지나간다. 좌우는 맞은 쪽 기준이며,
        // 무기가 맞은 쪽의 왼쪽에서 오른쪽으로 지나가면 왼쪽 피격이다.
        FromDirection = -HorizontalMotion.GetSafeNormal();
    }
    else if (Attacker != nullptr)
    {
        // 내려찍기처럼 좌우를 판단할 수 없으면 공격자가 선 쪽을 쓴다.
        FromDirection = (Attacker->GetActorLocation() - Victim->GetActorLocation()).GetSafeNormal2D();
    }

    if (FromDirection.IsNearlyZero())
    {
        return EKataHitDirection::Front;
    }

    const double ForwardDot = FVector::DotProduct(FromDirection, Victim->GetActorForwardVector().GetSafeNormal2D());
    const double RightDot = FVector::DotProduct(FromDirection, Victim->GetActorRightVector().GetSafeNormal2D());
    if (FMath::Abs(ForwardDot) >= FMath::Abs(RightDot))
    {
        return ForwardDot >= 0.0 ? EKataHitDirection::Front : EKataHitDirection::Back;
    }
    return RightDot >= 0.0 ? EKataHitDirection::Right : EKataHitDirection::Left;
}

void UKataHitReactionAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    AActor* Avatar = ActorInfo != nullptr ? ActorInfo->AvatarActor.Get() : nullptr;
    // FKataContext의 대상은 const가 아닌 액터를 받는다. 공격자를 대상으로 넘길 뿐 수정하지 않는다.
    AActor* Attacker = TriggerEventData != nullptr ? const_cast<AActor*>(TriggerEventData->Instigator.Get()) : nullptr;
    FHitResult HitResult;
    if (TriggerEventData != nullptr && TriggerEventData->ContextHandle.GetHitResult() != nullptr)
    {
        HitResult = *TriggerEventData->ContextHandle.GetHitResult();
    }

    const EKataHitDirection Direction = ResolveHitDirection(Avatar, HitResult, Attacker, UKataCombatSettings::Get()->MinHorizontalDirectionRatio);
    UE_LOG(LogKataFramework, Verbose, TEXT("Kata hit reaction ability '%s' on '%s': direction %s"),
        *GetName(), *GetNameSafe(Avatar), *UEnum::GetValueAsString(Direction));

    if (Mode == EKataHitReactionMode::KataAction)
    {
        UKataAction* Action = KataHitReaction::FindForDirection(DirectionalActions, Direction);
        if (Action == nullptr)
        {
            UE_LOG(LogKataFramework, Warning, TEXT("Kata hit reaction ability '%s' has no reaction action for %s or Front"),
                *GetName(), *UEnum::GetValueAsString(Direction));
            EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
            return;
        }

        // 반응 Kata는 공격자를 대상으로 받아 공격자 쪽을 바라보는 등의 태스크에 쓸 수 있다.
        UAbilityTask_PlayKataAction* Task = UAbilityTask_PlayKataAction::PlayKataAction(this, Action, Attacker);
        Task->OnCompleted.AddDynamic(this, &UKataHitReactionAbility::HandleKataEnded);
        Task->OnInterrupted.AddDynamic(this, &UKataHitReactionAbility::HandleKataEnded);
        Task->OnBranched.AddDynamic(this, &UKataHitReactionAbility::HandleKataEnded);
        Task->OnFailed.AddDynamic(this, &UKataHitReactionAbility::HandleKataFailed);
        Task->ReadyForActivation();
        return;
    }

    UAnimMontage* Montage = KataHitReaction::FindForDirection(DirectionalMontages, Direction);
    if (Montage == nullptr)
    {
        UE_LOG(LogKataFramework, Warning, TEXT("Kata hit reaction ability '%s' has no reaction montage for %s or Front"),
            *GetName(), *UEnum::GetValueAsString(Direction));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, Montage);
    Task->OnCompleted.AddDynamic(this, &UKataHitReactionAbility::HandleMontageEnded);
    Task->OnInterrupted.AddDynamic(this, &UKataHitReactionAbility::HandleMontageEnded);
    Task->OnCancelled.AddDynamic(this, &UKataHitReactionAbility::HandleMontageEnded);
    Task->ReadyForActivation();
}

void UKataHitReactionAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    if (bResetGroggyOnEnd && IsActive())
    {
        UAbilitySystemComponent* AbilitySystem = ActorInfo != nullptr ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
        if (AbilitySystem != nullptr && AbilitySystem->GetSet<UKataAttributeSet_Stance>() != nullptr)
        {
            AbilitySystem->SetNumericAttributeBase(UKataAttributeSet_Stance::GetGroggyAttribute(), 0.0f);
        }
    }

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UKataHitReactionAbility::HandleKataEnded(EKataEndReason EndReason)
{
    FinishReaction();
}

void UKataHitReactionAbility::HandleKataFailed(EKataStartResult FailureReason)
{
    // 반응 Kata가 차단 정책 등으로 거절되면 반응 없이 끝낸다. 상태 태그가 남지 않도록 바로 종료한다.
    UE_LOG(LogKataFramework, Verbose, TEXT("Kata hit reaction ability '%s' could not start its action (result %d)"),
        *GetName(), static_cast<int32>(FailureReason));
    FinishReaction();
}

void UKataHitReactionAbility::HandleMontageEnded()
{
    FinishReaction();
}

void UKataHitReactionAbility::FinishReaction()
{
    EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}
