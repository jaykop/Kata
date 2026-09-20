#pragma once

#include "CoreMinimal.h"
#include "EditorViewportClient.h"
#include "UnrealWidgetFwd.h"

class SKataPreviewViewport;

DECLARE_DELEGATE_OneParam(FKataTargetTransformChanged, const FTransform&);

/**
 * 프리뷰 월드의 Target Actor를 직접 옮길 수 있는 뷰포트 클라이언트.
 *
 * 선택 모드를 켜면 프리뷰 전용 선택 집합에 Target Actor를 등록하고 Unreal 트랜스폼 위젯으로
 * 이동과 회전을 처리한다. 선택 모드가 꺼져 있으면 기본 카메라 조작만 남는다.
 */
class FKataPreviewViewportClient : public FEditorViewportClient
{
public:
    FKataPreviewViewportClient(FPreviewScene* InPreviewScene, const TSharedRef<SKataPreviewViewport>& InViewport);

    /** Target 선택 모드를 켜고 끈다. */
    void SetTargetSelectionEnabled(bool bEnabled);
    bool IsTargetSelectionEnabled() const { return bTargetSelectionEnabled; }

    /** 장면을 다시 만들 때 조작 대상 액터를 갱신한다. */
    void SetTargetActor(AActor* InTargetActor);

    /** 원점 기준 거리·높이 측정 표시를 갱신한다. */
    void SetMeasurementSettings(FVector InEnvironmentSize, float InCellSize, bool bInShowDebugShape,
        bool bInDrawSphere, FLinearColor InColor, float InThickness);

    /** 조작을 끝냈을 때 확정된 트랜스폼을 알린다. */
    FKataTargetTransformChanged OnTargetTransformChanged;

    virtual void Draw(const FSceneView* View, FPrimitiveDrawInterface* PDI) override;
    virtual void TrackingStarted(const FInputEventState& InInputState, bool bIsDraggingWidget, bool bNudge) override;
    virtual void TrackingStopped() override;
    virtual bool InputWidgetDelta(FViewport* InViewport, EAxisList::Type CurrentAxis,
        FVector& Drag, FRotator& Rot, FVector& Scale) override;
    virtual bool InputKey(const FInputKeyEventArgs& EventArgs) override;
    virtual void SetWidgetMode(UE::Widget::EWidgetMode NewMode) override;
    virtual bool CanSetWidgetMode(UE::Widget::EWidgetMode NewMode) const override;
    virtual UE::Widget::EWidgetMode GetWidgetMode() const override;
    virtual FVector GetWidgetLocation() const override;
    virtual FMatrix GetWidgetCoordSystem() const override;
    virtual ECoordSystem GetWidgetCoordSystemSpace() const override;

private:
    bool CanManipulateTarget() const;
    /** 프리뷰 전용 선택 집합을 현재 Target Actor와 맞춘다. */
    void RefreshTargetSelection();
    /** 원점 기준 측정 격자와 거리 구를 그린다. */
    void DrawMeasurements(FPrimitiveDrawInterface* PDI) const;
    /** 대상 트랜스폼을 에셋에 기록한다. */
    void CommitTransform();
    TWeakObjectPtr<AActor> TargetActor;
    UE::Widget::EWidgetMode WidgetMode;
    FVector EnvironmentSize = FVector(2000.0, 2000.0, 1000.0);
    FTransform LastCommittedTransform = FTransform::Identity;
    float CellSize = 100.0f;
    FLinearColor DebugColor = FLinearColor(0.16f, 0.55f, 0.68f, 0.65f);
    float DebugThickness = 2.0f;
    bool bTargetSelectionEnabled = false;
    bool bManipulating = false;
    bool bShowDebugShape = false;
    bool bDrawSphere = false;
};
