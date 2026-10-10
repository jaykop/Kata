#include "KataPlayerCameraManager.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Algo/StableSort.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "KataCameraData.h"
#include "KataCameraFeature.h"
#include "KataCameraFeature_LockOn.h"
#include "Components/SceneComponent.h"
#include "Curves/CurveFloat.h"
#include "KataCameraLog.h"
#include "KataCameraPlacement.h"
#include "KataCameraRailComponent.h"
#include "StateTree.h"
#include "StateTreeExecutionContext.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

namespace
{
    const TCHAR* GetRailDiagnostic(EKataCameraRailStatus Status)
    {
        switch (Status)
        {
        case EKataCameraRailStatus::Ready: return TEXT("Ready");
        case EKataCameraRailStatus::InvalidTag: return TEXT("Invalid rail tag");
        case EKataCameraRailStatus::Missing: return TEXT("No matching rail");
        case EKataCameraRailStatus::Duplicate: return TEXT("Multiple matching rails");
        case EKataCameraRailStatus::Closed: return TEXT("Rail must be open");
        case EKataCameraRailStatus::InvalidLength: return TEXT("Rail length is invalid or zero");
        case EKataCameraRailStatus::InvalidPitchRange: return TEXT("Pitch range must be finite and PitchMin < PitchMax");
        case EKataCameraRailStatus::InvalidSample: return TEXT("Rail or profile evaluation is not finite");
        default: return TEXT("");
        }
    }

    float ApplyBlendCurve(EKataCameraBlendCurve Curve, float Alpha)
    {
        // 모든 곡선은 [0, 1]에서 단조 증가한다. 블렌드 중 Pitch가 목표를 넘거나 되돌아가지 않게 하는 전제다.
        switch (Curve)
        {
        case EKataCameraBlendCurve::EaseIn: return FMath::InterpEaseIn(0.0f, 1.0f, Alpha, 2.0f);
        case EKataCameraBlendCurve::EaseOut: return FMath::InterpEaseOut(0.0f, 1.0f, Alpha, 2.0f);
        case EKataCameraBlendCurve::EaseInOut: return FMath::InterpEaseInOut(0.0f, 1.0f, Alpha, 2.0f);
        default: return Alpha;
        }
    }

    FVector BlendOrbitOffset(const FVector& From, const FVector& To, float Weight, EKataCameraOffsetBlend Mode)
    {
        if (Mode == EKataCameraOffsetBlend::DirectionSlerp)
        {
            const double FromLength = From.Size();
            const double ToLength = To.Size();
            if (FromLength > UE_KINDA_SMALL_NUMBER && ToLength > UE_KINDA_SMALL_NUMBER)
            {
                const FQuat Delta = FQuat::FindBetweenNormals(From / FromLength, To / ToLength);
                const FQuat Partial = FQuat::Slerp(FQuat::Identity, Delta, Weight);
                return Partial.RotateVector(From / FromLength) * FMath::Lerp(FromLength, ToLength, static_cast<double>(Weight));
            }
        }
        return FMath::Lerp(From, To, static_cast<double>(Weight));
    }

    /** 위치·회전은 섞지 않는다. 같은 가중치로 궤도 공간 값만 섞고, 위치와 시선은 호출하는 쪽이 다시 계산한다. */
    FKataCameraLayerPose BlendPose(const FKataCameraLayerPose& From, const FKataCameraLayerPose& To, float Weight, EKataCameraOffsetBlend Mode)
    {
        FKataCameraLayerPose Result;
        const double W = Weight;
        Result.Pivot = FMath::Lerp(From.Pivot, To.Pivot, W);
        Result.OrbitOffset = BlendOrbitOffset(From.OrbitOffset, To.OrbitOffset, Weight, Mode);
        Result.AimOffset = FMath::Lerp(From.AimOffset, To.AimOffset, W);
        Result.FieldOfView = FMath::Lerp(From.FieldOfView, To.FieldOfView, Weight);
        Result.PitchMin = FMath::Lerp(From.PitchMin, To.PitchMin, Weight);
        Result.PitchMax = FMath::Lerp(From.PitchMax, To.PitchMax, Weight);
        return Result;
    }
}

namespace
{
    /** 곡선에 데이터가 없으면 기본값을 돌려준다. */
    float EvaluateDistanceCurve(const FRuntimeFloatCurve& Curve, float Distance, float DefaultValue)
    {
        const FRichCurve* RichCurve = Curve.GetRichCurveConst();
        return RichCurve != nullptr && RichCurve->HasAnyData() ? RichCurve->Eval(Distance) : DefaultValue;
    }

    /**
     * 임계 감쇠 스무딩(Game Programming Gems 4, SmoothCD). 대략 SmoothTime 동안 따라잡으며 Rate에 속도를 유지한다.
     * 움직이던 목표가 멈추면 남은 속도 때문에 목표를 지나칠 수 있으므로, 지나치는 프레임에는 목표에 멈추고 속도를 버린다.
     * SmoothTime이 0 이하이면 즉시 목표로 맞춘다.
     */
    void SmoothCriticallyDamped(double& Value, double& Rate, double Target, float DeltaTime, float SmoothTime)
    {
        if (SmoothTime <= 0.0f)
        {
            Value = Target;
            Rate = 0.0;
            return;
        }
        if (DeltaTime <= 0.0f)
        {
            return;
        }
        const double Omega = 2.0 / SmoothTime;
        const double X = Omega * DeltaTime;
        const double Exp = 1.0 / (1.0 + X + 0.48 * X * X + 0.235 * X * X * X);
        const double Change = Value - Target;
        const double Temp = (Rate + Omega * Change) * DeltaTime;
        const double Result = Target + (Change + Temp) * Exp;
        if ((Target - Value > 0.0) == (Result > Target))
        {
            Value = Target;
            Rate = 0.0;
            return;
        }
        Rate = (Rate - Omega * Temp) * Exp;
        Value = Result;
    }

    /** 0~1 진행도에 곡선을 적용한다. 곡선이 없으면 Ease In-Out이고, 시간이 다 되면 곡선 끝값과 관계없이 1이다. */
    float EvaluateBlendAlpha(float Elapsed, float Duration, const UCurveFloat* Curve)
    {
        if (Duration <= 0.0f || Elapsed >= Duration)
        {
            return 1.0f;
        }
        const float Progress = FMath::Clamp(Elapsed / Duration, 0.0f, 1.0f);
        // 선형은 끝 프레임에서 속도가 끊겨 카메라가 붙는 느낌을 주므로 곡선이 없으면 양 끝을 감속한다.
        return Curve != nullptr ? FMath::Clamp(Curve->GetFloatValue(Progress), 0.0f, 1.0f) : ApplyBlendCurve(EKataCameraBlendCurve::EaseInOut, Progress);
    }
}

float FKataCameraBlendLayer::GetWeight() const
{
    if (bFrozen || BlendTime <= 0.0f)
    {
        return 1.0f;
    }
    if (BlendCurveAsset != nullptr)
    {
        return EvaluateBlendAlpha(Elapsed, BlendTime, BlendCurveAsset);
    }
    return ApplyBlendCurve(BlendCurve, FMath::Clamp(Elapsed / BlendTime, 0.0f, 1.0f));
}

AKataPlayerCameraManager::AKataPlayerCameraManager()
{
    LockOnRotation = CreateDefaultSubobject<UKataCameraFeature_LockOnRotation>(TEXT("LockOnRotation"));
    LockOnFraming = CreateDefaultSubobject<UKataCameraFeature_LockOnFraming>(TEXT("LockOnFraming"));
}

bool AKataPlayerCameraManager::HasLockOnFocus() const
{
    const USceneComponent* Focus = LockOnFocus.Get();
    return IsValid(Focus) && Focus->IsRegistered() && IsValid(Focus->GetOwner());
}

void AKataPlayerCameraManager::SetLockOnFocus(USceneComponent* Focus, UKataLockOnData* Data)
{
    if (LockOnFocus.Get() == Focus && !LockOnFocus.IsStale() && LockOnData == Data)
    {
        return;
    }

    const bool bHadFocus = HasLockOnFocus();
    // 진행 중인 블렌드의 현재 값에서 다음 전환을 시작한다. 획득·변경·해제가 겹쳐도 카메라가 튀지 않는다.
    TransitionFromWeight = LockOnWeight;
    TransitionFromFocus = CurrentFocusLocation;
    TransitionFromSettings = CurrentLockOnSettings;
    TransitionFromRotation = PCOwner != nullptr ? PCOwner->GetControlRotation() : LockOnViewRotation;

    LockOnFocus = Focus;
    LockOnData = Focus != nullptr ? Data : nullptr;

    if (HasLockOnFocus())
    {
        const FKataLockOnFramingSettings NewSettings = ResolveLockOnSettings();
        const FVector FocusLocation = Focus->GetComponentLocation();
        // 해제 블렌드가 끝난 뒤의 획득은 가중치만으로 들어오므로 구도 값과 초점을 새 값에서 시작한다.
        const bool bFreshAcquire = !bHadFocus && LockOnWeight <= 0.0f;
        if (bFreshAcquire)
        {
            TransitionFromFocus = FocusLocation;
            TransitionFromSettings = NewSettings;
            CurrentFocusLocation = FocusLocation;
            CurrentLockOnSettings = NewSettings;
        }
        TransitionDuration = NewSettings.BlendInDuration;
        TransitionCurve = NewSettings.BlendInCurve;
        StartLockOnSideTransition(ChooseLockOnSide(NewSettings, TransitionFromRotation, FocusLocation), bFreshAcquire);
    }
    else
    {
        // 해제는 지정 곡선 없이 같은 시간 동안 기본 Ease In-Out으로 돌아간다.
        TransitionDuration = CurrentLockOnSettings.BlendInDuration;
        TransitionCurve = nullptr;
        bSideTransitionActive = false;
    }

    TransitionElapsed = 0.0f;
    bTransitionActive = true;
    bLockOnCameraSelectionDirty = true;
}

FKataLockOnFramingSettings AKataPlayerCameraManager::ResolveLockOnSettings() const
{
    FKataLockOnFramingSettings Settings = LockOnData != nullptr && HasLockOnFocus() ? LockOnData->Settings : DefaultLockOnSettings;
    Settings.SideOffset = FMath::Max(Settings.SideOffset, 0.0f);
    Settings.AutoSwitchAngle = FMath::Clamp(Settings.AutoSwitchAngle, 0.0f, 180.0f);
    Settings.TargetScreenPosition.X = FMath::Clamp(Settings.TargetScreenPosition.X, 0.05, 0.95);
    Settings.TargetScreenPosition.Y = FMath::Clamp(Settings.TargetScreenPosition.Y, 0.05, 0.95);
    Settings.LookAtAlpha = FMath::Clamp(Settings.LookAtAlpha, 0.0f, 1.0f);
    Settings.MaxLookDistance = FMath::Max(Settings.MaxLookDistance, 0.0f);
    Settings.RotationLagTime = FMath::Max(Settings.RotationLagTime, 0.0f);
    Settings.BlendInDuration = FMath::Max(Settings.BlendInDuration, 0.0f);
    return Settings;
}

float AKataPlayerCameraManager::ChooseLockOnSide(const FKataLockOnFramingSettings& Settings, const FRotator& View, const FVector& FocusLocation) const
{
    switch (Settings.Alignment)
    {
    case EKataLockOnAlignment::Right: return 1.0f;
    case EKataLockOnAlignment::Left: return -1.0f;
    default: break;
    }

    const AActor* Target = GetViewTarget();
    const FVector Direction = Target != nullptr ? FocusLocation - Target->GetActorLocation() : FVector::ZeroVector;
    if (Direction.IsNearlyZero())
    {
        return SideTarget;
    }
    // Yaw가 커지는 쪽이 오른쪽이다. 정면에 있으면 오른쪽을 기본으로 한다.
    return FMath::FindDeltaAngleDegrees(View.Yaw, Direction.Rotation().Yaw) >= 0.0f ? 1.0f : -1.0f;
}

void AKataPlayerCameraManager::StartLockOnSideTransition(float NewSide, bool bInstant)
{
    if (bInstant)
    {
        LockOnSide = SideFrom = SideTarget = NewSide;
        bSideTransitionActive = false;
        return;
    }
    if (SideTarget == NewSide)
    {
        return;
    }
    SideFrom = LockOnSide;
    SideTarget = NewSide;
    SideElapsed = 0.0f;
    bSideTransitionActive = true;
}

void AKataPlayerCameraManager::UpdateLockOn(float DeltaTime, APawn* ViewPawn)
{
    const bool bActive = HasLockOnFocus();
    const bool bWasTransitioning = bTransitionActive;
    float Alpha = 1.0f;
    if (bTransitionActive)
    {
        TransitionElapsed += FMath::Max(DeltaTime, 0.0f);
        Alpha = EvaluateBlendAlpha(TransitionElapsed, TransitionDuration, TransitionCurve);
        bTransitionActive = TransitionElapsed < TransitionDuration;
    }

    // 해제 중에는 해제 순간의 설정을 유지한다. 활성 중 설정은 매니저 기본값과 지점 데이터만으로 정해져 락온 동안 바뀌지 않는다.
    const FKataLockOnFramingSettings TargetSettings = bActive ? ResolveLockOnSettings() : TransitionFromSettings;
    CurrentLockOnSettings = TargetSettings;
    CurrentLockOnSettings.TargetScreenPosition = FMath::Lerp(TransitionFromSettings.TargetScreenPosition, TargetSettings.TargetScreenPosition, static_cast<double>(Alpha));
    CurrentLockOnSettings.SideOffset = FMath::Lerp(TransitionFromSettings.SideOffset, TargetSettings.SideOffset, Alpha);
    CurrentLockOnSettings.LookAtAlpha = FMath::Lerp(TransitionFromSettings.LookAtAlpha, TargetSettings.LookAtAlpha, Alpha);
    // 0은 무제한이라 0과 양수 사이를 보간하면 의미 없는 상한이 생긴다. 양쪽 모두 제한이 있을 때만 보간한다.
    if (TransitionFromSettings.MaxLookDistance > 0.0f && TargetSettings.MaxLookDistance > 0.0f)
    {
        CurrentLockOnSettings.MaxLookDistance = FMath::Lerp(TransitionFromSettings.MaxLookDistance, TargetSettings.MaxLookDistance, Alpha);
    }
    LockOnWeight = FMath::Lerp(TransitionFromWeight, bActive ? 1.0f : 0.0f, Alpha);

    if (!bActive || ViewPawn == nullptr)
    {
        // 해제 중 초점과 좌우 값은 마지막 위치에 머문다. 회전은 플레이어 입력이 컨트롤 회전에서 이어 간다.
        return;
    }

    const FVector LiveFocus = LockOnFocus->GetComponentLocation();
    CurrentFocusLocation = FMath::Lerp(TransitionFromFocus, LiveFocus, static_cast<double>(Alpha));

    // 래그된 폰 위치를 기준으로 쓴다. 루트 모션의 짧은 좌우 흔들림마다 시선이 돌면 카메라가 흔들림을 키운다.
    // 높이는 카메라가 도는 피벗에 맞춘다. 폰 원점(캡슐 중심)에서 재면 피벗보다 낮은 지점도 올려다보는 Pitch가 나와 카메라가 내려간다.
    // 피벗의 수평 오프셋은 Yaw를 따라 돌므로 넣지 않는다. 넣으면 방향과 피벗이 서로를 바꾸는 되먹임이 생긴다.
    // 배치는 이 회전을 입력으로 평가하므로 피벗 높이는 직전 프레임 값을 쓴다.
    const FVector Direction = LiveFocus - (ViewPawn->GetActorLocation() + PivotLagOffset + FVector(0.0, 0.0, PivotHeightFromPawn));

    // 거리 곡선은 이전·새 설정에서 각각 구해 섞는다. 타겟 변경 때 곡선이 바뀌어도 값이 튀지 않는다.
    const float FocusDistance = Direction.Size();
    LockOnPitchOffset = FMath::Lerp(EvaluateDistanceCurve(TransitionFromSettings.PitchOffsetByDistance, FocusDistance, 0.0f),
        EvaluateDistanceCurve(TargetSettings.PitchOffsetByDistance, FocusDistance, 0.0f), Alpha);
    LockOnDistanceScale = FMath::Max(FMath::Lerp(EvaluateDistanceCurve(TransitionFromSettings.BoomDistanceScaleByDistance, FocusDistance, 1.0f),
        EvaluateDistanceCurve(TargetSettings.BoomDistanceScaleByDistance, FocusDistance, 1.0f), Alpha), 0.1f);
    // Framing은 설정의 LookAtAlpha를 읽으므로 거리 곡선을 반영한 실제 비율로 바꿔 둔다.
    CurrentLockOnSettings.LookAtAlpha = FMath::Clamp(FMath::Lerp(
        EvaluateDistanceCurve(TransitionFromSettings.LookAtAlphaByDistance, FocusDistance, TransitionFromSettings.LookAtAlpha),
        EvaluateDistanceCurve(TargetSettings.LookAtAlphaByDistance, FocusDistance, TargetSettings.LookAtAlpha), Alpha), 0.0f, 1.0f);

    FRotator Desired = Direction.IsNearlyZero() ? LockOnViewRotation : Direction.Rotation();
    // 양수 오프셋은 내려다보게 하므로 Pitch를 낮춘다. 락온 Pitch는 카메라 데이터의 입력 Pitch 제한만 따른다. ViewPitchMin/Max는 이전 프레임에 섞인 제한이다.
    Desired.Pitch = FMath::Clamp(static_cast<float>(Desired.Pitch) - LockOnPitchOffset, ViewPitchMin, ViewPitchMax);
    Desired.Roll = 0.0;

    if (!bWasTransitioning)
    {
        // 블렌드 중 시선은 의도적으로 뒤처져 있으므로 블렌드가 끝난 뒤에만 좌우 전환을 판정한다.
        switch (CurrentLockOnSettings.Alignment)
        {
        case EKataLockOnAlignment::Right:
            StartLockOnSideTransition(1.0f, false);
            break;
        case EKataLockOnAlignment::Left:
            StartLockOnSideTransition(-1.0f, false);
            break;
        default:
        {
            const float YawDelta = FMath::FindDeltaAngleDegrees(LockOnViewRotation.Yaw, Desired.Yaw);
            if (SideTarget > 0.0f && YawDelta < -CurrentLockOnSettings.AutoSwitchAngle)
            {
                StartLockOnSideTransition(-1.0f, false);
            }
            else if (SideTarget < 0.0f && YawDelta > CurrentLockOnSettings.AutoSwitchAngle)
            {
                StartLockOnSideTransition(1.0f, false);
            }
            break;
        }
        }
    }

    if (bSideTransitionActive)
    {
        SideElapsed += FMath::Max(DeltaTime, 0.0f);
        const float SideAlpha = EvaluateBlendAlpha(SideElapsed, CurrentLockOnSettings.BlendInDuration, CurrentLockOnSettings.BlendInCurve);
        LockOnSide = FMath::Lerp(SideFrom, SideTarget, SideAlpha);
        bSideTransitionActive = SideElapsed < CurrentLockOnSettings.BlendInDuration;
    }

    FRotator View = LockOnViewRotation;
    if (bWasTransitioning)
    {
        // 블렌드 중에는 전환 시작 회전에서 실제 지점 방향까지 곡선 진행도만큼 돈다. 각속도 제한을 함께 쓰면 곡선 모양이 깨진다.
        View = TransitionFromRotation;
        View.Yaw += FMath::FindDeltaAngleDegrees(TransitionFromRotation.Yaw, Desired.Yaw) * Alpha;
        View.Pitch += FMath::FindDeltaAngleDegrees(TransitionFromRotation.Pitch, Desired.Pitch) * Alpha;
        // 블렌드 곡선은 끝에서 속도가 0이 되므로 감쇠 추적을 정지 상태에서 시작한다.
        LockOnYawRate = 0.0;
        LockOnPitchRate = 0.0;
    }
    else
    {
        // 목표를 현재 값 근처로 펴서 180도 경계에서 반대로 도는 일을 막는다.
        double Yaw = View.Yaw;
        double Pitch = View.Pitch;
        SmoothCriticallyDamped(Yaw, LockOnYawRate, Yaw + FMath::FindDeltaAngleDegrees(Yaw, Desired.Yaw), DeltaTime, CurrentLockOnSettings.RotationLagTime);
        SmoothCriticallyDamped(Pitch, LockOnPitchRate, Pitch + FMath::FindDeltaAngleDegrees(Pitch, Desired.Pitch), DeltaTime, CurrentLockOnSettings.RotationLagTime);
        View.Yaw = FRotator::NormalizeAxis(Yaw);
        View.Pitch = Pitch;
    }
    View.Roll = 0.0;
    LockOnViewRotation = View;
}

void AKataPlayerCameraManager::InitializeFor(APlayerController* PC)
{
    Super::InitializeFor(PC);

    OrderedFeatures.Reset(Features.Num());
    OrderedFeatures.Add(LockOnRotation);
    OrderedFeatures.Add(LockOnFraming);
    for (UKataCameraFeature* Feature : Features)
    {
        if (Feature != nullptr)
        {
            OrderedFeatures.Add(Feature);
        }
    }

    // 같은 단계와 우선순위끼리는 Details에 적은 순서를 유지해야 결과가 예측 가능하다.
    Algo::StableSort(OrderedFeatures, [](const UKataCameraFeature* A, const UKataCameraFeature* B)
    {
        if (A->GetStage() != B->GetStage())
        {
            return A->GetStage() < B->GetStage();
        }
        return A->GetPriority() < B->GetPriority();
    });

    for (UKataCameraFeature* Feature : OrderedFeatures)
    {
        Feature->Initialize(this);
    }
}

void AKataPlayerCameraManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    StopStateTree();

    for (UKataCameraFeature* Feature : OrderedFeatures)
    {
        if (Feature != nullptr)
        {
            Feature->Deinitialize();
        }
    }
    OrderedFeatures.Reset();
    BlendLayers.Reset();

    LockOnFocus.Reset();
    bPivotLagValid = false;
    PivotLagOffset = FVector::ZeroVector;
    PivotHeightFromPawn = 0.0f;
    LockOnData = nullptr;
    LockOnWeight = 0.0f;
    bTransitionActive = false;
    bSideTransitionActive = false;
    bLockOnCameraSelectionDirty = false;

    Super::EndPlay(EndPlayReason);
}

#if WITH_EDITOR
EDataValidationResult AKataPlayerCameraManager::IsDataValid(FDataValidationContext& Context) const
{
    const EDataValidationResult Result = Super::IsDataValid(Context);
    UKataLockOnData::ValidateBlendCurve(DefaultLockOnSettings.BlendInCurve, Context);
    return Result;
}
#endif

UKataCameraData* AKataPlayerCameraManager::GetActiveCameraData() const
{
    return BlendLayers.IsEmpty() ? DefaultCameraData.Get() : BlendLayers.Last().CameraData.Get();
}

void AKataPlayerCameraManager::UpdateViewTargetInternal(FTViewTarget& OutVT, float DeltaTime)
{
    APawn* ViewPawn = Cast<APawn>(OutVT.Target);
    if (PCOwner == nullptr || ViewPawn == nullptr)
    {
        ApplyPitchLimits(nullptr);
        bPivotLagValid = false;
        PivotHeightFromPawn = 0.0f;
        DebugSnapshot = FKataCameraDebugSnapshot();
        DebugSnapshot.CameraDataName = GetNameSafe(GetActiveCameraData());
        Super::UpdateViewTargetInternal(OutVT, DeltaTime);
        return;
    }

    UpdateStateTree(ViewPawn, DeltaTime);
    const bool bNewStateRequest = bStateTreeRunning && StateTreeRequest.Serial != AppliedRequestSerial;
    if (bNewStateRequest)
    {
        AppliedRequestSerial = StateTreeRequest.Serial;
    }
    // 지점이 파괴되거나 등록 해제되면 해제로 처리한다. 레이어 선택 전에 해야 락온 CameraData 해제 블렌드가 같은 프레임에 시작된다.
    if (!HasLockOnFocus() && (LockOnFocus.IsStale() || LockOnFocus.Get() != nullptr || LockOnData != nullptr))
    {
        SetLockOnFocus(nullptr);
    }
    UKataCameraData* DesiredCameraData = bStateTreeRunning && AppliedRequestSerial != 0 ? StateTreeRequest.CameraData.Get() : DefaultCameraData.Get();
    const bool bLockOverride = HasLockOnFocus() && LockOnData != nullptr && LockOnData->CameraData != nullptr;
    if (bLockOverride)
    {
        DesiredCameraData = LockOnData->CameraData;
    }
    if (BlendLayers.IsEmpty() || GetActiveCameraData() != DesiredCameraData || bNewStateRequest || bLockOnCameraSelectionDirty)
    {
        if (bLockOnCameraSelectionDirty)
        {
            // 락온 때문에 바뀐 배치는 락온 전환과 같은 시간·곡선으로 섞어 회전·구도·배치가 함께 움직이게 한다.
            PushBlendLayer(DesiredCameraData, TransitionDuration, EKataCameraBlendCurve::EaseInOut, EKataCameraOffsetBlend::Linear, TransitionCurve);
        }
        else
        {
            const float BlendTime = bStateTreeRunning && !bLockOverride ? StateTreeRequest.BlendTime : 0.0f;
            PushBlendLayer(DesiredCameraData, BlendTime, StateTreeRequest.BlendCurve, StateTreeRequest.OffsetBlend);
        }
        bLockOnCameraSelectionDirty = false;
    }

    UpdatePivotLag(ViewPawn, DeltaTime);
    UpdateLockOn(DeltaTime, ViewPawn);

    // 맨 위부터 내려가며 가중치 1에 도달한 레이어를 찾고, 그 아래는 결과에 영향이 없으므로 제거한다.
    for (FKataCameraBlendLayer& Layer : BlendLayers)
    {
        Layer.Elapsed += DeltaTime;
    }
    for (int32 Index = BlendLayers.Num() - 1; Index > 0; --Index)
    {
        if (BlendLayers[Index].GetWeight() >= 1.0f)
        {
            BlendLayers.RemoveAt(0, Index);
            break;
        }
    }

    FKataCameraPipelineContext BaseContext;
    BaseContext.PlayerController = PCOwner;
    BaseContext.ViewTarget = ViewPawn;
    BaseContext.DeltaTime = DeltaTime;
    BaseContext.ViewRotation = PCOwner->GetControlRotation();
    BaseContext.CameraData = GetActiveCameraData();
    BaseContext.LockFocus = LockOnFocus.Get();
    BaseContext.LockFocusLocation = CurrentFocusLocation;
    BaseContext.bLockOnActive = HasLockOnFocus();
    BaseContext.LockOnWeight = LockOnWeight;
    BaseContext.LockOnSide = LockOnSide;
    BaseContext.LockOnDistanceScale = LockOnDistanceScale;
    // 회전 드라이버는 모든 레이어가 공유하는 입력 회전을 정한다. 레이어마다 다른 회전을 쓰지 않는다.
    RunFeatures(EKataCameraStage::Rotation, BaseContext);

    TArray<FKataCameraLayerPose, TInlineAllocator<MaxBlendLayers + 1>> Poses;
    TArray<bool, TInlineAllocator<MaxBlendLayers + 1>> PoseValid;
    FKataCameraPipelineContext TopContext;
    int32 TopValidIndex = INDEX_NONE;
    const FVector PawnLocation = ViewPawn->GetActorLocation();
    for (int32 Index = 0; Index < BlendLayers.Num(); ++Index)
    {
        FKataCameraBlendLayer& Layer = BlendLayers[Index];
        FKataCameraLayerPose& Pose = Poses.AddDefaulted_GetRef();
        bool& bValid = PoseValid.Add_GetRef(false);
        if (Layer.bFrozen)
        {
            Pose = Layer.FrozenPose;
            Pose.Pivot = PawnLocation + Layer.FrozenPivotFromPawn;
            bValid = true;
            continue;
        }

        FKataCameraPipelineContext LayerContext;
        bValid = EvaluateLayer(Layer, ViewPawn, BaseContext, LayerContext, Pose);
        if (bValid)
        {
            TopContext = LayerContext;
            TopValidIndex = Index;
        }
    }

    // 상한을 넘으면 바닥 두 레이어의 섞인 결과를 그 순간 값으로 고정해 하나로 합친다. 합친 순간의 출력은 바뀌지 않는다.
    while (BlendLayers.Num() > MaxBlendLayers)
    {
        FKataCameraBlendLayer& Upper = BlendLayers[1];
        FKataCameraLayerPose Merged = Poses[1];
        if (PoseValid[0] && PoseValid[1])
        {
            Merged = BlendPose(Poses[0], Poses[1], Upper.GetWeight(), Upper.OffsetBlend);
        }
        else if (PoseValid[0])
        {
            Merged = Poses[0];
        }

        Upper.bFrozen = PoseValid[0] || PoseValid[1];
        Upper.FrozenPose = Merged;
        Upper.FrozenPivotFromPawn = Merged.Pivot - PawnLocation;
        Poses[1] = Merged;
        PoseValid[1] = Upper.bFrozen;
        BlendLayers.RemoveAt(0);
        Poses.RemoveAt(0);
        PoseValid.RemoveAt(0);
        // 합쳐진 두 레이어 중 하나가 맨 위 평가 결과였다면 그 컨텍스트는 더 이상 이 프레임 출력과 맞지 않는다.
        TopValidIndex = TopValidIndex <= 1 ? INDEX_NONE : TopValidIndex - 1;
    }

    int32 ValidCount = 0;
    FKataCameraLayerPose Result;
    for (int32 Index = 0; Index < Poses.Num(); ++Index)
    {
        if (!PoseValid[Index])
        {
            continue;
        }
        Result = ValidCount == 0 ? Poses[Index] : BlendPose(Result, Poses[Index], BlendLayers[Index].GetWeight(), BlendLayers[Index].OffsetBlend);
        ++ValidCount;
    }

    if (ValidCount == 0)
    {
        ApplyPitchLimits(nullptr);
        bPivotLagValid = false;
        PivotHeightFromPawn = 0.0f;
        DebugSnapshot = FKataCameraDebugSnapshot();
        DebugSnapshot.CameraDataName = GetNameSafe(GetActiveCameraData());
        Super::UpdateViewTargetInternal(OutVT, DeltaTime);
        return;
    }

    ApplyPitchLimits(&Result);
    // 래그 적용 전 값이다. 락온 방향은 래그 오프셋을 따로 더한다.
    PivotHeightFromPawn = Result.Pivot.Z - PawnLocation.Z;

    FKataCameraPipelineContext Context = TopValidIndex != INDEX_NONE ? TopContext : BaseContext;
    Context.ViewRotation = BaseContext.ViewRotation;
    Context.PivotLocation = Result.Pivot;
    Context.OrbitOffset = Result.OrbitOffset;
    Context.AimOffset = Result.AimOffset;
    Context.FieldOfView = Result.FieldOfView;
    if (ValidCount > 1 || TopValidIndex == INDEX_NONE)
    {
        // 섞인 궤도 결과로 위치와 시선을 다시 계산한다. 시선 벡터가 가중치에 대해 선형이라 블렌드 중 Pitch가 단조 변화한다.
        const FRotator YawRotation(0.0, Context.ViewRotation.Yaw, 0.0);
        Context.CameraLocation = Result.Pivot + YawRotation.RotateVector(Result.OrbitOffset);
        const FVector AimDirection = Result.Pivot + YawRotation.RotateVector(Result.AimOffset) - Context.CameraLocation;
        Context.CameraRotation = AimDirection.IsNearlyZero() ? Context.ViewRotation : AimDirection.Rotation();
    }

    ApplyPivotLag(Context);

    RunFeatures(EKataCameraStage::Framing, Context);
    // Shrink가 피벗을 바꾸지는 않지만 조준선은 Framing이 실제로 쓴 값으로 남긴다.
    const FVector DebugLockLineStart = Context.PivotLocation;
    const FVector DebugLockAimPoint = Context.LockOnAimPoint;
    RunFeatures(EKataCameraStage::Constraint, Context);
    RunFeatures(EKataCameraStage::Reaction, Context);

    OutVT.POV.Location = Context.CameraLocation;
    OutVT.POV.Rotation = Context.CameraRotation;
    OutVT.POV.FOV = Context.FieldOfView;

    DebugSnapshot = FKataCameraDebugSnapshot();
    DebugSnapshot.bPipelineActive = true;
    DebugSnapshot.CameraDataName = GetNameSafe(GetActiveCameraData());
    DebugSnapshot.ViewTargetLocation = ViewPawn->GetActorLocation();
    DebugSnapshot.PivotLocation = Context.PivotLocation;
    DebugSnapshot.ViewRotation = Context.ViewRotation;
    DebugSnapshot.CameraLocation = Context.CameraLocation;
    DebugSnapshot.CameraRotation = Context.CameraRotation;
    DebugSnapshot.FieldOfView = Context.FieldOfView;
    DebugSnapshot.OrbitOffset = Context.OrbitOffset;
    DebugSnapshot.AimOffset = Context.AimOffset;
    DebugSnapshot.StateTreeName = GetNameSafe(CameraStateTree.GetStateTree());
    DebugSnapshot.bStateTreeRunning = bStateTreeRunning;
    DebugSnapshot.bLockOnActive = HasLockOnFocus();
    DebugSnapshot.LockOnWeight = LockOnWeight;
    DebugSnapshot.LockOnSide = LockOnSide;
    DebugSnapshot.LockOnDataName = LockOnData != nullptr ? LockOnData->GetName() : FString();
    DebugSnapshot.PivotLagOffset = PivotLagOffset;
    DebugSnapshot.LockOnLineStart = DebugLockLineStart;
    DebugSnapshot.LockOnLineEnd = CurrentFocusLocation;
    DebugSnapshot.LockOnAimPoint = DebugLockAimPoint;
    DebugSnapshot.LockOnLookAtAlpha = CurrentLockOnSettings.LookAtAlpha;
    DebugSnapshot.LockOnPitchOffset = LockOnPitchOffset;
    DebugSnapshot.LockOnDistanceScale = LockOnDistanceScale;
    for (const FKataCameraBlendLayer& Layer : BlendLayers)
    {
        FKataCameraDebugLayer& DebugLayer = DebugSnapshot.Layers.AddDefaulted_GetRef();
        DebugLayer.CameraDataName = GetNameSafe(Layer.CameraData);
        DebugLayer.Weight = Layer.GetWeight();
        DebugLayer.RemainingTime = Layer.bFrozen ? 0.0f : FMath::Max(Layer.BlendTime - Layer.Elapsed, 0.0f);
        DebugLayer.bFrozen = Layer.bFrozen;
    }

    const UKataCameraData* TopData = TopValidIndex != INDEX_NONE ? BlendLayers[TopValidIndex].CameraData.Get() : nullptr;
    const UKataCameraPlacement_Spline* SplinePlacement = TopData != nullptr ? Cast<UKataCameraPlacement_Spline>(TopData->Placement) : nullptr;
    DebugSnapshot.PlacementName = TopData != nullptr && TopData->Placement != nullptr
        ? TopData->Placement->GetClass()->GetDisplayNameText().ToString() : FString();
    if (SplinePlacement != nullptr)
    {
        DebugSnapshot.RailTag = SplinePlacement->RailTag.ToString();
        DebugSnapshot.RailDiagnostic = GetRailDiagnostic(TopContext.RailStatus);
        DebugSnapshot.bUsingSpline = TopContext.RailStatus == EKataCameraRailStatus::Ready && TopContext.Rail != nullptr;
        DebugSnapshot.RailAlpha = TopContext.RailAlpha;
        DebugSnapshot.RailDistance = TopContext.RailDistance;
        if (DebugSnapshot.bUsingSpline)
        {
            DebugSnapshot.RailLength = TopContext.Rail->GetSplineLength();
#if WITH_GAMEPLAY_DEBUGGER
            constexpr int32 SampleCount = 33;
            DebugSnapshot.RailSamples.Reserve(SampleCount);
            for (int32 Index = 0; Index < SampleCount; ++Index)
            {
                const float Distance = DebugSnapshot.RailLength * static_cast<float>(Index) / (SampleCount - 1);
                const FVector Sample = TopContext.Rail->GetOrbitOffsetAtDistance(Distance);
                if (Sample.ContainsNaN())
                {
                    DebugSnapshot.RailSamples.Reset();
                    break;
                }
                DebugSnapshot.RailSamples.Add(Sample);
            }
#endif
        }
    }
}

bool AKataPlayerCameraManager::EvaluateLayer(FKataCameraBlendLayer& Layer, APawn* ViewPawn, const FKataCameraPipelineContext& BaseContext,
    FKataCameraPipelineContext& OutContext, FKataCameraLayerPose& OutPose)
{
    const UKataCameraData* CameraData = Layer.CameraData;
    if (CameraData == nullptr)
    {
        return false;
    }
    if (CameraData->Placement == nullptr)
    {
        if (MissingPlacementWarned.Get() != CameraData)
        {
            UE_LOG(LogKataCamera, Warning, TEXT("Camera data %s has no Placement. The layer is skipped."), *GetNameSafe(CameraData));
            MissingPlacementWarned = CameraData;
        }
        return false;
    }

    OutContext = BaseContext;
    OutContext.CameraData = CameraData;
    OutContext.FieldOfView = CameraData->FieldOfView;

    // 초기 피벗은 뷰 타깃 위치다. Boom Arm은 자기 피벗 오프셋을 더하고, Spline은 레일 원점으로 바꾼다.
    OutContext.PivotLocation = ViewPawn->GetActorLocation();

    const UKataCameraPlacement_Spline* SplinePlacement = Cast<UKataCameraPlacement_Spline>(CameraData->Placement);
    ResolveRail(ViewPawn, SplinePlacement, OutContext);
    CameraData->Placement->Evaluate(OutContext);
    UpdateRailDiagnostic(ViewPawn, SplinePlacement, OutContext, Layer.RailDiagnostic);

    OutPose.Pivot = OutContext.PivotLocation;
    OutPose.OrbitOffset = OutContext.OrbitOffset;
    OutPose.AimOffset = OutContext.AimOffset;
    OutPose.FieldOfView = OutContext.FieldOfView;
    OutPose.PitchMin = CameraData->PitchMin;
    OutPose.PitchMax = CameraData->PitchMax;
    return !OutPose.Pivot.ContainsNaN() && !OutPose.OrbitOffset.ContainsNaN() && !OutPose.AimOffset.ContainsNaN();
}

void AKataPlayerCameraManager::UpdatePivotLag(APawn* ViewPawn, float DeltaTime)
{
    const UKataCameraData* CameraData = GetActiveCameraData();
    const FVector PawnLocation = ViewPawn->GetActorLocation();
    if (!bPivotLagValid || PivotLagPawn.Get() != ViewPawn || CameraData == nullptr)
    {
        // 처음이거나 폰이 바뀌었으면 이전 위치에서 날아오지 않게 바로 붙인다.
        LaggedAnchor = PawnLocation;
        PivotLagRate = FVector::ZeroVector;
        PivotLagPawn = ViewPawn;
        bPivotLagValid = true;
    }
    else
    {
        SmoothCriticallyDamped(LaggedAnchor.X, PivotLagRate.X, PawnLocation.X, DeltaTime, CameraData->PivotLagTimeHorizontal);
        SmoothCriticallyDamped(LaggedAnchor.Y, PivotLagRate.Y, PawnLocation.Y, DeltaTime, CameraData->PivotLagTimeHorizontal);
        SmoothCriticallyDamped(LaggedAnchor.Z, PivotLagRate.Z, PawnLocation.Z, DeltaTime, CameraData->PivotLagTimeVertical);
        const float MaxDistance = CameraData->PivotLagMaxDistance;
        if (MaxDistance > 0.0f && FVector::DistSquared(LaggedAnchor, PawnLocation) > FMath::Square(MaxDistance))
        {
            LaggedAnchor = PawnLocation + (LaggedAnchor - PawnLocation).GetClampedToMaxSize(MaxDistance);
        }
    }
    PivotLagOffset = LaggedAnchor - PawnLocation;
}

void AKataPlayerCameraManager::ApplyPivotLag(FKataCameraPipelineContext& Context) const
{
    // 피벗과 카메라를 같은 양만큼 옮기므로 배치가 정한 시선 방향은 바뀌지 않는다. Shrink는 폰 위치에서 래그된 피벗까지 먼저 스윕한다.
    Context.PivotLocation += PivotLagOffset;
    Context.CameraLocation += PivotLagOffset;
    Context.PivotLagOffset = PivotLagOffset;
}

void AKataPlayerCameraManager::PushBlendLayer(UKataCameraData* CameraData, float BlendTime, EKataCameraBlendCurve BlendCurve, EKataCameraOffsetBlend OffsetBlend,
    UCurveFloat* CurveAsset)
{
    if (CameraData == nullptr)
    {
        return;
    }
    if (!BlendLayers.IsEmpty() && !BlendLayers.Last().bFrozen && BlendLayers.Last().CameraData == CameraData)
    {
        return;
    }

    FKataCameraBlendLayer& Layer = BlendLayers.AddDefaulted_GetRef();
    Layer.CameraData = CameraData;
    // 첫 레이어는 섞을 대상이 없으므로 즉시 적용해 시작할 때 카메라가 날아오지 않게 한다.
    Layer.BlendTime = BlendLayers.Num() == 1 ? 0.0f : FMath::Max(BlendTime, 0.0f);
    Layer.BlendCurve = BlendCurve;
    Layer.BlendCurveAsset = CurveAsset;
    Layer.OffsetBlend = OffsetBlend;
}

void AKataPlayerCameraManager::UpdateStateTree(APawn* ViewPawn, float DeltaTime)
{
    const UStateTree* StateTree = CameraStateTree.GetStateTree();
    UAbilitySystemComponent* AbilitySystem = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(ViewPawn);
    if (bStateTreeRunning && (StateTree == nullptr || StateTreePawn.Get() != ViewPawn || StateTreeAbilitySystem.Get() != AbilitySystem))
    {
        StopStateTree();
    }
    if (StateTree == nullptr || AbilitySystem == nullptr || StateTreeFailedPawn.Get() == ViewPawn)
    {
        return;
    }

    FStateTreeExecutionContext Context(*this, *StateTree, StateTreeInstanceData);
    if (!Context.IsValid() || !SetupStateTreeContext(Context, ViewPawn, AbilitySystem))
    {
        UE_LOG(LogKataCamera, Warning, TEXT("Camera StateTree %s cannot run for pawn %s. Using DefaultCameraData."),
            *GetNameSafe(StateTree), *GetNameSafe(ViewPawn));
        StateTreeFailedPawn = ViewPawn;
        return;
    }

    if (!bStateTreeRunning)
    {
        const EStateTreeRunStatus Status = Context.Start(CameraStateTree.GetGlobalParameters());
        if (Status != EStateTreeRunStatus::Running)
        {
            UE_LOG(LogKataCamera, Warning, TEXT("Camera StateTree %s did not start (status %d). Using DefaultCameraData."),
                *GetNameSafe(StateTree), static_cast<int32>(Status));
            StateTreeFailedPawn = ViewPawn;
            return;
        }
        bStateTreeRunning = true;
        StateTreePawn = ViewPawn;
        StateTreeAbilitySystem = AbilitySystem;
        return;
    }

    // 상태는 Status 태그가 바뀔 때만 다시 고른다. 이벤트가 없으면 트리를 갱신하지 않는다.
    if (StateTreeInstanceData.GetEventQueue().HasEvents())
    {
        Context.Tick(DeltaTime);
    }
}

void AKataPlayerCameraManager::StopStateTree()
{
    const UStateTree* StateTree = CameraStateTree.GetStateTree();
    APawn* Pawn = StateTreePawn.Get();
    UAbilitySystemComponent* AbilitySystem = StateTreeAbilitySystem.Get();
    if (bStateTreeRunning && StateTree != nullptr && Pawn != nullptr && AbilitySystem != nullptr)
    {
        // 정상 정지여야 Evaluator가 ASC 태그 구독을 해제한다. 폰이나 ASC가 이미 사라졌으면 구독도 함께 사라졌다.
        FStateTreeExecutionContext Context(*this, *StateTree, StateTreeInstanceData);
        if (Context.IsValid() && SetupStateTreeContext(Context, Pawn, AbilitySystem))
        {
            Context.Stop();
        }
    }

    StateTreeInstanceData.Reset();
    StateTreeRequest = FKataCameraStateTreeRequest();
    AppliedRequestSerial = 0;
    bStateTreeRunning = false;
    StateTreePawn.Reset();
    StateTreeAbilitySystem.Reset();
    StateTreeFailedPawn.Reset();
}

bool AKataPlayerCameraManager::SetupStateTreeContext(FStateTreeExecutionContext& Context, APawn* Pawn, UAbilitySystemComponent* AbilitySystem)
{
    Context.SetContextDataByName(KataCameraStateTree::CameraManagerName, FStateTreeDataView(this));
    Context.SetContextDataByName(KataCameraStateTree::PawnName, FStateTreeDataView(Pawn));
    Context.SetContextDataByName(KataCameraStateTree::AbilitySystemName, FStateTreeDataView(AbilitySystem));
    Context.SetCollectExternalDataCallback(FOnCollectStateTreeExternalData::CreateLambda(
        [this](const FStateTreeExecutionContext&, const UStateTree*, TArrayView<const FStateTreeExternalDataDesc> ExternalDescs,
            TArrayView<FStateTreeDataView> OutDataViews)
        {
            for (int32 Index = 0; Index < ExternalDescs.Num(); ++Index)
            {
                if (ExternalDescs[Index].Struct == FKataCameraStateTreeRequest::StaticStruct())
                {
                    OutDataViews[Index] = FStateTreeDataView(FStructView::Make(StateTreeRequest));
                }
            }
            return true;
        }));
    return Context.AreContextDataViewsValid();
}

void AKataPlayerCameraManager::ResolveRail(APawn* ViewPawn, const UKataCameraPlacement_Spline* Placement, FKataCameraPipelineContext& Context) const
{
    if (Placement == nullptr)
    {
        return;
    }
    if (!Placement->RailTag.IsValid())
    {
        Context.RailStatus = EKataCameraRailStatus::InvalidTag;
        return;
    }
    if (!FMath::IsFinite(Context.CameraData->PitchMin) || !FMath::IsFinite(Context.CameraData->PitchMax) ||
        Context.CameraData->PitchMax - Context.CameraData->PitchMin <= UE_SMALL_NUMBER)
    {
        Context.RailStatus = EKataCameraRailStatus::InvalidPitchRange;
        return;
    }

    TInlineComponentArray<UKataCameraRailComponent*> Rails;
    ViewPawn->GetComponents(Rails);
    int32 MatchCount = 0;
    UKataCameraRailComponent* MatchedRail = nullptr;
    for (UKataCameraRailComponent* Rail : Rails)
    {
        if (IsValid(Rail) && Rail->RailTag.IsValid() && Rail->RailTag.MatchesTagExact(Placement->RailTag))
        {
            ++MatchCount;
            MatchedRail = Rail;
        }
    }
    if (MatchCount != 1)
    {
        Context.RailStatus = MatchCount == 0 ? EKataCameraRailStatus::Missing : EKataCameraRailStatus::Duplicate;
        return;
    }
    if (MatchedRail->IsClosedLoop())
    {
        Context.RailStatus = EKataCameraRailStatus::Closed;
        return;
    }
    const float Length = MatchedRail->GetSplineLength();
    if (!FMath::IsFinite(Length) || Length <= UE_SMALL_NUMBER)
    {
        Context.RailStatus = EKataCameraRailStatus::InvalidLength;
        return;
    }

    Context.Rail = MatchedRail;
    Context.RailStatus = EKataCameraRailStatus::Ready;
}

void AKataPlayerCameraManager::UpdateRailDiagnostic(APawn* ViewPawn, const UKataCameraPlacement_Spline* Placement,
    const FKataCameraPipelineContext& Context, FKataCameraRailDiagnosticState& State) const
{
    const FName Tag = Placement != nullptr ? Placement->RailTag.GetTagName() : NAME_None;
    if (Context.RailStatus != EKataCameraRailStatus::NotRequested && Context.RailStatus != EKataCameraRailStatus::Ready &&
        (State.Pawn.Get() != ViewPawn || State.Placement.Get() != Placement || State.Tag != Tag || State.Status != Context.RailStatus))
    {
        UE_LOG(LogKataCamera, Warning, TEXT("Camera data %s, pawn %s, rail %s: %s. Using Boom Arm fallback for this layer."),
            *GetNameSafe(Context.CameraData), *GetNameSafe(ViewPawn), *Tag.ToString(), GetRailDiagnostic(Context.RailStatus));
    }
    State.Pawn = ViewPawn;
    State.Placement = Placement;
    State.Tag = Tag;
    State.Status = Context.RailStatus;
}

void AKataPlayerCameraManager::RunFeatures(EKataCameraStage Stage, FKataCameraPipelineContext& Context)
{
    for (UKataCameraFeature* Feature : OrderedFeatures)
    {
        if (Feature != nullptr && Feature->GetStage() == Stage && Feature->IsEnabled())
        {
            Feature->Evaluate(Context);
        }
    }
}

void AKataPlayerCameraManager::ApplyPitchLimits(const FKataCameraLayerPose* Pose)
{
    if (Pose != nullptr && FMath::IsFinite(Pose->PitchMin) && FMath::IsFinite(Pose->PitchMax) && Pose->PitchMin <= Pose->PitchMax)
    {
        ViewPitchMin = Pose->PitchMin;
        ViewPitchMax = Pose->PitchMax;
        return;
    }

    // Blueprint 파생 클래스에서 바꾼 기본값을 존중하기 위해 이 클래스의 CDO에서 되돌린다.
    const APlayerCameraManager* Defaults = GetDefault<APlayerCameraManager>(GetClass());
    ViewPitchMin = Defaults->ViewPitchMin;
    ViewPitchMax = Defaults->ViewPitchMax;
}
