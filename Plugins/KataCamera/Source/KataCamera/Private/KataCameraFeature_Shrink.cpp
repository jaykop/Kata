#include "KataCameraFeature_Shrink.h"

#include "CollisionQueryParams.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"

UKataCameraFeature_Shrink::UKataCameraFeature_Shrink()
{
    Stage = EKataCameraStage::Constraint;
}

void UKataCameraFeature_Shrink::Deinitialize()
{
    CurrentRatio = 1.0f;
    bLastBlocked = false;
    LastViewTarget.Reset();

    Super::Deinitialize();
}

void UKataCameraFeature_Shrink::Evaluate(FKataCameraPipelineContext& Context)
{
    const FVector Pivot = Context.PivotLocation;
    const FVector ToCamera = Context.CameraLocation - Pivot;
    const double FullDistance = ToCamera.Size();
    UWorld* World = Context.ViewTarget != nullptr ? Context.ViewTarget->GetWorld() : nullptr;
    if (World == nullptr || FullDistance <= UE_KINDA_SMALL_NUMBER || ToCamera.ContainsNaN())
    {
        bLastBlocked = false;
        return;
    }

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(KataCameraShrink), false);
    QueryParams.AddIgnoredActor(Context.ViewTarget);
    if (Context.PlayerController != nullptr)
    {
        QueryParams.AddIgnoredActor(Context.PlayerController);
        QueryParams.AddIgnoredActor(Context.PlayerController->PlayerCameraManager);
    }

    FHitResult Hit;
    const bool bBlocked = World->SweepSingleByChannel(
        Hit, Pivot, Context.CameraLocation, FQuat::Identity, ProbeChannel, FCollisionShape::MakeSphere(ProbeRadius), QueryParams);
    bLastBlocked = bBlocked;

    // 시작부터 겹친 경우 Time이 0이므로 카메라가 MinDistance까지 당겨진다. 피벗이 지형 안에 있는 설정 오류를 드러내는 쪽을 택했다.
    const double MinRatio = FMath::Clamp(MinDistance / FullDistance, 0.0, 1.0);
    const double TargetRatio = bBlocked ? FMath::Clamp(static_cast<double>(Hit.Time), MinRatio, 1.0) : 1.0;

    if (LastViewTarget.Get() != Context.ViewTarget || Context.DeltaTime <= 0.0f)
    {
        CurrentRatio = TargetRatio;
        LastViewTarget = Context.ViewTarget;
    }
    else if (TargetRatio < CurrentRatio)
    {
        CurrentRatio = PullInInterpSpeed > 0.0f
            ? FMath::FInterpTo(CurrentRatio, static_cast<float>(TargetRatio), Context.DeltaTime, PullInInterpSpeed)
            : static_cast<float>(TargetRatio);
    }
    else
    {
        CurrentRatio = RecoverInterpSpeed > 0.0f
            ? FMath::FInterpTo(CurrentRatio, static_cast<float>(TargetRatio), Context.DeltaTime, RecoverInterpSpeed)
            : static_cast<float>(TargetRatio);
    }

    Context.CameraLocation = Pivot + ToCamera * CurrentRatio;
}

FString UKataCameraFeature_Shrink::GetDebugString() const
{
    return FString::Printf(TEXT("ratio=%.2f %s"), CurrentRatio, bLastBlocked ? TEXT("blocked") : TEXT("clear"));
}
