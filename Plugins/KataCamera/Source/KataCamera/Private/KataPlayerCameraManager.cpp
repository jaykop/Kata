#include "KataPlayerCameraManager.h"

#include "Algo/StableSort.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "KataCameraData.h"
#include "KataCameraFeature.h"
#include "KataCameraLog.h"
#include "KataCameraPlacement.h"
#include "KataCameraRailComponent.h"

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
}

void AKataPlayerCameraManager::InitializeFor(APlayerController* PC)
{
    Super::InitializeFor(PC);

    OrderedFeatures.Reset(Features.Num());
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
    for (UKataCameraFeature* Feature : OrderedFeatures)
    {
        if (Feature != nullptr)
        {
            Feature->Deinitialize();
        }
    }
    OrderedFeatures.Reset();
    ActiveRail.Reset();

    Super::EndPlay(EndPlayReason);
}

UKataCameraData* AKataPlayerCameraManager::GetActiveCameraData() const
{
    return DefaultCameraData;
}

void AKataPlayerCameraManager::UpdateViewTargetInternal(FTViewTarget& OutVT, float DeltaTime)
{
    const UKataCameraData* CameraData = GetActiveCameraData();
    if (!bPitchLimitsApplied || PitchLimitSource.Get() != CameraData)
    {
        ApplyPitchLimits(CameraData);
    }

    APawn* ViewPawn = Cast<APawn>(OutVT.Target);
    const bool bHasPlacement = CameraData != nullptr && CameraData->Placement != nullptr;
    if (CameraData != nullptr && !bHasPlacement && MissingPlacementWarned.Get() != CameraData)
    {
        UE_LOG(LogKataCamera, Warning, TEXT("Camera data %s has no Placement. Using the engine default camera."), *GetNameSafe(CameraData));
        MissingPlacementWarned = CameraData;
    }

    if (PCOwner == nullptr || ViewPawn == nullptr || !bHasPlacement)
    {
        ActiveRail.Reset();
        LastRailStatus = EKataCameraRailStatus::NotRequested;
        DebugSnapshot = FKataCameraDebugSnapshot();
        DebugSnapshot.CameraDataName = GetNameSafe(CameraData);
        Super::UpdateViewTargetInternal(OutVT, DeltaTime);
        return;
    }

    FKataCameraPipelineContext Context;
    Context.PlayerController = PCOwner;
    Context.ViewTarget = ViewPawn;
    Context.CameraData = CameraData;
    Context.DeltaTime = DeltaTime;
    Context.FieldOfView = CameraData->FieldOfView;
    Context.ViewRotation = PCOwner->GetControlRotation();

    RunFeatures(EKataCameraStage::Rotation, Context);

    // X·Y 오프셋을 Yaw 기준으로 돌려 어깨 너머 오프셋이 시점을 돌려도 화면의 같은 쪽에 머물게 한다.
    const FVector& DataPivotOffset = CameraData->PivotOffset;
    const FVector PlanarOffset = FRotator(0.0, Context.ViewRotation.Yaw, 0.0).RotateVector(FVector(DataPivotOffset.X, DataPivotOffset.Y, 0.0));
    Context.PivotLocation = ViewPawn->GetActorLocation() + PlanarOffset + FVector(0.0, 0.0, DataPivotOffset.Z);

    const UKataCameraPlacement_Spline* SplinePlacement = Cast<UKataCameraPlacement_Spline>(CameraData->Placement);
    ResolveRail(ViewPawn, SplinePlacement, Context);
    CameraData->Placement->Evaluate(Context);
    UpdateRailDiagnostic(ViewPawn, SplinePlacement, Context);

    RunFeatures(EKataCameraStage::Framing, Context);
    RunFeatures(EKataCameraStage::Constraint, Context);
    RunFeatures(EKataCameraStage::Reaction, Context);

    OutVT.POV.Location = Context.CameraLocation;
    OutVT.POV.Rotation = Context.CameraRotation;
    OutVT.POV.FOV = Context.FieldOfView;

    DebugSnapshot = FKataCameraDebugSnapshot();
    DebugSnapshot.bPipelineActive = true;
    DebugSnapshot.CameraDataName = GetNameSafe(CameraData);
    DebugSnapshot.PlacementName = CameraData->Placement->GetClass()->GetDisplayNameText().ToString();
    DebugSnapshot.PivotLocation = Context.PivotLocation;
    DebugSnapshot.ViewRotation = Context.ViewRotation;
    DebugSnapshot.CameraLocation = Context.CameraLocation;
    DebugSnapshot.CameraRotation = Context.CameraRotation;
    DebugSnapshot.FieldOfView = Context.FieldOfView;
    DebugSnapshot.OrbitOffset = Context.OrbitOffset;
    DebugSnapshot.AimOffset = Context.AimOffset;
    if (SplinePlacement != nullptr)
    {
        DebugSnapshot.RailTag = SplinePlacement->RailTag.ToString();
        DebugSnapshot.RailDiagnostic = GetRailDiagnostic(Context.RailStatus);
        DebugSnapshot.bUsingSpline = Context.RailStatus == EKataCameraRailStatus::Ready;
        DebugSnapshot.RailAlpha = Context.RailAlpha;
        DebugSnapshot.RailDistance = Context.RailDistance;
        if (DebugSnapshot.bUsingSpline)
        {
            DebugSnapshot.RailLength = Context.Rail->GetSplineLength();
#if WITH_GAMEPLAY_DEBUGGER
            constexpr int32 SampleCount = 33;
            DebugSnapshot.RailSamples.Reserve(SampleCount);
            for (int32 Index = 0; Index < SampleCount; ++Index)
            {
                const float Distance = DebugSnapshot.RailLength * static_cast<float>(Index) / (SampleCount - 1);
                const FVector Sample = Context.Rail->GetOrbitOffsetAtDistance(Distance);
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

void AKataPlayerCameraManager::ResolveRail(APawn* ViewPawn, const UKataCameraPlacement_Spline* Placement, FKataCameraPipelineContext& Context)
{
    ActiveRail.Reset();
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

    ActiveRail = MatchedRail;
    Context.Rail = ActiveRail.Get();
    Context.RailStatus = EKataCameraRailStatus::Ready;
}

void AKataPlayerCameraManager::UpdateRailDiagnostic(APawn* ViewPawn, const UKataCameraPlacement_Spline* Placement, const FKataCameraPipelineContext& Context)
{
    const FName Tag = Placement != nullptr ? Placement->RailTag.GetTagName() : NAME_None;
    if (Context.RailStatus != EKataCameraRailStatus::NotRequested && Context.RailStatus != EKataCameraRailStatus::Ready &&
        (LastRailPawn.Get() != ViewPawn || LastRailPlacement.Get() != Placement ||
            LastRailTag != Tag || LastRailStatus != Context.RailStatus))
    {
        UE_LOG(LogKataCamera, Warning, TEXT("Camera data %s, pawn %s, rail %s: %s. Using Boom Arm fallback."),
            *GetNameSafe(Context.CameraData), *GetNameSafe(ViewPawn), *Tag.ToString(), GetRailDiagnostic(Context.RailStatus));
    }
    LastRailPawn = ViewPawn;
    LastRailPlacement = Placement;
    LastRailTag = Tag;
    LastRailStatus = Context.RailStatus;
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

void AKataPlayerCameraManager::ApplyPitchLimits(const UKataCameraData* CameraData)
{
    if (CameraData != nullptr && FMath::IsFinite(CameraData->PitchMin) && FMath::IsFinite(CameraData->PitchMax) &&
        CameraData->PitchMin <= CameraData->PitchMax)
    {
        ViewPitchMin = CameraData->PitchMin;
        ViewPitchMax = CameraData->PitchMax;
    }
    else
    {
        // Blueprint 파생 클래스에서 바꾼 기본값을 존중하기 위해 이 클래스의 CDO에서 되돌린다.
        const APlayerCameraManager* Defaults = GetDefault<APlayerCameraManager>(GetClass());
        ViewPitchMin = Defaults->ViewPitchMin;
        ViewPitchMax = Defaults->ViewPitchMax;
    }

    PitchLimitSource = CameraData;
    bPitchLimitsApplied = true;
}
