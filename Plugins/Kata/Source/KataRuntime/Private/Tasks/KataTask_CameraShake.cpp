#include "Tasks/KataTask_CameraShake.h"

#include "Camera/CameraShakeBase.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

namespace
{
    /**
     * 진원에서 카메라까지의 거리로 흔들림 배율(0~1)을 구한다.
     * 엔진의 APlayerCameraManager::CalcRadialShakeScale은 protected라서 같은 규칙을 여기서 계산한다.
     */
    float CalcRadialShakeScale(const APlayerCameraManager& CameraManager, const FVector& Epicenter, float InnerRadius, float OuterRadius, float Falloff)
    {
        const float Distance = FVector::Dist(Epicenter, CameraManager.GetCameraLocation());
        if (Distance <= InnerRadius)
        {
            return 1.0f;
        }
        if (OuterRadius <= InnerRadius || Distance >= OuterRadius)
        {
            return 0.0f;
        }
        const float Alpha = 1.0f - (Distance - InnerRadius) / (OuterRadius - InnerRadius);
        return FMath::Pow(Alpha, Falloff);
    }
}

UKataTask_CameraShake::UKataTask_CameraShake()
{
    Phase = EKataTaskPhase::Presentation;
}

TSubclassOf<UKataTaskInstance> UKataTask_CameraShake::GetTaskInstanceClass_Implementation() const
{
    return UKataTaskInstance_CameraShake::StaticClass();
}

FName UKataTask_CameraShake::GetConfigurationError() const
{
    const FName SuperError = Super::GetConfigurationError();
    if (!SuperError.IsNone())
    {
        return SuperError;
    }
    if (ShakeClass == nullptr)
    {
        return TEXT("MissingShakeClass");
    }
    if (Target == EKataCameraShakeTarget::LocalPlayersInRadius && OuterRadius < InnerRadius)
    {
        return TEXT("InvalidShakeRadius");
    }
    return NAME_None;
}

FString UKataTask_CameraShake::DescribeConfigurationError(FName ErrorCode) const
{
    if (ErrorCode == TEXT("MissingShakeClass"))
    {
        return TEXT("'Shake Class' is not set");
    }
    if (ErrorCode == TEXT("InvalidShakeRadius"))
    {
        return TEXT("'Outer Radius' must be greater than or equal to 'Inner Radius'");
    }
    return Super::DescribeConfigurationError(ErrorCode);
}

bool UKataTask_CameraShake::IsInfiniteShake() const
{
    FCameraShakeDuration ShakeDuration;
    return ShakeClass != nullptr
        && UCameraShakeBase::GetCameraShakeDuration(ShakeClass, ShakeDuration)
        && ShakeDuration.IsInfinite();
}

#if WITH_EDITOR
EDataValidationResult UKataTask_CameraShake::IsDataValid(FDataValidationContext& Context) const
{
    const EDataValidationResult Result = Super::IsDataValid(Context);
    if (CompletedPolicy == EKataEffectEndPolicy::KeepPlaying && IsInfiniteShake())
    {
        Context.AddWarning(FText::FromString(FString::Printf(
            TEXT("Camera shake '%s' has an infinite duration, so Keep Playing never lets it finish. It will be stopped when the task completes."),
            *GetNameSafe(ShakeClass.Get()))));
    }
    return Result;
}
#endif

void UKataTaskInstance_CameraShake::StartShakeOn(APlayerCameraManager* CameraManager, float ShakeScale)
{
    const UKataTask_CameraShake* Definition = Cast<UKataTask_CameraShake>(GetTaskDefinition());
    if (CameraManager == nullptr || Definition == nullptr || ShakeScale <= 0.0f)
    {
        return;
    }
    if (UCameraShakeBase* Shake = CameraManager->StartCameraShake(Definition->ShakeClass, ShakeScale))
    {
        StartedShakes.Add({ CameraManager, Shake });
    }
}

void UKataTaskInstance_CameraShake::OnTaskStarted_Implementation()
{
    const UKataTask_CameraShake* Definition = Cast<UKataTask_CameraShake>(GetTaskDefinition());
    UWorld* World = GetWorld();
    AActor* Avatar = GetKataContext().GetAvatarActor();
    if (Definition == nullptr || Definition->ShakeClass == nullptr || World == nullptr || Avatar == nullptr)
    {
        return;
    }

    if (Definition->Target == EKataCameraShakeTarget::AvatarCamera)
    {
        const APawn* Pawn = Cast<APawn>(Avatar);
        APlayerController* PlayerController = Pawn != nullptr ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
        if (PlayerController != nullptr && PlayerController->IsLocalController())
        {
            StartShakeOn(PlayerController->PlayerCameraManager, Definition->Scale);
        }
        return;
    }

    // 엔진의 PlayWorldCameraShake는 시작한 인스턴스를 돌려주지 않아 멈출 수 없으므로, 카메라마다 직접 시작해 인스턴스를 받는다.
    const FVector Epicenter = Avatar->GetActorLocation();
    for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
    {
        APlayerController* PlayerController = It->Get();
        if (PlayerController == nullptr || !PlayerController->IsLocalController() || PlayerController->PlayerCameraManager == nullptr)
        {
            continue;
        }
        const float RadialScale = CalcRadialShakeScale(
            *PlayerController->PlayerCameraManager, Epicenter, Definition->InnerRadius, Definition->OuterRadius, Definition->Falloff);
        StartShakeOn(PlayerController->PlayerCameraManager, Definition->Scale * RadialScale);
    }
}

void UKataTaskInstance_CameraShake::OnTaskTick_Implementation(float DeltaTime)
{
    ReleaseFinishedShakes();
}

void UKataTaskInstance_CameraShake::ReleaseFinishedShakes()
{
    StartedShakes.RemoveAll([](const FKataStartedCameraShake& Started)
    {
        const UCameraShakeBase* Shake = Started.Shake.Get();
        return Shake == nullptr || !Shake->IsActive() || Shake->IsFinished();
    });
}

void UKataTaskInstance_CameraShake::OnTaskEnded_Implementation(EKataTaskEndReason Reason)
{
    ReleaseFinishedShakes();
    const UKataTask_CameraShake* Definition = Cast<UKataTask_CameraShake>(GetTaskDefinition());
    // 무한 흔들림은 스스로 끝나지 않으므로 Keep Playing이어도 멈춘다.
    const bool bKeepPlaying = Reason == EKataTaskEndReason::Completed
        && Definition != nullptr
        && Definition->CompletedPolicy == EKataEffectEndPolicy::KeepPlaying
        && !Definition->IsInfiniteShake();
    if (!bKeepPlaying)
    {
        for (const FKataStartedCameraShake& Started : StartedShakes)
        {
            APlayerCameraManager* CameraManager = Started.CameraManager.Get();
            UCameraShakeBase* Shake = Started.Shake.Get();
            if (CameraManager != nullptr && Shake != nullptr)
            {
                // 즉시 끊지 않고 셰이크의 블렌드 아웃으로 멈춘다.
                CameraManager->StopCameraShake(Shake, false);
            }
        }
    }
    StartedShakes.Reset();
}
