#pragma once

#include "CoreMinimal.h"
#include "KataRuntimeTypes.h"
#include "Widgets/SLeafWidget.h"

class FUICommandList;

struct FKataTimelineRow
{
    FKataTaskId Id;
    FGuid GroupId;
    FString Label;
    float Start = 0.0f;
    float Duration = 0.0f;
    bool bEnabled = true;
    bool bInherited = false;
    /** 한 프레임 태스크는 길이를 조절할 수 없고 짧은 표식으로 그린다. */
    bool bSingleFrame = false;
    FLinearColor DisplayColor = FLinearColor(0.12f, 0.55f, 0.72f);
    /** 소속 그룹을 태스크 행에 표시할 때 사용하는 색상. */
    FLinearColor GroupColor = FLinearColor::Transparent;
    FString Comment;
    bool bGroupHeader = false;
    bool bGroupCollapsed = false;
    bool bGroupSelected = false;
};

DECLARE_DELEGATE_TwoParams(FKataSelectTask, FKataTaskId, bool);
DECLARE_DELEGATE_ThreeParams(FKataMoveTask, FKataTaskId, float, float);
DECLARE_DELEGATE_OneParam(FKataSeekPreview, float);
DECLARE_DELEGATE_OneParam(FKataToggleTimelineGroup, FGuid);
DECLARE_DELEGATE_OneParam(FKataSelectTimelineGroup, FGuid);
DECLARE_DELEGATE_RetVal_TwoParams(TSharedPtr<SWidget>, FKataTimelineMenu, float, FGuid);

/** 드래그 중에는 화면만 갱신하고 마우스를 놓을 때 한 번의 편집으로 확정한다. */
class SKataTimeline : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SKataTimeline) {}
        SLATE_EVENT(FKataSelectTask, OnSelect)
        SLATE_EVENT(FKataMoveTask, OnMove)
        SLATE_EVENT(FKataSeekPreview, OnSeek)
        SLATE_EVENT(FKataToggleTimelineGroup, OnToggleGroup)
        SLATE_EVENT(FKataSelectTimelineGroup, OnSelectGroup)
        /** 우클릭 위치의 시각을 받아 팝업 메뉴를 만든다. */
        SLATE_EVENT(FKataTimelineMenu, OnContextMenu)
        /** Delete, Ctrl+Z, Ctrl+C, Ctrl+V 처리를 담당하는 명령 목록. */
        SLATE_ARGUMENT(TSharedPtr<FUICommandList>, CommandList)
        SLATE_ATTRIBUTE(float, Playhead)
        SLATE_ATTRIBUTE(float, ViewDuration)
        /** 눈금과 드래그 스냅에 사용하는 간격(초). */
        SLATE_ATTRIBUTE(float, SnapInterval)
        /** 켜면 드래그를 눈금과 다른 태스크 경계에 맞춘다. */
        SLATE_ATTRIBUTE(bool, SnapEnabled)
        /** 켜면 태스크 클립 안에 편집기 주석을 표시한다. */
        SLATE_ATTRIBUTE(bool, ShowComments)
    SLATE_END_ARGS()

    void Construct(const FArguments& Args);
    void SetRows(TArray<FKataTimelineRow> InRows, const TSet<FKataTaskId>& InSelected);
    virtual FVector2D ComputeDesiredSize(float) const override;
    virtual int32 OnPaint(const FPaintArgs&, const FGeometry&, const FSlateRect&,
        FSlateWindowElementList&, int32, const FWidgetStyle&, bool) const override;
    virtual FReply OnMouseButtonDown(const FGeometry&, const FPointerEvent&) override;
    virtual FReply OnMouseMove(const FGeometry&, const FPointerEvent&) override;
    virtual FReply OnMouseButtonUp(const FGeometry&, const FPointerEvent&) override;
    virtual void OnMouseCaptureLost(const FCaptureLostEvent&) override;
    virtual bool SupportsKeyboardFocus() const override { return true; }
    virtual FReply OnKeyDown(const FGeometry&, const FKeyEvent&) override;
    virtual FCursorReply OnCursorQuery(const FGeometry&, const FPointerEvent&) const override;

private:
    float TimeAt(const FGeometry&, float X) const;
    float XAt(const FGeometry&, float Time) const;
    /** 지정한 지역 좌표가 가리키는 행을 선택한다. 행이 없으면 선택을 유지한다. */
    int32 SelectRowAt(const FVector2D& Local, bool bToggle);
    /** 눈금 간격과 다른 태스크의 시작·끝 중 가까운 값으로 시각을 맞춘다. */
    float SnapTime(const FGeometry& Geometry, float Time, int32 IgnoreRow) const;
    /** 화면에 그릴 눈금 간격. 선이 너무 촘촘해지면 배수로 늘린다. */
    float GetGridStep(const FGeometry& Geometry) const;

    /** 막대에서 마우스가 가리키는 부분. */
    enum class EKataTimelineHandle : uint8
    {
        None,
        Body,
        StartEdge,
        EndEdge
    };

    /** 지정한 지역 좌표가 어느 행의 어느 부분을 가리키는지 찾는다. */
    EKataTimelineHandle HitTest(const FGeometry& Geometry, const FVector2D& Local, int32& OutRow) const;
    TArray<FKataTimelineRow> Rows;
    TSet<FKataTaskId> Selected;
    FKataSelectTask OnSelect;
    FKataMoveTask OnMove;
    FKataSeekPreview OnSeek;
    FKataToggleTimelineGroup OnToggleGroup;
    FKataSelectTimelineGroup OnSelectGroup;
    FKataTimelineMenu OnContextMenu;
    TSharedPtr<FUICommandList> CommandList;
    TAttribute<float> Playhead;
    TAttribute<float> ViewDuration;
    TAttribute<float> SnapInterval;
    TAttribute<bool> SnapEnabled;
    TAttribute<bool> ShowComments;
    int32 DragRow = INDEX_NONE;
    EKataTimelineHandle DragHandle = EKataTimelineHandle::None;
    bool bSeek = false;
    bool bMenuPending = false;
    float MenuTime = 0.0f;
    FGuid MenuGroupId;
    float DragOrigin = 0.0f;
    float InitialStart = 0.0f;
    float InitialDuration = 0.0f;
};
