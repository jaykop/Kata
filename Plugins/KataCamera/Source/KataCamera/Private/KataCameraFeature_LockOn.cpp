#include "KataCameraFeature_LockOn.h"

#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "KataCameraData.h"
#include "KataCameraPlacement.h"
#include "KataPlayerCameraManager.h"

UKataCameraFeature_LockOnRotation::UKataCameraFeature_LockOnRotation()
{
    Stage = EKataCameraStage::Rotation;
}

void UKataCameraFeature_LockOnRotation::Evaluate(FKataCameraPipelineContext& Context)
{
    const AKataPlayerCameraManager* Manager = GetCameraManager();
    if (!Context.bLockOnActive || Manager == nullptr)
    {
        return;
    }
    Context.ViewRotation = Manager->GetLockOnViewRotation();
    // 컨트롤 회전은 궤도 입력값이다. 매 프레임 기록해 두면 해제 시 플레이어 입력이 마지막 락온 회전에서 이어진다.
    if (Context.PlayerController != nullptr)
    {
        Context.PlayerController->SetControlRotation(Context.ViewRotation);
    }
}

UKataCameraFeature_LockOnFraming::UKataCameraFeature_LockOnFraming()
{
    Stage = EKataCameraStage::Framing;
}

void UKataCameraFeature_LockOnFraming::Deinitialize()
{
    bHasReleaseOffset = false;
    Super::Deinitialize();
}

void UKataCameraFeature_LockOnFraming::Evaluate(FKataCameraPipelineContext& Context)
{
    if (Context.LockOnWeight <= 0.0f || GetCameraManager() == nullptr || Context.ViewTarget == nullptr)
    {
        bHasReleaseOffset = false;
        return;
    }

    const FRotator YawRotation(0.0, Context.ViewRotation.Yaw, 0.0);
    if (!Context.bLockOnActive)
    {
        // 해제 중에는 시점 입력이 회전을 정한다. 마지막 보정량을 시점에 붙여 줄이기만 하므로 마우스를 움직여도 카메라가 함께 돈다.
        if (bHasReleaseOffset && ReleaseWeight > 0.0f)
        {
            const double Scale = FMath::Clamp(Context.LockOnWeight / ReleaseWeight, 0.0f, 1.0f);
            Context.CameraLocation += YawRotation.RotateVector(ReleaseLocationOffset) * Scale;
            Context.CameraRotation.Pitch += ReleaseRotationOffset.Pitch * Scale;
            Context.CameraRotation.Yaw += ReleaseRotationOffset.Yaw * Scale;
        }
        return;
    }

    const FVector BaseLocation = Context.CameraLocation;
    const FRotator BaseRotation = Context.CameraRotation;
    ApplyFraming(Context);
    ReleaseLocationOffset = YawRotation.UnrotateVector(Context.CameraLocation - BaseLocation);
    ReleaseRotationOffset = FRotator(FMath::FindDeltaAngleDegrees(BaseRotation.Pitch, Context.CameraRotation.Pitch),
        FMath::FindDeltaAngleDegrees(BaseRotation.Yaw, Context.CameraRotation.Yaw), 0.0);
    ReleaseWeight = Context.LockOnWeight;
    bHasReleaseOffset = true;
}

void UKataCameraFeature_LockOnFraming::ApplyFraming(FKataCameraPipelineContext& Context) const
{
    const FKataLockOnFramingSettings& Settings = GetCameraManager()->GetLockOnSettings();
    const float Weight = Context.LockOnWeight;
    const double Side = FMath::Clamp(Context.LockOnSide, -1.0f, 1.0f);

    // SideOffset은 플레이어를 지나는 시선 평면에서 잰 최종 거리다. 피벗 오프셋·레일이 이미 옆으로 비킨 만큼은 빼고 맞춘다.
    const FVector Right = FRotator(0.0, Context.ViewRotation.Yaw, 0.0).RotateVector(FVector::RightVector);
    // 거리 배율은 Boom Arm에만 적용한다. Spline 오프셋을 늘이면 카메라가 레일을 벗어난다. 옆 오프셋 보정 전에 해야 옆 거리가 배율에 영향받지 않는다.
    const bool bBoomArm = Context.CameraData != nullptr && Cast<UKataCameraPlacement_BoomArm>(Context.CameraData->Placement) != nullptr;
    if (bBoomArm && !FMath::IsNearlyEqual(Context.LockOnDistanceScale, 1.0f))
    {
        const double Scale = FMath::Lerp(1.0, static_cast<double>(Context.LockOnDistanceScale), static_cast<double>(Weight));
        Context.CameraLocation = Context.PivotLocation + (Context.CameraLocation - Context.PivotLocation) * Scale;
    }

    // 기준점에 피벗 래그를 더해야 래그가 걸러 낸 좌우 흔들림을 여기서 다시 맞추지 않는다.
    const double CurrentLateral = FVector::DotProduct(Context.CameraLocation - (Context.ViewTarget->GetActorLocation() + Context.PivotLagOffset), Right);
    const double DesiredLateral = Side * Settings.SideOffset;
    Context.CameraLocation += Right * ((DesiredLateral - CurrentLateral) * Weight);

    // 조준점은 피벗과 락온 지점을 잇는 선 위에 둔다. 카메라 위치를 기준으로 쓰면 LookAtAlpha가 0에 가까울 때 자기 자신을 보게 된다.
    FVector AimPoint = FMath::Lerp(Context.PivotLocation, Context.LockFocusLocation, static_cast<double>(Settings.LookAtAlpha));
    if (Settings.MaxLookDistance > 0.0f && Settings.LookAtAlpha < 1.0f)
    {
        AimPoint = Context.PivotLocation + (AimPoint - Context.PivotLocation).GetClampedToMaxSize(Settings.MaxLookDistance);
    }
    Context.LockOnAimPoint = AimPoint;
    const FVector ToAim = AimPoint - Context.CameraLocation;
    if (ToAim.IsNearlyZero())
    {
        return;
    }

    int32 Width = 0;
    int32 Height = 0;
    if (Context.PlayerController != nullptr)
    {
        Context.PlayerController->GetViewportSize(Width, Height);
    }
    const double Aspect = Width > 0 && Height > 0 ? static_cast<double>(Width) / Height : 16.0 / 9.0;
    const double TanHalfFOV = FMath::Tan(FMath::DegreesToRadians(FMath::Clamp(Context.FieldOfView, 5.0f, 170.0f) * 0.5f));
    // X는 Right 정렬 기준 값이므로 좌우 값으로 화면 중앙을 축으로 뒤집는다. 전환 중에는 그 사이를 지난다.
    const double ScreenX = 0.5 + (FMath::Clamp(Settings.TargetScreenPosition.X, 0.05, 0.95) - 0.5) * Side;
    const double X = (ScreenX * 2.0 - 1.0) * TanHalfFOV;
    const double Y = (1.0 - FMath::Clamp(Settings.TargetScreenPosition.Y, 0.05, 0.95) * 2.0) * TanHalfFOV / Aspect;

    // 화면 위치의 카메라 공간 광선을 월드 조준 방향으로 옮기는 회전을 구한다. Roll은 0으로 유지한다.
    const FVector ScreenRay = FVector(1.0, X, Y).GetSafeNormal();
    const FVector WorldRay = ToAim.GetSafeNormal();
    const double PitchRadius = FMath::Sqrt(ScreenRay.X * ScreenRay.X + ScreenRay.Z * ScreenRay.Z);
    const double Pitch = FMath::Asin(FMath::Clamp(WorldRay.Z / PitchRadius, -1.0, 1.0)) - FMath::Atan2(ScreenRay.Z, ScreenRay.X);
    const double ProjectedX = FMath::Cos(Pitch) * ScreenRay.X - FMath::Sin(Pitch) * ScreenRay.Z;
    const double Yaw = FMath::Atan2(WorldRay.Y, WorldRay.X) - FMath::Atan2(ScreenRay.Y, ProjectedX);
    const FRotator Desired(FMath::RadiansToDegrees(Pitch), FMath::RadiansToDegrees(Yaw), 0.0);
    Context.CameraRotation.Pitch += FMath::FindDeltaAngleDegrees(Context.CameraRotation.Pitch, Desired.Pitch) * Weight;
    Context.CameraRotation.Yaw += FMath::FindDeltaAngleDegrees(Context.CameraRotation.Yaw, Desired.Yaw) * Weight;
    Context.CameraRotation.Roll = 0.0;
}
