#pragma once

#include "CoreMinimal.h"
#include "KataRuntimeTypes.h"
#include "Widgets/SLeafWidget.h"

class FUICommandList;

/** 타임라인에서 편집기 주석을 어떻게 보여줄지 정하는 표시 모드. */
enum class EKataTimelineCommentDisplay : uint8
{
    /** 주석을 표시하지 않는다. */
    Hidden,
    /** 호버한 태스크와 그룹의 주석을 툴팁으로만 보여준다. */
    Tooltip,
    /** 클립과 그룹 헤더에 주석을 그린다. 호버 툴팁도 함께 동작한다. */
    Inline,
};

/** 태스크를 끌 때 자석처럼 붙을 대상. 여러 개를 함께 켤 수 있다. */
enum class EKataTimelineSnapTarget : uint8
{
    None = 0,
    /** 다른 태스크의 시작·끝. */
    Tasks = 1 << 0,
    /** 현재 재생 헤드 위치. */
    Playhead = 1 << 1,
    /** Interval 눈금 경계. */
    Interval = 1 << 2,
    Default = Tasks | Playhead
};
ENUM_CLASS_FLAGS(EKataTimelineSnapTarget);

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
/**
 * 태스크(첫 인자)를 대상 그룹(둘째 인자, 유효하지 않으면 최상위)의 지정 항목(셋째 인자) 앞으로 옮긴다.
 * 셋째 인자는 그룹 안이면 태스크의 TaskId 값, 최상위면 태스크의 TaskId 값이나 그룹의 GroupId다.
 * 셋째 인자가 유효하지 않으면 대상 구역의 끝으로 옮긴다.
 */
DECLARE_DELEGATE_ThreeParams(FKataReorderTask, FKataTaskId, FGuid, FGuid);
/** 그룹(첫 인자)을 최상위 항목(둘째 인자, 태스크의 TaskId 값이나 그룹의 GroupId) 앞으로 옮긴다. 둘째 인자가 유효하지 않으면 맨 뒤로 옮긴다. */
DECLARE_DELEGATE_TwoParams(FKataReorderGroup, FGuid, FGuid);
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
        /** 라벨 칸을 위아래로 끌어 태스크 행을 옮길 때 호출한다. 다른 그룹으로 넣거나 그룹 밖으로 뺄 수 있다. */
        SLATE_EVENT(FKataReorderTask, OnReorder)
        /** 그룹 머리글을 위아래로 끌어 그룹 순서를 바꿀 때 호출한다. */
        SLATE_EVENT(FKataReorderGroup, OnReorderGroup)
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
        /** 켜면 드래그가 SnapTargets 중 일정 거리 안에 있는 대상에 붙는다. */
        SLATE_ATTRIBUTE(bool, SnapEnabled)
        /** 스냅할 대상. */
        SLATE_ATTRIBUTE(EKataTimelineSnapTarget, SnapTargets)
        /** 태스크와 그룹의 편집기 주석을 보여줄 방식. */
        SLATE_ATTRIBUTE(EKataTimelineCommentDisplay, CommentDisplay)
    SLATE_END_ARGS()

    void Construct(const FArguments& Args);
    void SetRows(TArray<FKataTimelineRow> InRows, const TSet<FKataTaskId>& InSelected);
    virtual FVector2D ComputeDesiredSize(float) const override;
    virtual int32 OnPaint(const FPaintArgs&, const FGeometry&, const FSlateRect&,
        FSlateWindowElementList&, int32, const FWidgetStyle&, bool) const override;
    virtual FReply OnMouseButtonDown(const FGeometry&, const FPointerEvent&) override;
    virtual FReply OnMouseMove(const FGeometry&, const FPointerEvent&) override;
    virtual FReply OnMouseButtonUp(const FGeometry&, const FPointerEvent&) override;
    virtual FReply OnMouseButtonDoubleClick(const FGeometry&, const FPointerEvent&) override;
    virtual void OnMouseLeave(const FPointerEvent&) override;
    virtual void OnMouseCaptureLost(const FCaptureLostEvent&) override;
    virtual bool SupportsKeyboardFocus() const override { return true; }
    virtual FReply OnKeyDown(const FGeometry&, const FKeyEvent&) override;
    virtual FCursorReply OnCursorQuery(const FGeometry&, const FPointerEvent&) const override;

private:
    float TimeAt(const FGeometry&, float X) const;
    float XAt(const FGeometry&, float Time) const;
    /** 지정한 로컬 좌표가 가리키는 행 번호를 반환한다. 눈금과 빈 영역은 INDEX_NONE이다. */
    int32 RowAt(const FVector2D& Local) const;
    /** 호버 중인 태스크와 그룹을 갱신한다. 툴팁 내용은 이 값으로 정해진다. */
    void UpdateHoveredRow(const FVector2D& Local);
    /** 호버 중인 행의 주석을 반환한다. 주석이 없으면 빈 값을 반환해 툴팁을 띄우지 않는다. */
    FText GetHoveredCommentText() const;
    /** 지정한 로컬 좌표가 가리키는 행을 선택한다. 행이 없으면 선택을 유지한다. */
    int32 SelectRowAt(const FVector2D& Local, bool bToggle);
    /**
     * 켜진 스냅 대상(눈금, 다른 태스크의 시작·끝, 재생 헤드) 중 화면 8픽셀 안에서 가장 가까운 값으로 시각을 맞춘다.
     * 범위 안에 대상이 없으면 1ms 단위로만 맞춰 자유롭게 움직이게 한다.
     */
    float SnapTime(const FGeometry& Geometry, float Time, int32 IgnoreRow) const;
    /** 순서 변경 드래그를 놓을 위치. 그리기와 확정에 함께 쓴다. */
    struct FKataTimelineDrop
    {
        bool bValid = false;
        /** 삽입선을 그릴 위치. 그룹 안으로 들어가는 위치는 들여 그려 그룹 밖과 구분한다. */
        float LineX = 0.0f;
        float LineY = 0.0f;
        /** 태스크 드롭: 들어갈 그룹. 유효하지 않으면 최상위(그룹 없음)다. 그룹 드롭은 항상 최상위다. */
        FGuid GroupId;
        /** 이 항목 앞에 넣는다. 태스크의 TaskId 값이나 그룹의 GroupId이며, 유효하지 않으면 구역의 끝이다. */
        FGuid BeforeEntry;
    };
    /**
     * 태스크 행을 놓을 위치를 구한다. 행의 위쪽 절반은 그 행 앞, 아래쪽 절반은 그 행 뒤를 뜻한다.
     * 그룹 머리글의 위쪽 절반은 최상위에서 그 그룹 앞, 아래쪽 절반은 그룹의 맨 앞이다.
     * 같은 경계선이라도 위아래 행에 따라 그룹 안팎이 갈린다.
     */
    FKataTimelineDrop GetTaskDrop(const FVector2D& Local) const;
    /**
     * 그룹 머리글을 놓을 위치를 구한다. 그룹은 머리글과 소속 태스크 행을 한 덩어리로 옮기며,
     * 다른 그룹 덩어리나 그룹 없는 태스크 행 사이 어디로든 옮길 수 있다.
     */
    FKataTimelineDrop GetGroupDrop(const FVector2D& Local) const;
    /** 지정한 머리글 행에서 시작하는 그룹 덩어리의 마지막 행 번호를 반환한다. */
    int32 GetGroupBlockEnd(int32 HeaderRow) const;
    /** 순서 변경 드래그 상태를 초기화한다. */
    void ResetReorder();
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

    /** 지정한 로컬 좌표가 어느 행의 어느 부분을 가리키는지 찾는다. */
    EKataTimelineHandle HitTest(const FGeometry& Geometry, const FVector2D& Local, int32& OutRow) const;
    TArray<FKataTimelineRow> Rows;
    TSet<FKataTaskId> Selected;
    FKataSelectTask OnSelect;
    FKataMoveTask OnMove;
    FKataReorderTask OnReorder;
    FKataReorderGroup OnReorderGroup;
    FKataSeekPreview OnSeek;
    FKataToggleTimelineGroup OnToggleGroup;
    FKataSelectTimelineGroup OnSelectGroup;
    FKataTimelineMenu OnContextMenu;
    TSharedPtr<FUICommandList> CommandList;
    TAttribute<float> Playhead;
    TAttribute<float> ViewDuration;
    TAttribute<float> SnapInterval;
    TAttribute<bool> SnapEnabled;
    TAttribute<EKataTimelineSnapTarget> SnapTargets;
    TAttribute<EKataTimelineCommentDisplay> CommentDisplay;
    /** 호버 중인 태스크. 유효하지 않으면 태스크 위에 있지 않다. */
    FKataTaskId HoveredTaskId;
    /** 호버 중인 그룹 헤더. 유효하지 않으면 헤더 위에 있지 않다. */
    FGuid HoveredGroupId;
    int32 DragRow = INDEX_NONE;
    EKataTimelineHandle DragHandle = EKataTimelineHandle::None;
    bool bSeek = false;
    bool bMenuPending = false;
    float MenuTime = 0.0f;
    FGuid MenuGroupId;
    float DragOrigin = 0.0f;
    float InitialStart = 0.0f;
    float InitialDuration = 0.0f;
    /** 라벨 칸에서 누른 태스크 행이나 그룹 머리글 행. 일정 거리 이상 끌어야 순서 변경으로 취급한다. */
    int32 ReorderRow = INDEX_NONE;
    float ReorderOriginY = 0.0f;
    bool bReordering = false;
    FKataTimelineDrop ReorderDrop;
};
