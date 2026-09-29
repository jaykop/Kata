#include "KataPlayerCameraManager.h"

#include "Algo/StableSort.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "KataCameraData.h"
#include "KataCameraFeature.h"
#include "KataCameraLog.h"
#include "KataCameraPlacement.h"

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

    CameraData->Placement->Evaluate(Context);

    RunFeatures(EKataCameraStage::Framing, Context);
    RunFeatures(EKataCameraStage::Constraint, Context);
    RunFeatures(EKataCameraStage::Reaction, Context);

    OutVT.POV.Location = Context.CameraLocation;
    OutVT.POV.Rotation = Context.CameraRotation;
    OutVT.POV.FOV = Context.FieldOfView;

    DebugSnapshot.bPipelineActive = true;
    DebugSnapshot.CameraDataName = GetNameSafe(CameraData);
    DebugSnapshot.PlacementName = CameraData->Placement->GetClass()->GetDisplayNameText().ToString();
    DebugSnapshot.PivotLocation = Context.PivotLocation;
    DebugSnapshot.ViewRotation = Context.ViewRotation;
    DebugSnapshot.CameraLocation = Context.CameraLocation;
    DebugSnapshot.CameraRotation = Context.CameraRotation;
    DebugSnapshot.FieldOfView = Context.FieldOfView;
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
    if (CameraData != nullptr)
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
