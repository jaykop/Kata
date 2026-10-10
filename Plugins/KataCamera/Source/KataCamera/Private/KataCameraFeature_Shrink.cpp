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
    bLastPivotBlocked = false;
    LastViewTarget.Reset();

    Super::Deinitialize();
}

void UKataCameraFeature_Shrink::Evaluate(FKataCameraPipelineContext& Context)
{
    UWorld* World = Context.ViewTarget != nullptr ? Context.ViewTarget->GetWorld() : nullptr;
    if (World == nullptr || Context.PivotLocation.ContainsNaN() || Context.CameraLocation.ContainsNaN())
    {
        bLastBlocked = false;
        bLastPivotBlocked = false;
        return;
    }

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(KataCameraShrink), false);
    QueryParams.AddIgnoredActor(Context.ViewTarget);
    if (Context.PlayerController != nullptr)
    {
        QueryParams.AddIgnoredActor(Context.PlayerController);
        QueryParams.AddIgnoredActor(Context.PlayerController->PlayerCameraManager);
    }

    // 피벗에는 배치 오프셋·레일 원점·피벗 래그가 더해져 있어 폰이 벽에 붙으면 벽 안이나 너머로 나갈 수 있다.
    // 캡슐 안에 있는 뷰 타깃 위치에서 피벗까지 먼저 스윕해, 막히면 이번 프레임의 스윕 시작점을 충돌 지점으로 당긴다.
    // 피벗이 이미 장애물 밖이면 결과가 기존과 같고, 회전·조준은 바꾸지 않으므로 Context.PivotLocation은 그대로 둔다.
    const FVector Anchor = Context.ViewTarget->GetActorLocation();
    FVector Pivot = Context.PivotLocation;
    bLastPivotBlocked = false;
    if (!FVector::PointsAreNear(Anchor, Pivot, UE_KINDA_SMALL_NUMBER))
    {
        FHitResult PivotHit;
        if (World->SweepSingleByChannel(
            PivotHit, Anchor, Pivot, FQuat::Identity, ProbeChannel, FCollisionShape::MakeSphere(ProbeRadius), QueryParams))
        {
            Pivot = FMath::Lerp(Anchor, Pivot, static_cast<double>(PivotHit.Time));
            bLastPivotBlocked = true;
        }
    }

    const FVector ToCamera = Context.CameraLocation - Pivot;
    const double FullDistance = ToCamera.Size();
    if (FullDistance <= UE_KINDA_SMALL_NUMBER)
    {
        bLastBlocked = false;
        return;
    }

    FHitResult Hit;
    const bool bBlocked = World->SweepSingleByChannel(
        Hit, Pivot, Context.CameraLocation, FQuat::Identity, ProbeChannel, FCollisionShape::MakeSphere(ProbeRadius), QueryParams);
    bLastBlocked = bBlocked;

    // 시작부터 겹친 경우 Time이 0이므로 카메라가 MinDistance까지 당겨진다. 앞 단계에서 피벗을 장애물 밖으로 당겼으므로
    // 이 경로는 뷰 타깃 위치 자체가 지형에 걸친 경우에만 남는다.
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
    return FString::Printf(TEXT("ratio=%.2f %s%s"), CurrentRatio, bLastBlocked ? TEXT("blocked") : TEXT("clear"),
        bLastPivotBlocked ? TEXT(" pivot-blocked") : TEXT(""));
}
