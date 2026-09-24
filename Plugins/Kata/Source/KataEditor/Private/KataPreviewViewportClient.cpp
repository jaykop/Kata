#include "KataPreviewViewportClient.h"

#include "AssetEditorModeManager.h"
#include "EditorModeManager.h"
#include "Engine/Selection.h"
#include "GameFramework/Actor.h"
#include "InputKeyEventArgs.h"
#include "SceneManagement.h"
#include "UnrealClient.h"
#include "SKataPreviewViewport.h"
#include "UnrealWidget.h"

namespace
{
    /** 키보드 한 번에 움직일 기본량. Shift를 누르면 큰 단위를 사용한다. */
    constexpr float NudgeUnits = 10.0f;
    constexpr float LargeNudgeUnits = 50.0f;
    constexpr float NudgeDegrees = 5.0f;
    constexpr float LargeNudgeDegrees = 15.0f;
    constexpr int32 MaximumMeasurementLines = 400;
}

FKataPreviewViewportClient::FKataPreviewViewportClient(FPreviewScene* InPreviewScene,
    const TSharedRef<SKataPreviewViewport>& InViewport)
    : FEditorViewportClient(nullptr, InPreviewScene, InViewport)
    , WidgetMode(UE::Widget::WM_Translate)
{
    bShowWidget = true;
    // ITF 기즈모는 이 프리뷰에서 생성되지 않으므로 레거시 FWidget 경로를 쓰도록 고정한다.
    GetModeTools()->SetSupportsViewportITF(false);
    static_cast<FAssetEditorModeManager*>(GetModeTools())->SetPreviewScene(InPreviewScene);
    // FWidget에 ModeTools를 연결하면 FWidget::Render가 활성 레거시 에디터 모드를 요구한다.
    // 프리뷰에는 에디터 모드가 없으므로 연결하지 않고 이 클라이언트의 WidgetMode·Location만 사용한다.
    if (Widget)
    {
        // 그리드 스냅은 이동 델타를 에디터 그리드 단위로 양자화해 프리뷰 배치를 방해한다.
        Widget->SetSnapEnabled(false);
    }
}

void FKataPreviewViewportClient::SetManipulatedActor(AActor* InActor, EKataPreviewActorSlot InSlot)
{
    ManipulatedActor = InActor;
    ManipulatedSlot = InSlot;
    // 대상이 바뀌면 진행 중이던 드래그는 이어갈 수 없다.
    bManipulating = false;
    LastCommittedTransform = InActor ? InActor->GetActorTransform() : FTransform::Identity;
    RefreshSelection();
    if (Viewport)
    {
        Viewport->InvalidateHitProxy();
    }
    Invalidate();
}

void FKataPreviewViewportClient::SetMeasurementSettings(FVector InEnvironmentSize, float InCellSize,
    bool bInShowDebugShape, bool bInDrawSphere, FLinearColor InColor, float InThickness)
{
    EnvironmentSize = InEnvironmentSize.ComponentMax(FVector(1.0));
    CellSize = FMath::Max(1.0f, InCellSize);
    bShowDebugShape = bInShowDebugShape;
    bDrawSphere = bInDrawSphere;
    DebugColor = InColor;
    DebugThickness = FMath::Max(0.0f, InThickness);
    Invalidate();
}

void FKataPreviewViewportClient::RefreshSelection()
{
    FEditorModeTools* Tools = GetModeTools();
    if (!Tools)
    {
        return;
    }
    Tools->GetSelectedActors()->DeselectAll();
    Tools->GetSelectedObjects()->DeselectAll();
    if (CanManipulateActor())
    {
        Tools->GetSelectedActors()->Select(ManipulatedActor.Get(), true);
    }
    Tools->ActorSelectionChangeNotify();
}

bool FKataPreviewViewportClient::CanManipulateActor() const
{
    return ManipulatedSlot != EKataPreviewActorSlot::None && ManipulatedActor.IsValid();
}

void FKataPreviewViewportClient::Draw(const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
    // 히트 프록시 패스에서는 측정선을 그리지 않는다. 원점을 지나는 축선이 이동 기즈모 손잡이와
    // 겹쳐 히트 프록시를 덮으면 Translate 축을 잡을 수 없다.
    if (!PDI->IsHitTesting())
    {
        DrawMeasurements(PDI);
    }
    FEditorViewportClient::Draw(View, PDI);
}

void FKataPreviewViewportClient::DrawMeasurements(FPrimitiveDrawInterface* PDI) const
{
    if (!bShowDebugShape)
    {
        return;
    }
    const double HalfX = EnvironmentSize.X * 0.5;
    const double HalfY = EnvironmentSize.Y * 0.5;
    const double Height = EnvironmentSize.Z;
    const double Step = FMath::Max(1.0, static_cast<double>(CellSize));
    const FLinearColor MinorColor = DebugColor;
    const FLinearColor MajorColor = DebugColor.CopyWithNewOpacity(FMath::Min(1.0f, DebugColor.A * 1.5f));
    const float MinorThickness = DebugThickness;
    const float MajorThickness = DebugThickness * 1.5f;
    auto DrawMeasurementLine = [&](const FVector& Start, const FVector& End, bool bMajor)
    {
        PDI->DrawLine(Start, End, bMajor ? MajorColor : MinorColor, SDPG_World,
            bMajor ? MajorThickness : MinorThickness);
    };

    if (!bDrawSphere)
    {
        const int32 XSteps = FMath::Min(MaximumMeasurementLines, FMath::CeilToInt(HalfX / Step));
        const int32 YSteps = FMath::Min(MaximumMeasurementLines, FMath::CeilToInt(HalfY / Step));
        for (int32 Index = -XSteps; Index <= XSteps; ++Index)
        {
            const double X = FMath::Clamp(Index * Step, -HalfX, HalfX);
            DrawMeasurementLine(FVector(X, -HalfY, 0.5), FVector(X, HalfY, 0.5),
                Index == 0 || Index % 5 == 0);
        }
        for (int32 Index = -YSteps; Index <= YSteps; ++Index)
        {
            const double Y = FMath::Clamp(Index * Step, -HalfY, HalfY);
            DrawMeasurementLine(FVector(-HalfX, Y, 0.5), FVector(HalfX, Y, 0.5),
                Index == 0 || Index % 5 == 0);
        }

        const int32 XWallSteps = FMath::Min(MaximumMeasurementLines, FMath::CeilToInt(HalfX / Step));
        const int32 YWallSteps = FMath::Min(MaximumMeasurementLines, FMath::CeilToInt(HalfY / Step));
        const int32 HeightSteps = FMath::Min(MaximumMeasurementLines, FMath::CeilToInt(Height / Step));
        for (int32 Index = -XWallSteps; Index <= XWallSteps; ++Index)
        {
            const double X = FMath::Clamp(Index * Step, -HalfX, HalfX);
            DrawMeasurementLine(FVector(X, 0, 0), FVector(X, 0, Height),
                Index == 0 || Index % 5 == 0);
        }
        for (int32 Index = -YWallSteps; Index <= YWallSteps; ++Index)
        {
            const double Y = FMath::Clamp(Index * Step, -HalfY, HalfY);
            DrawMeasurementLine(FVector(0, Y, 0), FVector(0, Y, Height),
                Index == 0 || Index % 5 == 0);
        }
        for (int32 Index = 0; Index <= HeightSteps; ++Index)
        {
            const double Z = FMath::Min(Index * Step, Height);
            const bool bMajor = Index == 0 || Index % 5 == 0;
            DrawMeasurementLine(FVector(-HalfX, 0, Z), FVector(HalfX, 0, Z), bMajor);
            DrawMeasurementLine(FVector(0, -HalfY, Z), FVector(0, HalfY, Z), bMajor);
        }
    }
    else
    {
        const double MaximumRadius = FMath::Max3(HalfX, HalfY, Height);
        const int32 SphereCount = FMath::Min(MaximumMeasurementLines, FMath::CeilToInt(MaximumRadius / Step));
        for (int32 Index = 1; Index <= SphereCount; ++Index)
        {
            const bool bMajor = Index % 5 == 0 || Index == SphereCount;
            const double Radius = FMath::Min(Index * Step, MaximumRadius);
            DrawWireSphere(PDI, FTransform::Identity, bMajor ? MajorColor : MinorColor,
                Radius, 48, SDPG_World, bMajor ? MajorThickness : MinorThickness);
        }
    }
}

void FKataPreviewViewportClient::TrackingStarted(const FInputEventState& InInputState, bool bIsDraggingWidget, bool bNudge)
{
    const bool bTrackingHandledExternally = GetModeTools()->StartTracking(this, Viewport);
    if (!bManipulating && bIsDraggingWidget && !bTrackingHandledExternally && CanManipulateActor())
    {
        bManipulating = true;
    }
}

void FKataPreviewViewportClient::TrackingStopped()
{
    const bool bTrackingHandledExternally = GetModeTools()->EndTracking(this, Viewport);
    const bool bWasManipulating = bManipulating;
    if (bManipulating && !bTrackingHandledExternally)
    {
        bManipulating = false;
    }
    if (bWasManipulating && !bTrackingHandledExternally)
    {
        CommitTransform();
    }
    // 드래그가 끝난 뒤 옮겨진 위치에서 축을 다시 잡을 수 있도록 히트 프록시를 갱신한다.
    if (Viewport)
    {
        Viewport->InvalidateHitProxy();
    }
}

bool FKataPreviewViewportClient::InputWidgetDelta(FViewport* InViewport, EAxisList::Type CurrentAxis,
    FVector& Drag, FRotator& Rot, FVector& Scale)
{
    if (!CanManipulateActor() || CurrentAxis == EAxisList::None)
    {
        return FEditorViewportClient::InputWidgetDelta(InViewport, CurrentAxis, Drag, Rot, Scale);
    }
    AActor* Actor = ManipulatedActor.Get();
    if (Actor == nullptr)
    {
        return false;
    }
    // 기본 처리기가 델타를 소비해도 프리뷰 액터에는 적용하지 않는 경우가 있으므로 직접 반영한다.
    bManipulating = true;
    FTransform Transform = Actor->GetActorTransform();
    switch (GetWidgetMode())
    {
    case UE::Widget::WM_Translate:
        Transform.SetLocation(Transform.GetLocation() + Drag);
        break;
    case UE::Widget::WM_Rotate:
        Transform.SetRotation((Rot.Quaternion() * Transform.GetRotation()).GetNormalized());
        break;
    default:
        return false;
    }
    // 물리 바디를 함께 옮겨 프리뷰 배치가 속도로 해석되지 않게 한다.
    Actor->SetActorTransform(Transform, false, nullptr, ETeleportType::TeleportPhysics);
    // 드래그 중에는 히트 프록시를 다시 만들지 않는다. 잡고 있는 축 판정을 유지한다.
    Invalidate();
    return true;
}

void FKataPreviewViewportClient::CommitTransform()
{
    if (const AActor* Actor = ManipulatedActor.Get())
    {
        LastCommittedTransform = Actor->GetActorTransform();
        OnTransformChanged.ExecuteIfBound(ManipulatedSlot, LastCommittedTransform);
    }
}

bool FKataPreviewViewportClient::InputKey(const FInputKeyEventArgs& EventArgs)
{
    if (CanManipulateActor() && EventArgs.Event == IE_Pressed && !IsAltPressed() && !IsCtrlPressed())
    {
        if (EventArgs.Key == EKeys::Q) { SetWidgetMode(UE::Widget::WM_None); return true; }
        if (EventArgs.Key == EKeys::W) { SetWidgetMode(UE::Widget::WM_Translate); return true; }
        if (EventArgs.Key == EKeys::E) { SetWidgetMode(UE::Widget::WM_Rotate); return true; }
    }

    if (CanManipulateActor() && EventArgs.Key == EKeys::LeftMouseButton && EventArgs.Event == IE_Released)
    {
        const bool bHandled = FEditorViewportClient::InputKey(EventArgs);
        const AActor* Actor = ManipulatedActor.Get();
        if (Actor && !Actor->GetActorTransform().Equals(LastCommittedTransform))
        {
            // Interactive Tools Framework 위젯도 마우스를 놓는 시점에 에셋 값으로 확정한다.
            CommitTransform();
        }
        return bHandled;
    }

    const bool bNudgeEvent = EventArgs.Event == IE_Pressed || EventArgs.Event == IE_Repeat;
    AActor* Actor = ManipulatedActor.Get();
    if (CanManipulateActor() && bNudgeEvent && Actor != nullptr)
    {
        const bool bLarge = IsShiftPressed();
        FVector Axis = FVector::ZeroVector;
        float Sign = 1.0f;
        if (EventArgs.Key == EKeys::Up) { Axis = FVector::ForwardVector; }
        else if (EventArgs.Key == EKeys::Down) { Axis = FVector::ForwardVector; Sign = -1.0f; }
        else if (EventArgs.Key == EKeys::Right) { Axis = FVector::RightVector; }
        else if (EventArgs.Key == EKeys::Left) { Axis = FVector::RightVector; Sign = -1.0f; }
        else if (EventArgs.Key == EKeys::PageUp) { Axis = FVector::UpVector; }
        else if (EventArgs.Key == EKeys::PageDown) { Axis = FVector::UpVector; Sign = -1.0f; }

        if (!Axis.IsNearlyZero())
        {
            FTransform Transform = Actor->GetActorTransform();
            switch (WidgetMode)
            {
            case UE::Widget::WM_Rotate:
            {
                const float Degrees = Sign * (bLarge ? LargeNudgeDegrees : NudgeDegrees);
                Transform.SetRotation((FQuat(Axis, FMath::DegreesToRadians(Degrees))
                    * Transform.GetRotation()).GetNormalized());
                break;
            }
            default:
                Transform.AddToTranslation(Axis * Sign * (bLarge ? LargeNudgeUnits : NudgeUnits));
                break;
            }
            Actor->SetActorTransform(Transform, false, nullptr, ETeleportType::TeleportPhysics);
            if (Viewport)
            {
                Viewport->InvalidateHitProxy();
            }
            Invalidate();
            CommitTransform();
            return true;
        }
    }
    return FEditorViewportClient::InputKey(EventArgs);
}

void FKataPreviewViewportClient::SyncCommitBaseline()
{
    if (const AActor* Actor = ManipulatedActor.Get())
    {
        LastCommittedTransform = Actor->GetActorTransform();
    }
}

void FKataPreviewViewportClient::SetViewportType(ELevelViewportType InViewportType)
{
    const ELevelViewportType PreviousType = GetViewportType();
    if (PreviousType == InViewportType)
    {
        FEditorViewportClient::SetViewportType(InViewportType);
        return;
    }
    // 위치와 확대 배율은 현재 종류의 트랜스폼에서만 읽을 수 있으므로 전환 전에 기록하게 한다.
    OnViewportTypeLeaving.ExecuteIfBound(PreviousType);
    FEditorViewportClient::SetViewportType(InViewportType);
    OnViewportTypeEntered.ExecuteIfBound(InViewportType);
}

void FKataPreviewViewportClient::SetWidgetMode(UE::Widget::EWidgetMode NewMode)
{
    WidgetMode = NewMode;
    GetModeTools()->SetWidgetMode(NewMode);
    if (Viewport)
    {
        Viewport->InvalidateHitProxy();
    }
    Invalidate();
}

bool FKataPreviewViewportClient::CanSetWidgetMode(UE::Widget::EWidgetMode NewMode) const
{
    // 프리뷰 배치는 이동과 회전만 지원한다. 크기 조절은 제공하지 않는다.
    return CanManipulateActor() && (NewMode == UE::Widget::WM_None || NewMode == UE::Widget::WM_Translate
        || NewMode == UE::Widget::WM_Rotate);
}

UE::Widget::EWidgetMode FKataPreviewViewportClient::GetWidgetMode() const
{
    // WM_None을 반환하면 위젯을 그리지 않는다.
    return CanManipulateActor() ? WidgetMode : UE::Widget::WM_None;
}

FVector FKataPreviewViewportClient::GetWidgetLocation() const
{
    const AActor* Actor = ManipulatedActor.Get();
    return (CanManipulateActor() && Actor) ? Actor->GetActorLocation() : FVector::ZeroVector;
}

FMatrix FKataPreviewViewportClient::GetWidgetCoordSystem() const
{
    return FMatrix::Identity;
}

ECoordSystem FKataPreviewViewportClient::GetWidgetCoordSystemSpace() const
{
    // 프리뷰 배치는 항상 월드 축을 기준으로 다룬다.
    return COORD_World;
}
