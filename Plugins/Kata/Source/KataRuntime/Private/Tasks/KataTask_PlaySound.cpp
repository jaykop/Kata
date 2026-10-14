#include "Tasks/KataTask_PlaySound.h"

#include "Components/AudioComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "KataRuntimeLog.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

UKataTask_PlaySound::UKataTask_PlaySound()
{
    // 효과음은 같은 시각의 판정·애니메이션 요청이 끝난 뒤 표현 단계에서 낸다.
    Phase = EKataTaskPhase::Presentation;
}

TSubclassOf<UKataTaskInstance> UKataTask_PlaySound::GetTaskInstanceClass_Implementation() const
{
    return UKataTaskInstance_PlaySound::StaticClass();
}

FName UKataTask_PlaySound::GetConfigurationError() const
{
    const FName SuperError = Super::GetConfigurationError();
    if (!SuperError.IsNone())
    {
        return SuperError;
    }
    if (Sound == nullptr)
    {
        return TEXT("MissingSound");
    }
    if (!(PitchMultiplier > 0.0f) || !FMath::IsFinite(PitchMultiplier))
    {
        return TEXT("InvalidPitchMultiplier");
    }
    return NAME_None;
}

FString UKataTask_PlaySound::DescribeConfigurationError(FName ErrorCode) const
{
    if (ErrorCode == TEXT("MissingSound"))
    {
        return TEXT("'Sound' is not set");
    }
    if (ErrorCode == TEXT("InvalidPitchMultiplier"))
    {
        return TEXT("'Pitch Multiplier' must be greater than zero");
    }
    return Super::DescribeConfigurationError(ErrorCode);
}

#if WITH_EDITOR
EDataValidationResult UKataTask_PlaySound::IsDataValid(FDataValidationContext& Context) const
{
    const EDataValidationResult Result = Super::IsDataValid(Context);
    if (Sound != nullptr && Sound->IsLooping() && CompletedPolicy == EKataEffectEndPolicy::KeepPlaying)
    {
        Context.AddWarning(FText::FromString(FString::Printf(
            TEXT("Sound '%s' loops, so Keep Playing never lets it finish. It will be stopped when the task completes."),
            *GetNameSafe(Sound))));
    }
    return Result;
}
#endif

USceneComponent* UKataTaskInstance_PlaySound::ResolveAttachParent() const
{
    const FKataContext Context = GetKataContext();
    AActor* Avatar = Context.GetAvatarActor();
    if (Avatar == nullptr)
    {
        return nullptr;
    }
    if (USkeletalMeshComponent* Mesh = Avatar->FindComponentByClass<USkeletalMeshComponent>())
    {
        return Mesh;
    }
    return Avatar->GetRootComponent();
}

void UKataTaskInstance_PlaySound::OnTaskStarted_Implementation()
{
    // 재생하지 못해도 태스크를 일찍 완료하지 않는다. 프리뷰 Seek처럼 소리만 막힌 경우에도
    // 의존성으로 이어진 태스크의 시작 시각이 실제 재생 때와 같아야 하기 때문이다.
    const UKataTask_PlaySound* Definition = Cast<UKataTask_PlaySound>(GetTaskDefinition());
    UWorld* World = GetWorld();
    if (Definition == nullptr || Definition->Sound == nullptr || World == nullptr || !World->bAllowAudioPlayback)
    {
        return;
    }

    USceneComponent* AttachParent = ResolveAttachParent();
    if (AttachParent == nullptr)
    {
        UE_LOG(LogKata, Warning, TEXT("Kata sound task '%s' found no avatar component to play '%s' from"),
            *GetDisplayName(), *GetNameSafe(Definition->Sound));
        return;
    }

    // UGameplayStatics의 생성 함수는 만들자마자 재생하므로 직접 만든다.
    // 입력 값을 재생 전에 넣어야 MetaSound가 첫 오디오 블록부터 그 값을 쓴다.
    UAudioComponent* Audio = NewObject<UAudioComponent>(AttachParent->GetOwner());
    Audio->SetSound(Definition->Sound);
    Audio->bAutoDestroy = true;
    Audio->SetVolumeMultiplier(Definition->VolumeMultiplier);
    Audio->SetPitchMultiplier(Definition->PitchMultiplier);
    if (Definition->AttenuationOverride != nullptr)
    {
        Audio->AttenuationSettings = Definition->AttenuationOverride;
    }
    Audio->DefaultParameters = Definition->AudioParameters;
    if (World->WorldType == EWorldType::EditorPreview)
    {
        // 액션 편집기 프리뷰에서는 일반 게임 사운드가 들리지 않았다(PIE에서는 들림).
        // 엔진의 AnimNotify_PlaySound가 프리뷰에서 감쇠 없는 2D 재생(PlaySound2D, UI 사운드)을 쓰는 것과 같게,
        // 게임이 진행하지 않는 동안에도 재생되는 UI 사운드로 표시하고 리스너 위치와 무관하게 공간화를 끈다.
        Audio->bIsUISound = true;
        Audio->bAllowSpatialization = false;
    }

    if (Definition->bAttachToMesh)
    {
        Audio->SetupAttachment(AttachParent, Definition->AttachSocket);
    }
    else
    {
        // 부모 없이 등록하면 이후 소유 액터가 움직여도 시작 위치에 남는다.
        Audio->SetWorldLocation(AttachParent->GetSocketLocation(Definition->AttachSocket));
    }
    Audio->RegisterComponent();
    Audio->Play();
    AudioComponent = Audio;
}

void UKataTaskInstance_PlaySound::OnTaskEnded_Implementation(EKataTaskEndReason Reason)
{
    UAudioComponent* Audio = AudioComponent.Get();
    AudioComponent.Reset();
    if (Audio == nullptr || !Audio->IsPlaying())
    {
        return;
    }

    const UKataTask_PlaySound* Definition = Cast<UKataTask_PlaySound>(GetTaskDefinition());
    // 루프 사운드는 스스로 끝나지 않으므로 Keep Playing이어도 멈춘다.
    const bool bKeepPlaying = Reason == EKataTaskEndReason::Completed
        && Definition != nullptr
        && Definition->CompletedPolicy == EKataEffectEndPolicy::KeepPlaying
        && !(Audio->Sound != nullptr && Audio->Sound->IsLooping());
    if (bKeepPlaying)
    {
        return;
    }

    const float FadeOutTime = Definition != nullptr ? Definition->FadeOutTime : 0.0f;
    if (FadeOutTime > 0.0f)
    {
        Audio->FadeOut(FadeOutTime, 0.0f);
    }
    else
    {
        Audio->Stop();
    }
}
