#pragma once

#include "CoreMinimal.h"
#include "EditorViewportClient.h"
#include "UnrealWidgetFwd.h"

class SKataPreviewViewport;

/**
 * 프리뷰에서 트랜스폼 위젯으로 옮길 수 있는 액터의 자리.
 *
 * 트랜스폼 위젯은 한 번에 한 곳에만 그려지므로 조작 대상도 하나만 유지한다.
 * 자리를 값으로 다루면 "둘 다 선택됨" 같은 상태가 생기지 않고,
 * 옮긴 결과를 어느 프로퍼티에 기록할지도 이 값 하나로 정해진다.
 */
enum class EKataPreviewActorSlot : uint8
{
    /** 조작하지 않는다. 카메라 조작만 남는다. */
    None,
    /** Kata를 실행하는 주체 액터. */
    Self,
    /** 대상 액터. */
    Target
};

/** 옮긴 액터의 자리와 확정된 트랜스폼을 함께 알린다. */
DECLARE_DELEGATE_TwoParams(FKataPreviewTransformChanged, EKataPreviewActorSlot, const FTransform&);

/** 뷰포트 종류 전환의 전후를 알린다. 인자는 떠나는 종류 또는 새로 들어온 종류다. */
DECLARE_DELEGATE_OneParam(FKataPreviewViewportTypeEvent, ELevelViewportType);

/**
 * 프리뷰 월드의 액터를 직접 옮길 수 있는 뷰포트 클라이언트.
 *
 * 조작할 자리를 지정하면 프리뷰 전용 선택 집합에 해당 액터를 등록하고 Unreal 트랜스폼 위젯으로
 * 이동과 회전을 처리한다. 자리가 None이면 기본 카메라 조작만 남는다.
 */
class FKataPreviewViewportClient : public FEditorViewportClient
{
public:
    FKataPreviewViewportClient(FPreviewScene* InPreviewScene, const TSharedRef<SKataPreviewViewport>& InViewport);

    /**
     * 조작할 액터와 그 자리를 함께 지정한다.
     * 자리가 None이거나 액터가 없으면 위젯을 그리지 않고 카메라 조작만 남긴다.
     * 장면을 다시 만들 때도 이 함수로 대상을 갱신한다.
     */
    void SetManipulatedActor(AActor* InActor, EKataPreviewActorSlot InSlot);

    EKataPreviewActorSlot GetManipulatedSlot() const { return ManipulatedSlot; }

    /**
     * 조작 대상의 현재 트랜스폼을 이미 기록된 값으로 간주한다.
     * 포즈 탐색처럼 편집기가 액터를 임시로 옮긴 뒤 호출해, 뷰포트 클릭만으로 그 위치가 에셋에 기록되지 않게 한다.
     */
    void SyncCommitBaseline();

    /** 원점 기준 거리·높이 측정 표시를 갱신한다. */
    void SetMeasurementSettings(FVector InEnvironmentSize, float InCellSize, bool bInShowDebugShape,
        bool bInDrawSphere, FLinearColor InColor, float InThickness);

    /** 조작을 끝냈을 때 확정된 트랜스폼을 알린다. */
    FKataPreviewTransformChanged OnTransformChanged;

    /**
     * 다른 종류로 바뀌기 직전에 떠나는 종류를 알린다. 이때 읽은 카메라 값은 아직 떠나는 종류의 것이다.
     * 같은 종류를 다시 고르면 호출하지 않는다.
     */
    FKataPreviewViewportTypeEvent OnViewportTypeLeaving;

    /** 다른 종류로 바뀐 직후에 새 종류를 알린다. 구도별 카메라 복원에 쓴다. */
    FKataPreviewViewportTypeEvent OnViewportTypeEntered;

    /** 툴바의 카메라 메뉴와 단축키가 모두 이 경로로 뷰 종류를 바꾼다. */
    virtual void SetViewportType(ELevelViewportType InViewportType) override;

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
    /** 조작할 자리와 액터가 모두 유효한지. 위젯 표시와 입력 처리의 단일 판정이다. */
    bool CanManipulateActor() const;
    /** 프리뷰 전용 선택 집합을 현재 조작 대상과 맞춘다. */
    void RefreshSelection();
    /** 원점 기준 측정 격자와 거리 구를 그린다. */
    void DrawMeasurements(FPrimitiveDrawInterface* PDI) const;
    /** 대상 트랜스폼을 에셋에 기록한다. */
    void CommitTransform();
    TWeakObjectPtr<AActor> ManipulatedActor;
    EKataPreviewActorSlot ManipulatedSlot = EKataPreviewActorSlot::None;
    UE::Widget::EWidgetMode WidgetMode;
    FVector EnvironmentSize = FVector(10000.0, 10000.0, 1000.0);
    FTransform LastCommittedTransform = FTransform::Identity;
    float CellSize = 100.0f;
    FLinearColor DebugColor = FLinearColor(0.16f, 0.55f, 0.68f, 0.65f);
    float DebugThickness = 2.0f;
    bool bManipulating = false;
    bool bShowDebugShape = false;
    bool bDrawSphere = false;
};
