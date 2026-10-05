#include "KataCameraPlacement.h"

#include "KataCameraTypes.h"
#include "KataCameraData.h"
#include "KataCameraRailComponent.h"

namespace
{
    void EvaluateBoomArm(FKataCameraPipelineContext& Context, float Distance)
    {
        const FRotator YawRotation(0.0, Context.ViewRotation.Yaw, 0.0);
        Context.CameraRotation = Context.ViewRotation;
        Context.CameraLocation = Context.PivotLocation - Context.ViewRotation.Vector() * Distance;
        Context.OrbitOffset = YawRotation.UnrotateVector(Context.CameraLocation - Context.PivotLocation);
        Context.AimOffset = FVector::ZeroVector;
    }
}

void UKataCameraPlacement_BoomArm::Evaluate(FKataCameraPipelineContext& Context) const
{
    // 매니저는 뷰 타깃 위치를 피벗으로 넘긴다. 피벗 오프셋은 Boom Arm만의 설정이므로 여기서 더한다.
    const FVector PlanarOffset = FRotator(0.0, Context.ViewRotation.Yaw, 0.0).RotateVector(FVector(PivotOffset.X, PivotOffset.Y, 0.0));
    Context.PivotLocation += PlanarOffset + FVector(0.0, 0.0, PivotOffset.Z);
    EvaluateBoomArm(Context, Distance);
}

void UKataCameraPlacement_Spline::Evaluate(FKataCameraPipelineContext& Context) const
{
    if (Context.RailStatus == EKataCameraRailStatus::Ready && Context.ViewRotation.ContainsNaN())
    {
        Context.RailStatus = EKataCameraRailStatus::InvalidSample;
    }
    if (Context.RailStatus == EKataCameraRailStatus::Ready && Context.Rail != nullptr)
    {
        const float InputPitch = FRotator::NormalizeAxis(Context.ViewRotation.Pitch);
        Context.RailAlpha = FMath::Clamp((InputPitch - Context.CameraData->PitchMin) /
            (Context.CameraData->PitchMax - Context.CameraData->PitchMin), 0.0f, 1.0f);
        Context.RailDistance = Context.RailAlpha * Context.Rail->GetSplineLength();

        const FVector OrbitOffset = Context.Rail->GetOrbitOffsetAtDistance(Context.RailDistance);
        const FVector AimOffset = AimOffsetCurve.GetValue(Context.RailAlpha);
        const FVector RailPivot = Context.Rail->GetComponentLocation();
        const FRotator YawRotation(0.0, Context.ViewRotation.Yaw, 0.0);
        const FRichCurve* FOVCurve = FieldOfViewCurve.GetRichCurveConst();
        const float FieldOfView = FOVCurve->HasAnyData() ? FOVCurve->Eval(Context.RailAlpha) : Context.FieldOfView;

        if (FMath::IsFinite(InputPitch) && FMath::IsFinite(Context.ViewRotation.Yaw) &&
            !OrbitOffset.ContainsNaN() && !AimOffset.ContainsNaN() && !RailPivot.ContainsNaN() && FMath::IsFinite(FieldOfView))
        {
            Context.PivotLocation = RailPivot;
            Context.OrbitOffset = OrbitOffset;
            Context.AimOffset = AimOffset;
            Context.CameraLocation = RailPivot + YawRotation.RotateVector(OrbitOffset);
            const FVector AimDirection = RailPivot + YawRotation.RotateVector(AimOffset) - Context.CameraLocation;
            // 위치와 조준점이 같으면 시선 방향을 구할 수 없으므로 입력 회전을 유지한다.
            Context.CameraRotation = AimDirection.IsNearlyZero() ? Context.ViewRotation : AimDirection.Rotation();
            Context.FieldOfView = FMath::Clamp(FieldOfView, 5.0f, 170.0f);
            return;
        }

        Context.RailStatus = EKataCameraRailStatus::InvalidSample;
    }

    // 실패 경로에서는 매니저가 넘긴 뷰 타깃 위치 피벗과 기본 FOV를 유지한다.
    const float SafeDistance = FMath::IsFinite(FallbackDistance) ? FMath::Max(FallbackDistance, 0.0f) : 400.0f;
    EvaluateBoomArm(Context, SafeDistance);
}
