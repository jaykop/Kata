#include "SKataTimeline.h"

#include "Framework/Application/SlateApplication.h"
#include "Framework/Commands/UICommandList.h"
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
}

void SKataTimeline::Construct(const FArguments& Args)
{
    SetClipping(EWidgetClipping::ClipToBounds);
    OnSelect = Args._OnSelect;
    OnMove = Args._OnMove;
    OnSeek = Args._OnSeek;
    OnContextMenu = Args._OnContextMenu;
    CommandList = Args._CommandList;
    Playhead = Args._Playhead;
    ViewDuration = Args._ViewDuration;
    SnapInterval = Args._SnapInterval;
    SnapEnabled = Args._SnapEnabled;
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
    for (int32 Index = 0; Index < Rows.Num(); ++Index)
    {
        const FKataTimelineRow& Row = Rows[Index];
        const float Y = RulerHeight + Index * RowHeight;
        const bool bSelected = Selected.Contains(Row.Id);
        Box(0, Y, LabelWidth, RowHeight - 1, bSelected
            ? FLinearColor(0.12f, 0.24f, 0.32f) : FLinearColor(0.09f, 0.09f, 0.1f));
        Text(8, Y + 9, Row.Label.Left(24) + (Row.bInherited ? TEXT(" [P]") : TEXT("")), FLinearColor::White);
        const float X = XAt(Geometry, Row.Start);
        const float Width = FMath::Max(MinimumBarWidth, XAt(Geometry, Row.Start + Row.Duration) - X);
        Box(X, Y + 6, Width, RowHeight - 12, Row.bEnabled
            ? FLinearColor(0.12f, 0.55f, 0.72f) : FLinearColor(0.3f, 0.3f, 0.3f));
        // 양쪽 끝에 길이 조절 손잡이를 표시한다.
        const float HandleWidth = FMath::Min(4.0f, Width * 0.5f);
        Box(X, Y + 6, HandleWidth, RowHeight - 12, FLinearColor(0.6f, 0.8f, 0.9f));
        Box(X + Width - HandleWidth, Y + 6, HandleWidth, RowHeight - 12, FLinearColor(0.6f, 0.8f, 0.9f));
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
    return Layer + 2;
}

int32 SKataTimeline::SelectRowAt(const FVector2D& Local, bool bToggle)
{
    if (Local.Y < RulerHeight)
    {
        return INDEX_NONE;
    }
    const int32 Index = FMath::FloorToInt((Local.Y - RulerHeight) / RowHeight);
    if (!Rows.IsValidIndex(Index))
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
    if (Local.Y < RulerHeight || Local.X < LabelWidth)
    {
        return EKataTimelineHandle::None;
    }
    const int32 Index = FMath::FloorToInt((Local.Y - RulerHeight) / RowHeight);
    if (!Rows.IsValidIndex(Index))
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
    if (Local.Y < RulerHeight && Local.X >= LabelWidth)
    {
        return FCursorReply::Cursor(EMouseCursor::ResizeLeftRight);
    }
    return FCursorReply::Cursor(EMouseCursor::Default);
}

FReply SKataTimeline::OnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event)
{
    const FVector2D Local = Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition());
    if (Event.GetEffectingButton() == EKeys::RightMouseButton)
    {
        // 메뉴는 버튼을 놓을 때 띄우고, 누른 위치의 행과 시각을 기억한다.
        const int32 Row = FMath::FloorToInt((Local.Y - RulerHeight) / RowHeight);
        if (!Rows.IsValidIndex(Row) || !Selected.Contains(Rows[Row].Id))
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
    if (Local.Y < RulerHeight && Local.X >= LabelWidth)
    {
        bSeek = true;
        return FReply::Handled().CaptureMouse(SharedThis(this)).SetUserFocus(SharedThis(this), EFocusCause::Mouse);
    }
    const bool bToggleSelection = Event.IsControlDown() || Event.IsShiftDown();
    SelectRowAt(Local, bToggleSelection);
    if (bToggleSelection)
    {
        // 복수 선택 제스처는 선택만 바꾸고 단일 클립 드래그를 시작하지 않는다.
        return FReply::Handled().SetUserFocus(SharedThis(this), EFocusCause::Mouse);
    }
    int32 Index = INDEX_NONE;
    const EKataTimelineHandle Handle = HitTest(Geometry, Local, Index);
    if (Handle != EKataTimelineHandle::None && Rows.IsValidIndex(Index))
    {
        DragRow = Index;
        DragHandle = Handle;
        DragOrigin = TimeAt(Geometry, Local.X);
        InitialStart = Rows[Index].Start;
        InitialDuration = Rows[Index].Duration;
        return FReply::Handled().CaptureMouse(SharedThis(this)).SetUserFocus(SharedThis(this), EFocusCause::Mouse);
    }
    return FReply::Handled().SetUserFocus(SharedThis(this), EFocusCause::Mouse);
}

FReply SKataTimeline::OnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event)
{
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
        const TSharedPtr<SWidget> Menu = OnContextMenu.IsBound() ? OnContextMenu.Execute(MenuTime) : nullptr;
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
    SLeafWidget::OnMouseCaptureLost(Event);
}
