#include "SKataGraphDebugView.h"

#include "KataActionNode.h"
#include "KataEdge.h"
#include "KataGraphDebugger.h"
#include "KataGraphInstance.h"
#include "KataGraphNodeBase.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/STableRow.h"

#define LOCTEXT_NAMESPACE "KataGraphDebugView"

namespace
{
    FText GetNodeLabel(const UKataGraphNodeBase* Node)
    {
        if (Node == nullptr)
        {
            return LOCTEXT("NoNode", "-");
        }
        const FText Title = Node->GetNodeTitle();
        return Title.IsEmpty() ? Node->GetDescription() : Title;
    }

    FText GetTriggerLabel(const FGameplayTag& Tag)
    {
        return Tag.IsValid() ? FText::FromName(Tag.GetTagName()) : LOCTEXT("AutoTrigger", "Auto");
    }

    TSharedPtr<FKataGraphDebugRow> MakeRow(const FKataGraphDebugRecord& Record)
    {
        TSharedPtr<FKataGraphDebugRow> Row = MakeShared<FKataGraphDebugRow>();
        FNumberFormattingOptions TimeFormat = FNumberFormattingOptions::DefaultNoGrouping();
        TimeFormat.SetMinimumFractionalDigits(2).SetMaximumFractionalDigits(2);
        Row->Time = FText::AsNumber(Record.WorldSeconds, &TimeFormat);
        const FText From = GetNodeLabel(Record.FromNode.Get());
        const FText To = GetNodeLabel(Record.ToNode.Get());
        const FText Trigger = GetTriggerLabel(Record.TriggerTag);
        switch (Record.Event)
        {
        case EKataGraphDebugEvent::Transition:
            Row->Event = LOCTEXT("Transition", "Transition");
            Row->Detail = Record.FromNode.IsValid()
                ? FText::Format(LOCTEXT("TransitionDetail", "{0} > {1}  [{2}]"), From, To, Trigger)
                : FText::Format(LOCTEXT("EntryDetail", "Entry > {0}  [{1}]"), To, Trigger);
            Row->Node = Record.ToNode;
            break;
        case EKataGraphDebugEvent::Reserved:
            Row->Event = LOCTEXT("Reserved", "Reserved");
            Row->Detail = FText::Format(LOCTEXT("ReservedDetail", "{0} > {1}  [{2}] after action ends"), From, To, Trigger);
            Row->Node = Record.ToNode;
            break;
        case EKataGraphDebugEvent::Buffered:
            Row->Event = LOCTEXT("Buffered", "Buffered");
            Row->Detail = FText::Format(LOCTEXT("BufferedDetail", "{0}  [{1}] waiting for window"), From, Trigger);
            Row->Node = Record.FromNode;
            break;
        case EKataGraphDebugEvent::Rejected:
            Row->Event = LOCTEXT("Rejected", "Rejected");
            Row->Detail = FText::Format(LOCTEXT("RejectedDetail", "{0}  {1}  [{2}]"),
                To, UEnum::GetDisplayValueAsText(Record.StartResult), Trigger);
            Row->Node = Record.ToNode;
            break;
        case EKataGraphDebugEvent::Ended:
            Row->Event = LOCTEXT("Ended", "Ended");
            Row->Detail = FText::Format(LOCTEXT("EndedDetail", "{0}  at {1}"),
                UEnum::GetDisplayValueAsText(Record.EndReason), From);
            Row->Node = Record.FromNode;
            break;
        }
        return Row;
    }
}

void SKataGraphDebugView::Construct(const FArguments& InArgs, const TSharedRef<FKataGraphDebugger>& InDebugger)
{
    Debugger = InDebugger;
    OnJumpToNode = InArgs._OnJumpToNode;

    auto MakeLine = [this](const FText& Label, FText (SKataGraphDebugView::*Getter)() const)
    {
        return SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 8.0f, 0.0f)
            [
                SNew(STextBlock).Text(Label).ColorAndOpacity(FSlateColor::UseSubduedForeground())
            ]
            + SHorizontalBox::Slot().FillWidth(1.0f)
            [
                SNew(STextBlock).Text(this, Getter)
            ];
    };

    ChildSlot
    [
        SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight().Padding(6.0f)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight()[MakeLine(LOCTEXT("TargetLabel", "Debug Object"), &SKataGraphDebugView::GetTargetText)]
            + SVerticalBox::Slot().AutoHeight()[MakeLine(LOCTEXT("StateLabel", "State"), &SKataGraphDebugView::GetStateText)]
            + SVerticalBox::Slot().AutoHeight()[MakeLine(LOCTEXT("CurrentLabel", "Current"), &SKataGraphDebugView::GetCurrentText)]
            + SVerticalBox::Slot().AutoHeight()[MakeLine(LOCTEXT("PendingLabel", "Pending"), &SKataGraphDebugView::GetPendingText)]
        ]
        + SVerticalBox::Slot().FillHeight(1.0f)
        [
            SAssignNew(ListView, SListView<TSharedPtr<FKataGraphDebugRow>>)
            .ListItemsSource(&Rows)
            .SelectionMode(ESelectionMode::Single)
            .OnGenerateRow(this, &SKataGraphDebugView::GenerateRow)
            .OnMouseButtonDoubleClick(this, &SKataGraphDebugView::HandleRowDoubleClicked)
        ]
    ];
}

void SKataGraphDebugView::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
    SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

    const TSharedPtr<FKataGraphDebugger> DebuggerPtr = Debugger.Pin();
    const UKataGraphInstance* Instance = DebuggerPtr.IsValid() ? DebuggerPtr->GetDebugInstance() : nullptr;
    const uint32 Serial = Instance != nullptr ? Instance->GetDebugRecordSerial() : 0;
    if (Instance != ShownInstance.Get() || Serial != ShownSerial)
    {
        RebuildRows(Instance);
    }
}

void SKataGraphDebugView::RebuildRows(const UKataGraphInstance* Instance)
{
    ShownInstance = Instance;
    ShownSerial = Instance != nullptr ? Instance->GetDebugRecordSerial() : 0;
    Rows.Reset();
    if (Instance != nullptr)
    {
        const TArray<FKataGraphDebugRecord>& Records = Instance->GetDebugRecords();
        for (int32 Index = Records.Num() - 1; Index >= 0; --Index)
        {
            Rows.Add(MakeRow(Records[Index]));
        }
    }
    if (ListView.IsValid())
    {
        ListView->RequestListRefresh();
    }
}

TSharedRef<ITableRow> SKataGraphDebugView::GenerateRow(
    TSharedPtr<FKataGraphDebugRow> Row, const TSharedRef<STableViewBase>& OwnerTable)
{
    return SNew(STableRow<TSharedPtr<FKataGraphDebugRow>>, OwnerTable)
        .Padding(FMargin(4.0f, 2.0f))
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 8.0f, 0.0f)
            [
                SNew(STextBlock).Text(Row->Time).ColorAndOpacity(FSlateColor::UseSubduedForeground())
            ]
            + SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 8.0f, 0.0f)
            [
                SNew(STextBlock).Text(Row->Event)
            ]
            + SHorizontalBox::Slot().FillWidth(1.0f)
            [
                SNew(STextBlock).Text(Row->Detail)
            ]
        ];
}

void SKataGraphDebugView::HandleRowDoubleClicked(TSharedPtr<FKataGraphDebugRow> Row)
{
    if (Row.IsValid() && Row->Node.IsValid())
    {
        OnJumpToNode.ExecuteIfBound(Row->Node.Get());
    }
}

FText SKataGraphDebugView::GetTargetText() const
{
    const TSharedPtr<FKataGraphDebugger> DebuggerPtr = Debugger.Pin();
    return DebuggerPtr.IsValid() ? DebuggerPtr->GetDebugTargetLabel() : FText::GetEmpty();
}

FText SKataGraphDebugView::GetStateText() const
{
    const TSharedPtr<FKataGraphDebugger> DebuggerPtr = Debugger.Pin();
    const UKataGraphInstance* Instance = DebuggerPtr.IsValid() ? DebuggerPtr->GetDebugInstance() : nullptr;
    if (Instance == nullptr)
    {
        return LOCTEXT("NotRunning", "Not running this graph");
    }
    if (Instance->GetState() == EKataGraphInstanceState::Ended)
    {
        return FText::Format(LOCTEXT("EndedState", "Ended ({0})"), UEnum::GetDisplayValueAsText(Instance->GetEndReason()));
    }
    return UEnum::GetDisplayValueAsText(Instance->GetState());
}

FText SKataGraphDebugView::GetCurrentText() const
{
    const TSharedPtr<FKataGraphDebugger> DebuggerPtr = Debugger.Pin();
    const UKataGraphInstance* Instance = DebuggerPtr.IsValid() ? DebuggerPtr->GetDebugInstance() : nullptr;
    if (Instance == nullptr || Instance->GetCurrentNode() == nullptr)
    {
        return LOCTEXT("NoCurrent", "-");
    }
    return GetNodeLabel(Instance->GetCurrentNode());
}

FText SKataGraphDebugView::GetPendingText() const
{
    const TSharedPtr<FKataGraphDebugger> DebuggerPtr = Debugger.Pin();
    const UKataGraphInstance* Instance = DebuggerPtr.IsValid() ? DebuggerPtr->GetDebugInstance() : nullptr;
    if (Instance == nullptr || Instance->GetPendingTargetNode() == nullptr)
    {
        return LOCTEXT("NoPending", "-");
    }
    const UKataEdge* Edge = Instance->GetPendingEdge();
    return FText::Format(LOCTEXT("PendingDetail", "{0}  [{1}]"),
        GetNodeLabel(Instance->GetPendingTargetNode()),
        GetTriggerLabel(Edge != nullptr ? Edge->TriggerTag : FGameplayTag()));
}

#undef LOCTEXT_NAMESPACE
