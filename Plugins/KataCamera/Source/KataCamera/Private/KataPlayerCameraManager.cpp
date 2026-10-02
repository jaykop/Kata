#include "KataPlayerCameraManager.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Algo/StableSort.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "KataCameraData.h"
#include "KataCameraFeature.h"
#include "KataCameraLog.h"
#include "KataCameraPlacement.h"
#include "KataCameraRailComponent.h"
#include "StateTree.h"
#include "StateTreeExecutionContext.h"

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

float FKataCameraBlendLayer::GetWeight() const
{
    if (bFrozen || BlendTime <= 0.0f)
    {
        return 1.0f;
    }
    return ApplyBlendCurve(BlendCurve, FMath::Clamp(Elapsed / BlendTime, 0.0f, 1.0f));
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

    Super::EndPlay(EndPlayReason);
}

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
        DebugSnapshot = FKataCameraDebugSnapshot();
        DebugSnapshot.CameraDataName = GetNameSafe(GetActiveCameraData());
        Super::UpdateViewTargetInternal(OutVT, DeltaTime);
        return;
    }

    UpdateStateTree(ViewPawn, DeltaTime);
    if (bStateTreeRunning && StateTreeRequest.Serial != AppliedRequestSerial)
    {
        AppliedRequestSerial = StateTreeRequest.Serial;
        PushBlendLayer(StateTreeRequest.CameraData, StateTreeRequest.BlendTime, StateTreeRequest.BlendCurve, StateTreeRequest.OffsetBlend);
    }
    if (!bStateTreeRunning || AppliedRequestSerial == 0)
    {
        // 트리가 없거나 아직 요청하지 않았으면 기본 데이터를 즉시 쓴다. 에디터에서 기본 데이터를 바꿔도 다음 프레임에 반영된다.
        if (BlendLayers.IsEmpty() || BlendLayers.Last().CameraData != DefaultCameraData)
        {
            BlendLayers.Reset();
            PushBlendLayer(DefaultCameraData, 0.0f, EKataCameraBlendCurve::Linear, EKataCameraOffsetBlend::Linear);
        }
    }

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
        DebugSnapshot = FKataCameraDebugSnapshot();
        DebugSnapshot.CameraDataName = GetNameSafe(GetActiveCameraData());
        Super::UpdateViewTargetInternal(OutVT, DeltaTime);
        return;
    }

    ApplyPitchLimits(&Result);

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

    RunFeatures(EKataCameraStage::Framing, Context);
    RunFeatures(EKataCameraStage::Constraint, Context);
    RunFeatures(EKataCameraStage::Reaction, Context);

    OutVT.POV.Location = Context.CameraLocation;
    OutVT.POV.Rotation = Context.CameraRotation;
    OutVT.POV.FOV = Context.FieldOfView;

    DebugSnapshot = FKataCameraDebugSnapshot();
    DebugSnapshot.bPipelineActive = true;
    DebugSnapshot.CameraDataName = GetNameSafe(GetActiveCameraData());
    DebugSnapshot.PivotLocation = Context.PivotLocation;
    DebugSnapshot.ViewRotation = Context.ViewRotation;
    DebugSnapshot.CameraLocation = Context.CameraLocation;
    DebugSnapshot.CameraRotation = Context.CameraRotation;
    DebugSnapshot.FieldOfView = Context.FieldOfView;
    DebugSnapshot.OrbitOffset = Context.OrbitOffset;
    DebugSnapshot.AimOffset = Context.AimOffset;
    DebugSnapshot.StateTreeName = GetNameSafe(CameraStateTree.GetStateTree());
    DebugSnapshot.bStateTreeRunning = bStateTreeRunning;
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

    // X·Y 오프셋을 Yaw 기준으로 돌려 어깨 너머 오프셋이 시점을 돌려도 화면의 같은 쪽에 머물게 한다.
    const FVector& DataPivotOffset = CameraData->PivotOffset;
    const FVector PlanarOffset = FRotator(0.0, OutContext.ViewRotation.Yaw, 0.0).RotateVector(FVector(DataPivotOffset.X, DataPivotOffset.Y, 0.0));
    OutContext.PivotLocation = ViewPawn->GetActorLocation() + PlanarOffset + FVector(0.0, 0.0, DataPivotOffset.Z);

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

void AKataPlayerCameraManager::PushBlendLayer(UKataCameraData* CameraData, float BlendTime, EKataCameraBlendCurve BlendCurve, EKataCameraOffsetBlend OffsetBlend)
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
