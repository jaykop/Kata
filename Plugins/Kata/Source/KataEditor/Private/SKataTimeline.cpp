#include "SKataTimeline.h"

#include "Framework/Application/SlateApplication.h"
#include "Framework/Commands/UICommandList.h"
#include "Fonts/FontMeasure.h"
#include "InputCoreTypes.h"
#include "Rendering/DrawElements.h"
#include "Styling/AppStyle.h"

namespace
{
    constexpr float LabelWidth = 190.0f;
    constexpr float RulerHeight = 28.0f;
    constexpr float RowHeight = 34.0f;
    /** 길이 조절 손잡이의 폭과 막대의 최소 표시 폭. */
    constexpr float EdgeHandleWidth = 6.0f;
    constexpr float MinimumBarWidth = 8.0f;

    /** 여러 줄 주석을 한 줄로 합친다. 타임라인의 글자 그리기는 개행을 처리하지 못한다. */
    FString FlattenComment(const FString& Comment)
    {
        FString Flattened = Comment;
        Flattened.ReplaceInline(TEXT("\r\n"), TEXT(" | "));
        Flattened.ReplaceInline(TEXT("\n"), TEXT(" | "));
        Flattened.ReplaceInline(TEXT("\r"), TEXT(" | "));
        Flattened.ReplaceInline(TEXT("\t"), TEXT(" "));
        return Flattened.TrimStartAndEnd();
    }

    float MeasureTextWidth(const FString& Value, const FSlateFontInfo& Font)
    {
        return static_cast<float>(FSlateApplication::Get().GetRenderer()
            ->GetFontMeasureService()->Measure(Value, Font).X);
    }

    /**
     * 주어진 폭에 맞춰 글자를 자르고 말줄임표를 붙인다.
     * 글자 수가 아니라 실제 폭으로 재므로 한글처럼 폭이 넓은 글자도 클립을 넘지 않는다.
     */
    FString FitTextToWidth(const FString& Value, const FSlateFontInfo& Font, float MaxWidth)
    {
        if (Value.IsEmpty() || MaxWidth <= 0.0f)
        {
            return FString();
        }
        if (MeasureTextWidth(Value, Font) <= MaxWidth)
        {
            return Value;
        }
        const FString Ellipsis = TEXT("...");
        const float EllipsisWidth = MeasureTextWidth(Ellipsis, Font);
        if (EllipsisWidth > MaxWidth)
        {
            return FString();
        }
        int32 Count = Value.Len() - 1;
        while (Count > 0 && MeasureTextWidth(Value.Left(Count), Font) + EllipsisWidth > MaxWidth)
        {
            --Count;
        }
        return Count > 0 ? Value.Left(Count) + Ellipsis : FString();
    }
}

void SKataTimeline::Construct(const FArguments& Args)
{
    SetClipping(EWidgetClipping::ClipToBounds);
    OnSelect = Args._OnSelect;
    OnMove = Args._OnMove;
    OnSeek = Args._OnSeek;
    OnToggleGroup = Args._OnToggleGroup;
    OnSelectGroup = Args._OnSelectGroup;
    OnContextMenu = Args._OnContextMenu;
    CommandList = Args._CommandList;
    Playhead = Args._Playhead;
    ViewDuration = Args._ViewDuration;
    SnapInterval = Args._SnapInterval;
    SnapEnabled = Args._SnapEnabled;
    CommentDisplay = Args._CommentDisplay;
    // 툴팁은 한 번만 만들고 내용만 바꾼다. 마우스를 움직일 때마다 새로 만들면
    // 슬레이트가 툴팁이 바뀐 것으로 보고 팝업을 닫았다가 다시 소환해 사실상 뜨지 않는다.
    SetToolTipText(TAttribute<FText>::CreateSP(this, &SKataTimeline::GetHoveredCommentText));
}

float SKataTimeline::GetGridStep(const FGeometry& Geometry) const
{
    const float Duration = FMath::Max(0.1f, ViewDuration.Get(5.0f));
    float Step = FMath::Max(0.001f, SnapInterval.Get(0.5f));
    // 선 간격이 8픽셀보다 좁아지면 배수로 늘려 눈금을 읽을 수 있게 한다.
    const float Width = FMath::Max(1.0f, Geometry.GetLocalSize().X - LabelWidth);
    while (Step / Duration * Width < 8.0f)
    {
        Step *= 2.0f;
    }
    return Step;
}

float SKataTimeline::SnapTime(const FGeometry& Geometry, float Time, int32 IgnoreRow) const
{
    if (!SnapEnabled.Get(true))
    {
        return FMath::Max(0.0f, FMath::GridSnap(Time, 0.001f));
    }
    const float Interval = FMath::Max(0.001f, SnapInterval.Get(0.5f));
    float Best = FMath::GridSnap(Time, Interval);
    float BestDistance = FMath::Abs(Best - Time);
    // 자석 범위는 화면 기준 8픽셀을 시간으로 환산한 값이다.
    const float Threshold = 8.0f / FMath::Max(1.0f, Geometry.GetLocalSize().X - LabelWidth)
        * FMath::Max(0.1f, ViewDuration.Get(5.0f));
    for (int32 Index = 0; Index < Rows.Num(); ++Index)
    {
        if (Index == IgnoreRow)
        {
            continue;
        }
        const float Edges[] = { Rows[Index].Start, Rows[Index].Start + Rows[Index].Duration };
        for (float Edge : Edges)
        {
            const float Distance = FMath::Abs(Edge - Time);
            if (Distance < BestDistance && Distance <= Threshold)
            {
                Best = Edge;
                BestDistance = Distance;
            }
        }
    }
    return FMath::Max(0.0f, Best);
}

void SKataTimeline::SetRows(TArray<FKataTimelineRow> InRows, const TSet<FKataTaskId>& InSelected)
{
    Rows = MoveTemp(InRows);
    Selected = InSelected;
    // 행 구성이 바뀌면 호버 대상이 다른 행을 가리킬 수 있다. 다음 마우스 이동에서 다시 정한다.
    HoveredTaskId.Invalidate();
    HoveredGroupId.Invalidate();
    Invalidate(EInvalidateWidgetReason::Layout);
}

FVector2D SKataTimeline::ComputeDesiredSize(float) const
{
    return FVector2D(700.0f, RulerHeight + RowHeight * FMath::Max(3, Rows.Num()));
}

float SKataTimeline::TimeAt(const FGeometry& Geometry, float X) const
{
    return FMath::Clamp((X - LabelWidth) / FMath::Max(1.0f, Geometry.GetLocalSize().X - LabelWidth),
        0.0f, 1.0f) * FMath::Max(0.1f, ViewDuration.Get(5.0f));
}

float SKataTimeline::XAt(const FGeometry& Geometry, float Time) const
{
    return LabelWidth + Time / FMath::Max(0.1f, ViewDuration.Get(5.0f))
        * FMath::Max(1.0f, Geometry.GetLocalSize().X - LabelWidth);
}

int32 SKataTimeline::RowAt(const FVector2D& Local) const
{
    if (Local.Y < RulerHeight)
    {
        return INDEX_NONE;
    }
    const int32 Index = FMath::FloorToInt((Local.Y - RulerHeight) / RowHeight);
    return Rows.IsValidIndex(Index) ? Index : INDEX_NONE;
}

void SKataTimeline::UpdateHoveredRow(const FVector2D& Local)
{
    HoveredTaskId.Invalidate();
    HoveredGroupId.Invalidate();
    const int32 Index = RowAt(Local);
    if (!Rows.IsValidIndex(Index))
    {
        return;
    }
    // 라벨 칸과 트랙 어디에 올려도 그 행의 주석을 보여준다.
    if (Rows[Index].bGroupHeader)
    {
        HoveredGroupId = Rows[Index].GroupId;
    }
    else
    {
        HoveredTaskId = Rows[Index].Id;
    }
}

FText SKataTimeline::GetHoveredCommentText() const
{
    if (CommentDisplay.Get(EKataTimelineCommentDisplay::Tooltip) == EKataTimelineCommentDisplay::Hidden)
    {
        return FText::GetEmpty();
    }
    for (const FKataTimelineRow& Row : Rows)
    {
        const bool bHovered = Row.bGroupHeader
            ? (HoveredGroupId.IsValid() && Row.GroupId == HoveredGroupId)
            : (HoveredTaskId.IsValid() && Row.Id == HoveredTaskId);
        if (!bHovered)
        {
            continue;
        }
        // 주석이 없으면 빈 값을 반환해 툴팁 자체를 띄우지 않는다.
        if (Row.Comment.IsEmpty())
        {
            return FText::GetEmpty();
        }
        // 그룹은 어느 그룹의 주석인지 알 수 있게 제목을 함께 보여준다.
        return FText::FromString(Row.bGroupHeader
            ? Row.Label + TEXT("\n") + Row.Comment : Row.Comment);
    }
    return FText::GetEmpty();
}

int32 SKataTimeline::OnPaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& Cull,
    FSlateWindowElementList& Elements, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const
{
    const FSlateBrush* Brush = FAppStyle::GetBrush("WhiteBrush");
    const FSlateFontInfo Font = FAppStyle::GetFontStyle("SmallFont");
    const FVector2D Size = Geometry.GetLocalSize();
    auto Box = [&](float X, float Y, float W, float H, FLinearColor Color)
    {
        FSlateDrawElement::MakeBox(Elements, Layer, Geometry.ToPaintGeometry(FVector2D(W, H),
            FSlateLayoutTransform(FVector2D(X, Y))), Brush, ESlateDrawEffect::None, Color);
    };
    auto Text = [&](float X, float Y, const FString& Value, FLinearColor Color)
    {
        FSlateDrawElement::MakeText(Elements, Layer + 1,
            Geometry.ToPaintGeometry(FVector2D(Size.X - X, RowHeight), FSlateLayoutTransform(FVector2D(X, Y))),
            Value, Font, ESlateDrawEffect::None, Color);
    };
    const bool bInlineComments = CommentDisplay.Get(EKataTimelineCommentDisplay::Tooltip)
        == EKataTimelineCommentDisplay::Inline;
    Box(0, 0, Size.X, Size.Y, FLinearColor(0.035f, 0.035f, 0.04f));
    Text(8, 6, TEXT("Tasks / Seconds"), FLinearColor::White);
    const float Duration = FMath::Max(0.1f, ViewDuration.Get(5.0f));
    const float GridStep = GetGridStep(Geometry);
    const int32 StepCount = FMath::Min(2000, FMath::FloorToInt(Duration / GridStep));
    // 눈금이 촘촘할 때는 일정 간격으로만 숫자를 표시한다.
    const int32 LabelEvery = FMath::Max(1, FMath::CeilToInt(StepCount / 12.0f));
    for (int32 Step = 0; Step <= StepCount; ++Step)
    {
        const float Time = GridStep * Step;
        const float X = XAt(Geometry, Time);
        const bool bLabelled = (Step % LabelEvery) == 0;
        Box(X, RulerHeight, 1, Size.Y - RulerHeight, bLabelled
            ? FLinearColor(0.18f, 0.18f, 0.2f) : FLinearColor(0.11f, 0.11f, 0.13f));
        if (bLabelled)
        {
            Text(X + 2, 6, FString::Printf(TEXT("%.2f"), Time), FLinearColor(0.65f, 0.65f, 0.65f));
        }
    }
    // 눈금 영역과 태스크 배치 영역의 경계선. 눈금 세로선보다 밝게 그려 가로 경계가 먼저 읽히게 한다.
    // 행은 RulerHeight부터 그리므로 한 줄 위에 두면 어떤 행에도 덮이지 않는다.
    Box(0, RulerHeight - 1.0f, Size.X, 1.0f, FLinearColor(0.34f, 0.35f, 0.38f));
    for (int32 Index = 0; Index < Rows.Num(); ++Index)
    {
        const FKataTimelineRow& Row = Rows[Index];
        const float Y = RulerHeight + Index * RowHeight;
        if (Row.bGroupHeader)
        {
            FLinearColor HeaderColor = Row.DisplayColor * 0.28f;
            if (Row.bGroupSelected)
            {
                HeaderColor += FLinearColor(0.12f, 0.12f, 0.12f);
            }
            HeaderColor.A = 1.0f;
            Box(0, Y, Size.X, RowHeight - 1, HeaderColor);
            Box(0, Y, 5.0f, RowHeight - 1, Row.DisplayColor);
            const FString HeaderText = FString::Printf(TEXT("%s %s"),
                Row.bGroupCollapsed ? TEXT(">") : TEXT("v"), *Row.Label);
            Text(10, Y + 9, HeaderText, FLinearColor(0.9f, 0.94f, 0.98f));
            if (bInlineComments && !Row.Comment.IsEmpty())
            {
                // 제목 오른쪽에 남는 폭만큼만 주석을 적는다. 제목 자체는 줄이지 않는다.
                const float CommentX = 10.0f + MeasureTextWidth(HeaderText, Font) + 12.0f;
                const FString Comment = FitTextToWidth(FlattenComment(Row.Comment), Font, Size.X - CommentX - 8.0f);
                if (!Comment.IsEmpty())
                {
                    Text(CommentX, Y + 9, Comment, FLinearColor(0.72f, 0.78f, 0.84f));
                }
            }
            if (Row.bGroupSelected)
            {
                Box(0, Y, Size.X, 2.0f, FLinearColor(1.0f, 0.72f, 0.12f));
                Box(0, Y + RowHeight - 3.0f, Size.X, 2.0f, FLinearColor(1.0f, 0.72f, 0.12f));
            }
            continue;
        }
        const bool bSelected = Selected.Contains(Row.Id);
        const bool bGrouped = Row.GroupId.IsValid();
        FLinearColor LabelBackground = bSelected
            ? FLinearColor(0.12f, 0.24f, 0.32f) : FLinearColor(0.09f, 0.09f, 0.1f);
        if (bGrouped)
        {
            LabelBackground = FMath::Lerp(LabelBackground, Row.GroupColor, bSelected ? 0.18f : 0.30f);
            LabelBackground.A = 1.0f;
            FLinearColor TrackTint = Row.GroupColor;
            TrackTint.A = 0.08f;
            Box(LabelWidth, Y, Size.X - LabelWidth, RowHeight - 1, TrackTint);
        }
        Box(0, Y, LabelWidth, RowHeight - 1, LabelBackground);
        const float LabelX = bGrouped ? 28.0f : 8.0f;
        if (bGrouped)
        {
            // 연속된 세로 레일과 가지선으로 그룹 헤더 아래의 계층을 명시한다.
            Box(10.0f, Y, 3.0f, RowHeight - 1, Row.GroupColor);
            Box(10.0f, Y + RowHeight * 0.5f, 12.0f, 2.0f, Row.GroupColor);
        }
        Text(LabelX, Y + 9, Row.Label.Left(24) + (Row.bInherited ? TEXT(" [P]") : TEXT("")), FLinearColor::White);
        const float X = XAt(Geometry, Row.Start);
        const float Width = FMath::Max(MinimumBarWidth, XAt(Geometry, Row.Start + Row.Duration) - X);
        const FLinearColor BarColor = Row.bEnabled ? Row.DisplayColor : FLinearColor(0.3f, 0.3f, 0.3f);
        Box(X, Y + 6, Width, RowHeight - 12, BarColor);
        if (!Row.bSingleFrame)
        {
            // 양쪽 끝에 길이 조절 손잡이를 표시한다. 한 프레임 태스크는 길이를 바꿀 수 없다.
            const float HandleWidth = FMath::Min(4.0f, Width * 0.5f);
            Box(X, Y + 6, HandleWidth, RowHeight - 12, FLinearColor(0.6f, 0.8f, 0.9f));
            Box(X + Width - HandleWidth, Y + 6, HandleWidth, RowHeight - 12, FLinearColor(0.6f, 0.8f, 0.9f));
        }
        if (bInlineComments && !Row.Comment.IsEmpty())
        {
            // 막대 폭에 맞춰 줄이므로 클립 밖으로 넘치지 않는다.
            const FString Comment = FitTextToWidth(FlattenComment(Row.Comment), Font, Width - 10.0f);
            if (!Comment.IsEmpty())
            {
                // 막대 색이 밝으면 흰 글자가 묻힌다.
                const FLinearColor CommentColor = BarColor.GetLuminance() > 0.45f
                    ? FLinearColor(0.04f, 0.04f, 0.05f) : FLinearColor::White;
                Text(X + 5, Y + 9, Comment, CommentColor);
            }
        }
        if (bSelected)
        {
            // 선택 행뿐 아니라 실제 태스크 클립의 외곽선도 강조한다.
            const FLinearColor Outline(1.0f, 0.72f, 0.12f);
            constexpr float Thickness = 2.0f;
            Box(X, Y + 6, Width, Thickness, Outline);
            Box(X, Y + RowHeight - 8, Width, Thickness, Outline);
            Box(X, Y + 6, Thickness, RowHeight - 12, Outline);
            Box(X + Width - Thickness, Y + 6, Thickness, RowHeight - 12, Outline);
        }
    }
    const float HeadX = XAt(Geometry, Playhead.Get(0.0f));
    Box(HeadX, 0, 2, Size.Y, FLinearColor(1.0f, 0.35f, 0.15f));
    // 눈금에서 잡을 수 있는 재생 헤드 손잡이를 표시한다.
    Box(HeadX - 4, RulerHeight - 7, 10, 7, FLinearColor(1.0f, 0.35f, 0.15f));
    return Layer + 2;
}

int32 SKataTimeline::SelectRowAt(const FVector2D& Local, bool bToggle)
{
    const int32 Index = RowAt(Local);
    if (!Rows.IsValidIndex(Index) || Rows[Index].bGroupHeader)
    {
        return INDEX_NONE;
    }
    const FKataTaskId Id = Rows[Index].Id;
    if (bToggle)
    {
        if (Selected.Contains(Id))
        {
            Selected.Remove(Id);
        }
        else
        {
            Selected.Add(Id);
        }
    }
    else
    {
        Selected.Reset();
        Selected.Add(Id);
    }
    OnSelect.ExecuteIfBound(Id, bToggle);
    return Index;
}

SKataTimeline::EKataTimelineHandle SKataTimeline::HitTest(const FGeometry& Geometry, const FVector2D& Local, int32& OutRow) const
{
    OutRow = INDEX_NONE;
    if (Local.X < LabelWidth)
    {
        return EKataTimelineHandle::None;
    }
    const int32 Index = RowAt(Local);
    if (!Rows.IsValidIndex(Index) || Rows[Index].bGroupHeader)
    {
        return EKataTimelineHandle::None;
    }
    const float Left = XAt(Geometry, Rows[Index].Start);
    const float Right = FMath::Max(Left + MinimumBarWidth, XAt(Geometry, Rows[Index].Start + Rows[Index].Duration));
    if (Local.X < Left || Local.X > Right)
    {
        return EKataTimelineHandle::None;
    }
    OutRow = Index;
    if (Rows[Index].bSingleFrame)
    {
        // 한 프레임 태스크는 길이가 없다. 이동만 허용한다.
        return EKataTimelineHandle::Body;
    }
    // 막대가 좁으면 양쪽 손잡이가 겹치므로 절반씩 나눈다.
    const float Handle = FMath::Min(EdgeHandleWidth, (Right - Left) * 0.5f);
    if (Local.X >= Right - Handle)
    {
        return EKataTimelineHandle::EndEdge;
    }
    if (Local.X <= Left + Handle)
    {
        return EKataTimelineHandle::StartEdge;
    }
    return EKataTimelineHandle::Body;
}

FCursorReply SKataTimeline::OnCursorQuery(const FGeometry& Geometry, const FPointerEvent& Event) const
{
    if (DragHandle == EKataTimelineHandle::StartEdge || DragHandle == EKataTimelineHandle::EndEdge)
    {
        return FCursorReply::Cursor(EMouseCursor::ResizeLeftRight);
    }
    if (DragHandle == EKataTimelineHandle::Body)
    {
        return FCursorReply::Cursor(EMouseCursor::CardinalCross);
    }
    int32 Row = INDEX_NONE;
    const FVector2D Local = Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition());
    switch (HitTest(Geometry, Local, Row))
    {
    case EKataTimelineHandle::StartEdge:
    case EKataTimelineHandle::EndEdge:
        return FCursorReply::Cursor(EMouseCursor::ResizeLeftRight);
    case EKataTimelineHandle::Body:
        return FCursorReply::Cursor(EMouseCursor::CardinalCross);
    default:
        break;
    }
    if (Local.X >= LabelWidth)
    {
        return FCursorReply::Cursor(EMouseCursor::Crosshairs);
    }
    return FCursorReply::Cursor(EMouseCursor::Default);
}

FReply SKataTimeline::OnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event)
{
    const FVector2D Local = Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition());
    if (Event.GetEffectingButton() == EKeys::RightMouseButton)
    {
        // 메뉴는 버튼을 놓을 때 띄우고, 누른 위치의 행과 시각을 기억한다.
        const int32 Row = RowAt(Local);
        MenuGroupId = Rows.IsValidIndex(Row) && Rows[Row].bGroupHeader ? Rows[Row].GroupId : FGuid();
        if (!MenuGroupId.IsValid() && (!Rows.IsValidIndex(Row) || !Selected.Contains(Rows[Row].Id)))
        {
            SelectRowAt(Local, false);
        }
        MenuTime = TimeAt(Geometry, Local.X);
        bMenuPending = true;
        return FReply::Handled().CaptureMouse(SharedThis(this)).SetUserFocus(SharedThis(this), EFocusCause::Mouse);
    }
    if (Event.GetEffectingButton() != EKeys::LeftMouseButton)
    {
        return FReply::Unhandled();
    }
    const int32 ClickedRow = RowAt(Local);
    if (Rows.IsValidIndex(ClickedRow) && Rows[ClickedRow].bGroupHeader)
    {
        // 화살표 영역은 접기/펼치기, 나머지 헤더는 Timeline Details 선택에 사용한다.
        if (Local.X < 28.0f)
        {
            OnToggleGroup.ExecuteIfBound(Rows[ClickedRow].GroupId);
        }
        else
        {
            OnSelectGroup.ExecuteIfBound(Rows[ClickedRow].GroupId);
        }
        return FReply::Handled().SetUserFocus(SharedThis(this), EFocusCause::Mouse);
    }
    const bool bToggleSelection = Event.IsControlDown() || Event.IsShiftDown();
    int32 Index = INDEX_NONE;
    const EKataTimelineHandle Handle = HitTest(Geometry, Local, Index);
    if (Handle != EKataTimelineHandle::None && Rows.IsValidIndex(Index))
    {
        SelectRowAt(Local, bToggleSelection);
        if (bToggleSelection)
        {
            // 복수 선택 제스처는 선택만 바꾸고 단일 클립 드래그를 시작하지 않는다.
            return FReply::Handled().SetUserFocus(SharedThis(this), EFocusCause::Mouse);
        }
        DragRow = Index;
        DragHandle = Handle;
        DragOrigin = TimeAt(Geometry, Local.X);
        InitialStart = Rows[Index].Start;
        InitialDuration = Rows[Index].Duration;
        return FReply::Handled().CaptureMouse(SharedThis(this)).SetUserFocus(SharedThis(this), EFocusCause::Mouse);
    }

    if (Local.X >= LabelWidth)
    {
        // 눈금과 태스크가 없는 시간 영역은 모두 재생 헤드 탐색에 사용한다.
        bSeek = true;
        OnSeek.ExecuteIfBound(TimeAt(Geometry, Local.X));
        // Slate는 마우스를 누르고 있는 동안 기본으로 실시간 뷰포트 갱신을 멈춘다.
        // 탐색 드래그 중에도 프리뷰가 따라와야 하므로 이 캡처에서는 스로틀을 막는다.
        return FReply::Handled().CaptureMouse(SharedThis(this)).SetUserFocus(SharedThis(this), EFocusCause::Mouse)
            .PreventThrottling();
    }

    SelectRowAt(Local, bToggleSelection);
    return FReply::Handled().SetUserFocus(SharedThis(this), EFocusCause::Mouse);
}

FReply SKataTimeline::OnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event)
{
    if (HasMouseCapture())
    {
        // 드래그와 재생 헤드 이동 중에는 툴팁을 띄우지 않는다.
        HoveredTaskId.Invalidate();
        HoveredGroupId.Invalidate();
    }
    if (HasMouseCapture() && bSeek)
    {
        OnSeek.ExecuteIfBound(TimeAt(Geometry, Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition()).X));
        Invalidate(EInvalidateWidgetReason::Paint);
        return FReply::Handled();
    }
    if (HasMouseCapture() && Rows.IsValidIndex(DragRow))
    {
        const float Delta = TimeAt(Geometry, Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition()).X) - DragOrigin;
        const float InitialEnd = InitialStart + InitialDuration;
        if (DragHandle == EKataTimelineHandle::EndEdge)
        {
            // 오른쪽 끝은 시작 시각을 고정하고 끝 시각만 옮긴다.
            const float End = FMath::Max(InitialStart, SnapTime(Geometry, InitialEnd + Delta, DragRow));
            Rows[DragRow].Start = InitialStart;
            Rows[DragRow].Duration = End - InitialStart;
        }
        else if (DragHandle == EKataTimelineHandle::StartEdge)
        {
            // 왼쪽 끝은 끝 시각을 고정하고 시작 시각만 옮긴다.
            const float Start = FMath::Min(InitialEnd, SnapTime(Geometry, InitialStart + Delta, DragRow));
            Rows[DragRow].Start = Start;
            Rows[DragRow].Duration = InitialEnd - Start;
        }
        else
        {
            Rows[DragRow].Start = SnapTime(Geometry, InitialStart + Delta, DragRow);
            Rows[DragRow].Duration = InitialDuration;
        }
        Invalidate(EInvalidateWidgetReason::Paint);
        return FReply::Handled();
    }

    UpdateHoveredRow(Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition()));
    return FReply::Unhandled();
}

FReply SKataTimeline::OnMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& Event)
{
    if (Event.GetEffectingButton() == EKeys::RightMouseButton && bMenuPending)
    {
        bMenuPending = false;
        FReply Reply = FReply::Handled();
        if (HasMouseCapture())
        {
            Reply.ReleaseMouseCapture();
        }
        const TSharedPtr<SWidget> Menu = OnContextMenu.IsBound()
            ? OnContextMenu.Execute(MenuTime, MenuGroupId) : nullptr;
        if (Menu.IsValid())
        {
            FSlateApplication::Get().PushMenu(SharedThis(this), FWidgetPath(), Menu.ToSharedRef(),
                Event.GetScreenSpacePosition(), FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu));
        }
        return Reply;
    }
    if (HasMouseCapture() && Event.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        if (bSeek)
        {
            OnSeek.ExecuteIfBound(TimeAt(Geometry, Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition()).X));
        }
        else if (Rows.IsValidIndex(DragRow))
        {
            const FKataTimelineRow Row = Rows[DragRow];
            OnMove.ExecuteIfBound(Row.Id, Row.Start, Row.Duration);
        }
        DragRow = INDEX_NONE;
        DragHandle = EKataTimelineHandle::None;
        bSeek = false;
        return FReply::Handled().ReleaseMouseCapture();
    }
    return FReply::Unhandled();
}

FReply SKataTimeline::OnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
    if (CommandList.IsValid() && CommandList->ProcessCommandBindings(Event))
    {
        return FReply::Handled();
    }
    return SLeafWidget::OnKeyDown(Geometry, Event);
}

FReply SKataTimeline::OnMouseButtonDoubleClick(const FGeometry& Geometry, const FPointerEvent& Event)
{
    const FVector2D Local = Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition());
    const int32 Index = RowAt(Local);
    if (Event.GetEffectingButton() == EKeys::LeftMouseButton
        && Rows.IsValidIndex(Index) && Rows[Index].bGroupHeader)
    {
        OnToggleGroup.ExecuteIfBound(Rows[Index].GroupId);
        return FReply::Handled().SetUserFocus(SharedThis(this), EFocusCause::Mouse);
    }
    // 슬레이트는 처리하지 않은 더블 클릭을 누름으로 되돌리지 않는다.
    // 나머지 행은 직접 누름으로 넘겨 연속 클릭에서도 드래그와 재생 헤드 이동이 끊기지 않게 한다.
    return OnMouseButtonDown(Geometry, Event);
}

void SKataTimeline::OnMouseLeave(const FPointerEvent& Event)
{
    SLeafWidget::OnMouseLeave(Event);
    HoveredTaskId.Invalidate();
    HoveredGroupId.Invalidate();
}

void SKataTimeline::OnMouseCaptureLost(const FCaptureLostEvent& Event)
{
    // 편집을 확정하지 못한 드래그는 원래 화면 값으로 되돌린다.
    if (Rows.IsValidIndex(DragRow))
    {
        Rows[DragRow].Start = InitialStart;
        Rows[DragRow].Duration = InitialDuration;
    }
    DragRow = INDEX_NONE;
    DragHandle = EKataTimelineHandle::None;
    bSeek = false;
    bMenuPending = false;
    MenuGroupId = FGuid();
    SLeafWidget::OnMouseCaptureLost(Event);
}
