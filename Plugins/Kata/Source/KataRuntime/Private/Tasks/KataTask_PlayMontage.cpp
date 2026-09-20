#include "Tasks/KataTask_PlayMontage.h"

#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#include "KataRuntimeLog.h"
#include "Runtime/KataInstance.h"

UKataTask_PlayMontage::UKataTask_PlayMontage()
{
    // 몽타주 재생은 애니메이션 단계에서 처리한다.
    Phase = EKataTaskPhase::Animation;
}

TSubclassOf<UKataTaskInstance> UKataTask_PlayMontage::GetTaskInstanceClass_Implementation() const
{
    return UKataTaskInstance_PlayMontage::StaticClass();
}

FName UKataTask_PlayMontage::GetConfigurationError() const
{
    const FName SuperError = Super::GetConfigurationError();
    if (!SuperError.IsNone())
    {
        return SuperError;
    }
    if (Montage == nullptr)
    {
        return TEXT("MissingMontage");
    }
    if (!(PlayRate > 0.0f) || !FMath::IsFinite(PlayRate))
    {
        return TEXT("InvalidPlayRate");
    }
    return NAME_None;
}

FString UKataTask_PlayMontage::DescribeConfigurationError(FName ErrorCode) const
{
    if (ErrorCode == TEXT("MissingMontage"))
    {
        return TEXT("'Montage' is not set");
    }
    if (ErrorCode == TEXT("InvalidPlayRate"))
    {
        return TEXT("'Play Rate' must be greater than zero");
    }
    return Super::DescribeConfigurationError(ErrorCode);
}

UAnimInstance* UKataTaskInstance_PlayMontage::ResolveAnimInstance() const
{
    const FKataContext Context = GetKataContext();
    AActor* Avatar = Context.GetAvatarActor();
    if (Avatar == nullptr)
    {
        return nullptr;
    }

    const USkeletalMeshComponent* Mesh = Avatar->FindComponentByClass<USkeletalMeshComponent>();
    return Mesh != nullptr ? Mesh->GetAnimInstance() : nullptr;
}

void UKataTaskInstance_PlayMontage::OnTaskStarted_Implementation()
{
    const UKataTask_PlayMontage* Definition = Cast<UKataTask_PlayMontage>(GetTaskDefinition());
    if (Definition == nullptr || Definition->Montage == nullptr)
    {
        FinishTask();
        return;
    }

    UAnimInstance* AnimInstance = ResolveAnimInstance();
    if (AnimInstance == nullptr)
    {
        UE_LOG(LogKata, Warning, TEXT("Kata montage task '%s' found no anim instance on the avatar actor"), *GetDisplayName());
        FinishTask();
        return;
    }

    const FKataContext Context = GetKataContext();
    UAbilitySystemComponent* AbilitySystem = Context.ResolveAbilitySystem();
    UGameplayAbility* OwningAbility = Context.OwningAbility.Get();

    float PlayLength = 0.0f;
    if (AbilitySystem != nullptr && OwningAbility != nullptr)
    {
        // Ability에서 시작한 경우 ASC 경로로 재생해 GAS의 몽타주 관리와 어긋나지 않게 한다.
        PlayLength = AbilitySystem->PlayMontage(
            OwningAbility,
            OwningAbility->GetCurrentActivationInfo(),
            Definition->Montage,
            Definition->PlayRate,
            Definition->StartSection);
    }
    else
    {
        PlayLength = AnimInstance->Montage_Play(Definition->Montage, Definition->PlayRate);
        if (PlayLength > 0.0f && !Definition->StartSection.IsNone())
        {
            AnimInstance->Montage_JumpToSection(Definition->StartSection, Definition->Montage);
        }
    }

    if (PlayLength <= 0.0f)
    {
        // 재생 실패를 성공으로 감추지 않는다. 태스크만 완료하고 타임라인은 계속 진행한다.
        UE_LOG(LogKata, Warning, TEXT("Kata montage task '%s' failed to play montage '%s'"),
            *GetDisplayName(), *GetNameSafe(Definition->Montage));
        FinishTask();
        return;
    }

    PlayingMontage = Definition->Montage;
    if (const FAnimMontageInstance* MontageInstance = AnimInstance->GetActiveInstanceForMontage(Definition->Montage))
    {
        PlayingMontageInstanceId = MontageInstance->GetInstanceID();
    }

    FOnMontageEnded EndedDelegate;
    EndedDelegate.BindUObject(this, &UKataTaskInstance_PlayMontage::HandleMontageEnded);
    AnimInstance->Montage_SetEndDelegate(EndedDelegate, Definition->Montage);
    bMontageEndDelegateBound = true;
}

void UKataTaskInstance_PlayMontage::HandleMontageEnded(UAnimMontage* EndedMontage, bool bInterrupted)
{
    if (EndedMontage != PlayingMontage)
    {
        return;
    }

    bMontageEndDelegateBound = false;

    const UKataTask_PlayMontage* Definition = Cast<UKataTask_PlayMontage>(GetTaskDefinition());
    const EKataMontageEndPolicy Policy = Definition != nullptr ? Definition->MontageEndPolicy : EKataMontageEndPolicy::ContinueTimeline;

    switch (Policy)
    {
    case EKataMontageEndPolicy::FinishTaskOnMontageEnd:
        FinishTask();
        break;

    case EKataMontageEndPolicy::EndKataOnMontageEnd:
        if (UKataInstance* Instance = GetKataInstance())
        {
            Instance->RequestEnd(bInterrupted ? EKataEndReason::Interrupted : EKataEndReason::Completed);
        }
        break;

    case EKataMontageEndPolicy::ContinueTimeline:
    default:
        // 몽타주 종료는 Kata 완료가 아니다. 타임라인의 지속 시간을 그대로 따른다.
        break;
    }
}

void UKataTaskInstance_PlayMontage::OnTaskEnded_Implementation(EKataTaskEndReason Reason)
{
    UAnimInstance* AnimInstance = ResolveAnimInstance();
    const UKataTask_PlayMontage* Definition = Cast<UKataTask_PlayMontage>(GetTaskDefinition());

    if (AnimInstance != nullptr && PlayingMontage != nullptr)
    {
        if (bMontageEndDelegateBound)
        {
            // 구독을 비워 종료 후 콜백이 들어오지 않게 한다.
            FOnMontageEnded EmptyDelegate;
            AnimInstance->Montage_SetEndDelegate(EmptyDelegate, PlayingMontage);
            bMontageEndDelegateBound = false;
        }

        const FAnimMontageInstance* MontageInstance = AnimInstance->GetActiveInstanceForMontage(PlayingMontage);
        const bool bStillOurPlayback = MontageInstance != nullptr && MontageInstance->GetInstanceID() == PlayingMontageInstanceId;

        if (bStillOurPlayback && Definition != nullptr && Definition->bStopMontageWhenTaskEnds)
        {
            // 다른 곳에서 새로 시작한 재생은 건드리지 않는다.
            const float BlendOutTime = Definition->StopBlendOutTime >= 0.0f
                ? Definition->StopBlendOutTime
                : PlayingMontage->BlendOut.GetBlendTime();
            AnimInstance->Montage_Stop(BlendOutTime, PlayingMontage);
        }
    }

    PlayingMontage = nullptr;
    PlayingMontageInstanceId = INDEX_NONE;
}
