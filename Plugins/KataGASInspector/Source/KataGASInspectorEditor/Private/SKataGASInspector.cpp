#include "SKataGASInspector.h"

#include "AbilitySystemComponent.h"
#include "Editor.h"
#include "Engine/Selection.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "GameFramework/Actor.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Misc/ConfigCacheIni.h"
#include "Styling/AppStyle.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Views/SListView.h"

namespace KataGASInspectorUI
{
    FString PageName(EKataGASInspectionPage Page)
    {
        switch (Page)
        {
        case EKataGASInspectionPage::Tags: return TEXT("Tags");
        case EKataGASInspectionPage::Attributes: return TEXT("Attributes");
        case EKataGASInspectionPage::Abilities: return TEXT("Abilities");
        case EKataGASInspectionPage::Effects: return TEXT("Active Effects");
        case EKataGASInspectionPage::Triggers: return TEXT("Ability Triggers");
        default: return FString();
        }
    }

    FString Cell(const FKataGASInspectionRow& Row, FName Column)
    {
        if (Column == TEXT("Name")) { return Row.Name; }
        if (Column == TEXT("State")) { return Row.State; }
        if (Column == TEXT("Value")) { return Row.Value; }
        if (Column == TEXT("Tags")) { return Row.Tags; }
        return Row.Source;
    }

    class SKataGASInspectorRow : public SMultiColumnTableRow<TSharedPtr<FKataGASInspectionRow>>
    {
    public:
        SLATE_BEGIN_ARGS(SKataGASInspectorRow) {}
            SLATE_ARGUMENT(TSharedPtr<FKataGASInspectionRow>, Data)
        SLATE_END_ARGS()

        void Construct(const FArguments& Args, const TSharedRef<STableViewBase>& Owner)
        {
            Data = Args._Data;
            SMultiColumnTableRow::Construct(SMultiColumnTableRow::FArguments().Padding(2.0f), Owner);
        }

        virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& Column) override
        {
            TSharedRef<STextBlock> Text = SNew(STextBlock)
                .Text_Lambda([Data = Data, Column] { return FText::FromString(Cell(*Data, Column)); })
                .ToolTipText_Lambda([Data = Data, Column]
                {
                    return FText::FromString(Cell(*Data, Column) + TEXT("\n") + Data->Detail);
                });
            if (Column == TEXT("Name"))
            {
                return SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().AutoWidth()[SNew(SExpanderArrow, SharedThis(this))]
                    + SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)[Text];
            }
            return Text;
        }

    private:
        TSharedPtr<FKataGASInspectionRow> Data;
    };
}

void SKataGASInspector::Construct(const FArguments& Args)
{
    CanCollect = Args._CanCollect;
    Session = MakeUnique<FKataGASInspectionSession>();
    TriggerIndex = MakeUnique<FKataGASAbilityTriggerIndex>();
    LoadSettings();
    ChildSlot
    [
        SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight().Padding(4.0f)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
            [
                SNew(SCheckBox)
                .IsChecked_Lambda([this] { return Session->IsAutomaticWorld() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
                .OnCheckStateChanged_Lambda([this](ECheckBoxState State) { Session->SetAutomaticWorld(State == ECheckBoxState::Checked); })
                [SNew(STextBlock).Text(FText::FromString(TEXT("Auto world")))]
            ]
            + SHorizontalBox::Slot().AutoWidth().Padding(6.0f, 0.0f)
            [
                SNew(SComboButton).OnGetMenuContent(this, &SKataGASInspector::WorldMenu)
                .ButtonContent()[SNew(STextBlock).Text(this, &SKataGASInspector::WorldLabel)]
            ]
            + SHorizontalBox::Slot().FillWidth(1.0f)
            [
                SNew(SComboButton).OnGetMenuContent(this, &SKataGASInspector::TargetMenu)
                .ButtonContent()[SNew(STextBlock).Text(this, &SKataGASInspector::TargetLabel)]
            ]
            + SHorizontalBox::Slot().AutoWidth().Padding(6.0f, 0.0f)
            [SNew(SButton).Text(FText::FromString(TEXT("Use selection"))).OnClicked(this, &SKataGASInspector::FollowEditorSelection)]
        ]
        + SVerticalBox::Slot().AutoHeight().Padding(4.0f)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth()
            [SNew(SComboButton).OnGetMenuContent(this, &SKataGASInspector::PageMenu)
                .ButtonContent()[SNew(STextBlock).Text_Lambda([this] { return FText::FromString(KataGASInspectorUI::PageName(Page)); })]]
            + SHorizontalBox::Slot().AutoWidth().Padding(6.0f, 0.0f)
            [SNew(SButton).Text(FText::FromString(TEXT("Refresh"))).OnClicked(this, &SKataGASInspector::Refresh)]
            + SHorizontalBox::Slot().AutoWidth()
            [SNew(SButton).Text_Lambda([this] { return FText::FromString(Session->IsFrozen() ? TEXT("Resume") : TEXT("Freeze")); })
                .OnClicked(this, &SKataGASInspector::ToggleFrozen)]
            + SHorizontalBox::Slot().AutoWidth().Padding(6.0f, 0.0f).VAlign(VAlign_Center)
            [SNew(STextBlock).Text(FText::FromString(TEXT("Interval (s)")))]
            + SHorizontalBox::Slot().AutoWidth()
            [SNew(SBox).WidthOverride(75.0f)
                [SNew(SSpinBox<float>).MinValue(0.1f).MaxValue(1.0f).Delta(0.1f)
                    .Value_Lambda([this] { return Session->GetInterval(); })
                    .OnValueChanged_Lambda([this](float Value) { Session->SetInterval(Value); SaveSettings(); })]]
            + SHorizontalBox::Slot().FillWidth(1.0f).Padding(6.0f, 0.0f)
            [SNew(SSearchBox).InitialText(FText::FromString(Search)).HintText(FText::FromString(TEXT("Filter names, states, tags and sources")))
                .OnTextChanged_Lambda([this](const FText& Text) { Search = Text.ToString(); RebuildRows(); })]
        ]
        + SVerticalBox::Slot().AutoHeight().Padding(4.0f)
        [
            SNew(SHorizontalBox)
            .Visibility_Lambda([this] { return Page == EKataGASInspectionPage::Abilities ? EVisibility::Visible : EVisibility::Collapsed; })
            + SHorizontalBox::Slot().AutoWidth()
            [SNew(SCheckBox).IsChecked_Lambda([this] { return bActiveOnly ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
                .OnCheckStateChanged_Lambda([this](ECheckBoxState State) { bActiveOnly = State == ECheckBoxState::Checked; RebuildRows(); SaveSettings(); })
                [SNew(STextBlock).Text(FText::FromString(TEXT("Active only")))]]
            + SHorizontalBox::Slot().AutoWidth().Padding(12.0f, 0.0f)
            [SNew(SCheckBox).IsChecked_Lambda([this] { return bBlockedOnly ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
                .OnCheckStateChanged_Lambda([this](ECheckBoxState State) { bBlockedOnly = State == ECheckBoxState::Checked; RebuildRows(); SaveSettings(); })
                [SNew(STextBlock).Text(FText::FromString(TEXT("Observed block only")))]]
        ]
        + SVerticalBox::Slot().AutoHeight().Padding(4.0f)
        [
            SNew(SVerticalBox)
            .Visibility_Lambda([this] { return Page == EKataGASInspectionPage::Triggers ? EVisibility::Visible : EVisibility::Collapsed; })
            + SVerticalBox::Slot().AutoHeight()
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().FillWidth(1.0f)
                [SNew(SEditableTextBox).Text(FText::FromString(TriggerTags))
                    .HintText(FText::FromString(TEXT("Exact trigger tags, comma separated; empty = all")))
                    .OnTextChanged_Lambda([this](const FText& Text)
                    {
                        TriggerTags = Text.ToString();
                        TriggerIndex->SetQuery(TriggerTags, bIncludeChildTags);
                        RebuildRows();
                    })]
                + SHorizontalBox::Slot().AutoWidth().Padding(6.0f, 0.0f)
                [SNew(SCheckBox).IsChecked_Lambda([this] { return bIncludeChildTags ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
                    .OnCheckStateChanged_Lambda([this](ECheckBoxState State)
                    {
                        bIncludeChildTags = State == ECheckBoxState::Checked;
                        TriggerIndex->SetQuery(TriggerTags, bIncludeChildTags);
                        RebuildRows();
                        SaveSettings();
                    })[SNew(STextBlock).Text(FText::FromString(TEXT("Include child tags")))]]
                + SHorizontalBox::Slot().AutoWidth()
                [SNew(SBox).WidthOverride(150.0f)
                    [SNew(SEditableTextBox).Text(FText::FromString(ContentRoot))
                        .HintText(FText::FromString(TEXT("Content root; empty = all")))
                        .OnTextChanged_Lambda([this](const FText& Text) { ContentRoot = Text.ToString(); })]]
                + SHorizontalBox::Slot().AutoWidth().Padding(6.0f, 0.0f)
                [SNew(SButton).Text(FText::FromString(TEXT("Scan"))).OnClicked_Lambda([this]
                    {
                        TriggerIndex->Start(ContentRoot);
                        SaveSettings();
                        RebuildRows();
                        return FReply::Handled();
                    })]
                + SHorizontalBox::Slot().AutoWidth()
                [SNew(SButton).Text(FText::FromString(TEXT("Cancel"))).OnClicked_Lambda([this]
                    {
                        TriggerIndex->Cancel();
                        return FReply::Handled();
                    })]
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f)
            [SNew(STextBlock).Text(this, &SKataGASInspector::ScanStatus).AutoWrapText(true)]
        ]
        + SVerticalBox::Slot().AutoHeight().Padding(4.0f)
        [SNew(STextBlock).Text(this, &SKataGASInspector::Summary).AutoWrapText(true)]
        + SVerticalBox::Slot().FillHeight(1.0f).Padding(4.0f)
        [
            SNew(SSplitter).Orientation(Orient_Vertical)
            + SSplitter::Slot().Value(0.75f)
            [
                SAssignNew(Tree, STreeView<TSharedPtr<FKataGASInspectionRow>>)
                .TreeItemsSource(&VisibleRows)
                .SelectionMode(ESelectionMode::Single)
                .OnGenerateRow(this, &SKataGASInspector::GenerateRow)
                .OnGetChildren(this, &SKataGASInspector::GetRowChildren)
                .OnSelectionChanged(this, &SKataGASInspector::SelectRow)
                .OnMouseButtonDoubleClick(this, &SKataGASInspector::OpenRowAsset)
                .HeaderRow
                (
                    SAssignNew(Header, SHeaderRow).CanSelectGeneratedColumn(true).HiddenColumnsList(HiddenColumns)
                    .OnHiddenColumnsListChanged_Lambda([this] { SaveSettings(); })
                    + SHeaderRow::Column(TEXT("Name")).DefaultLabel(FText::FromString(TEXT("Name"))).FillWidth(0.22f)
                        .SortMode(this, &SKataGASInspector::ColumnSort, FName(TEXT("Name"))).OnSort(this, &SKataGASInspector::SortChanged)
                    + SHeaderRow::Column(TEXT("State")).DefaultLabel(FText::FromString(TEXT("State / Kind"))).FillWidth(0.15f)
                        .SortMode(this, &SKataGASInspector::ColumnSort, FName(TEXT("State"))).OnSort(this, &SKataGASInspector::SortChanged)
                    + SHeaderRow::Column(TEXT("Value")).DefaultLabel(FText::FromString(TEXT("Value / Timing"))).FillWidth(0.28f)
                        .SortMode(this, &SKataGASInspector::ColumnSort, FName(TEXT("Value"))).OnSort(this, &SKataGASInspector::SortChanged)
                    + SHeaderRow::Column(TEXT("Tags")).DefaultLabel(FText::FromString(TEXT("Tags"))).FillWidth(0.15f)
                        .SortMode(this, &SKataGASInspector::ColumnSort, FName(TEXT("Tags"))).OnSort(this, &SKataGASInspector::SortChanged)
                    + SHeaderRow::Column(TEXT("Source")).DefaultLabel(FText::FromString(TEXT("Source / Class"))).FillWidth(0.20f)
                        .SortMode(this, &SKataGASInspector::ColumnSort, FName(TEXT("Source"))).OnSort(this, &SKataGASInspector::SortChanged)
                )
            ]
            + SSplitter::Slot().Value(0.25f)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight()
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().AutoWidth()
                    [SNew(SButton).Text(FText::FromString(TEXT("Open asset"))).OnClicked(this, &SKataGASInspector::OpenSelectedAsset)
                        .IsEnabled_Lambda([this] { return SelectedRow.IsValid() && !SelectedRow->AssetPath.IsNull(); })]
                    + SHorizontalBox::Slot().AutoWidth().Padding(6.0f, 0.0f)
                    [SNew(SButton).Text(FText::FromString(TEXT("Copy name"))).OnClicked(this, &SKataGASInspector::CopySelected)
                        .IsEnabled_Lambda([this] { return SelectedRow.IsValid(); })]
                    + SHorizontalBox::Slot().AutoWidth()
                    [SNew(SButton).Text(FText::FromString(TEXT("Expand all"))).OnClicked_Lambda([this]
                        {
                            TFunction<void(const TArray<TSharedPtr<FKataGASInspectionRow>>&)> Expand;
                            Expand = [this, &Expand](const auto& Rows)
                            {
                                for (const auto& Row : Rows) { Tree->SetItemExpansion(Row, true); Expand(Row->VisibleChildren); }
                            };
                            Expand(VisibleRows);
                            return FReply::Handled();
                        })]
                    + SHorizontalBox::Slot().AutoWidth().Padding(6.0f, 0.0f)
                    [SNew(SButton).Text(FText::FromString(TEXT("Collapse all"))).OnClicked_Lambda([this]
                        {
                            Tree->ClearExpandedItems();
                            return FReply::Handled();
                        })]
                ]
                + SVerticalBox::Slot().FillHeight(1.0f).Padding(0.0f, 4.0f)
                [SNew(SScrollBox) + SScrollBox::Slot()[SNew(STextBlock).Text(this, &SKataGASInspector::Detail).AutoWrapText(true)]]
            ]
        ]
    ];
    TriggerIndex->SetQuery(TriggerTags, bIncludeChildTags);
    Session->Tick(0.0);
    RebuildRows();
}

SKataGASInspector::~SKataGASInspector()
{
    Stop();
}

void SKataGASInspector::Stop()
{
    if (!bStopped)
    {
        SaveSettings();
        TriggerIndex.Reset();
        bStopped = true;
    }
}

void SKataGASInspector::Tick(const FGeometry& Geometry, double CurrentTime, float DeltaTime)
{
    SCompoundWidget::Tick(Geometry, CurrentTime, DeltaTime);
    if (bStopped || !CanCollect.Get(true))
    {
        return;
    }
    if (Page != EKataGASInspectionPage::Triggers)
    {
        Session->Tick(CurrentTime);
    }
    else
    {
        TriggerIndex->Tick();
    }
    if (LastRevision != Session->GetRevision() || LastTriggerRevision != TriggerIndex->GetRevision())
    {
        RebuildRows();
    }
}

TSharedRef<SWidget> SKataGASInspector::WorldMenu()
{
    FMenuBuilder Menu(true, nullptr);
    for (const auto& World : Session->GetWorlds())
    {
        Menu.AddMenuEntry(FText::FromString(World->Label), FText::GetEmpty(), FSlateIcon(),
            FUIAction(FExecuteAction::CreateSPLambda(this, [this, Context = World->Context]
            {
                Session->SelectWorld(Context);
                RebuildRows();
            })));
    }
    return Menu.MakeWidget();
}

TSharedRef<SWidget> SKataGASInspector::TargetMenu()
{
    Session->RescanTargets();
    FilterTargets();
    return SNew(SBox).WidthOverride(650.0f).HeightOverride(350.0f)
    [
        SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight()
        [SNew(SSearchBox).InitialText(FText::FromString(TargetSearch)).HintText(FText::FromString(TEXT("Find actor, ASC or class")))
            .OnTextChanged_Lambda([this](const FText& Text) { TargetSearch = Text.ToString(); FilterTargets(); })]
        + SVerticalBox::Slot().FillHeight(1.0f)
        [
            SAssignNew(TargetList, SListView<TSharedPtr<FKataGASInspectionTarget>>).ListItemsSource(&TargetOptions)
            .OnGenerateRow_Lambda([](TSharedPtr<FKataGASInspectionTarget> Item, const TSharedRef<STableViewBase>& Table)
            {
                return SNew(STableRow<TSharedPtr<FKataGASInspectionTarget>>, Table)
                    [SNew(STextBlock).Text(FText::FromString(Item->Label))
                        .ToolTipText(FText::FromString(Item->ASC.IsValid() ? Item->ASC->GetPathName() : TEXT("Expired")))];
            })
            .OnSelectionChanged_Lambda([this](TSharedPtr<FKataGASInspectionTarget> Item, ESelectInfo::Type Type)
            {
                if (Item.IsValid())
                {
                    Session->SelectTarget(Item->ASC.Get());
                    RebuildRows();
                    FSlateApplication::Get().DismissAllMenus();
                }
            })
        ]
    ];
}

TSharedRef<SWidget> SKataGASInspector::PageMenu()
{
    FMenuBuilder Menu(true, nullptr);
    for (uint8 Index = 0; Index <= static_cast<uint8>(EKataGASInspectionPage::Triggers); ++Index)
    {
        const EKataGASInspectionPage Choice = static_cast<EKataGASInspectionPage>(Index);
        Menu.AddMenuEntry(FText::FromString(KataGASInspectorUI::PageName(Choice)), FText::GetEmpty(), FSlateIcon(),
            FUIAction(FExecuteAction::CreateSPLambda(this, [this, Choice]
            {
                Page = Choice;
                NavigationStatus.Reset();
                RebuildRows();
                SaveSettings();
            })));
    }
    return Menu.MakeWidget();
}

void SKataGASInspector::FilterTargets()
{
    TargetOptions.Reset();
    for (const auto& Target : Session->GetTargets())
    {
        if (TargetSearch.IsEmpty() || Target->Label.Contains(TargetSearch))
        {
            TargetOptions.Add(Target);
        }
    }
    if (TargetList.IsValid()) { TargetList->RequestListRefresh(); }
}

bool SKataGASInspector::FilterRow(const TSharedPtr<FKataGASInspectionRow>& Row, bool bParentMatches)
{
    const bool bMatches = bParentMatches || Search.IsEmpty() || Row->GetSearchText().Contains(Search);
    Row->VisibleChildren.Reset();
    for (const auto& Child : Row->Children)
    {
        if (FilterRow(Child, bMatches)) { Row->VisibleChildren.Add(Child); }
    }
    SortRows(Row->VisibleChildren);
    return bMatches || !Row->VisibleChildren.IsEmpty();
}

void SKataGASInspector::SortRows(TArray<TSharedPtr<FKataGASInspectionRow>>& Rows)
{
    Rows.Sort([this](const auto& A, const auto& B)
    {
        const int32 Compare = KataGASInspectorUI::Cell(*A, SortColumn).Compare(KataGASInspectorUI::Cell(*B, SortColumn));
        if (Compare == 0) { return A->Key < B->Key; }
        return SortMode == EColumnSortMode::Descending ? Compare > 0 : Compare < 0;
    });
}

void SKataGASInspector::RebuildRows()
{
    if (bStopped || !TriggerIndex)
    {
        return;
    }
    VisibleRows.Reset();
    const auto& Rows = Page == EKataGASInspectionPage::Triggers ? TriggerIndex->GetRows() : Session->GetSnapshot().Rows;
    for (const auto& Row : Rows)
    {
        if (Row->Page != Page || (Page == EKataGASInspectionPage::Abilities
            && ((bActiveOnly && !Row->bActive) || (bBlockedOnly && !Row->bBlocked))))
        {
            continue;
        }
        if (FilterRow(Row, false)) { VisibleRows.Add(Row); }
    }
    SortRows(VisibleRows);
    if (SelectedRow.IsValid())
    {
        bool bStillVisible = false;
        TFunction<void(const TArray<TSharedPtr<FKataGASInspectionRow>>&)> Find;
        Find = [&Find, &bStillVisible, this](const auto& Items)
        {
            for (const auto& Item : Items)
            {
                bStillVisible |= Item == SelectedRow;
                Find(Item->VisibleChildren);
            }
        };
        Find(VisibleRows);
        if (!bStillVisible)
        {
            SelectedRow.Reset();
            if (Tree.IsValid()) { Tree->ClearSelection(); }
        }
    }
    if (Tree.IsValid()) { Tree->RequestTreeRefresh(); }
    LastRevision = Session->GetRevision();
    LastTriggerRevision = TriggerIndex->GetRevision();
}

TSharedRef<ITableRow> SKataGASInspector::GenerateRow(TSharedPtr<FKataGASInspectionRow> Row, const TSharedRef<STableViewBase>& Table)
{
    return SNew(KataGASInspectorUI::SKataGASInspectorRow, Table).Data(Row);
}

void SKataGASInspector::GetRowChildren(TSharedPtr<FKataGASInspectionRow> Row, TArray<TSharedPtr<FKataGASInspectionRow>>& Children) const
{
    Children = Row->VisibleChildren;
}

void SKataGASInspector::SelectRow(TSharedPtr<FKataGASInspectionRow> Row, ESelectInfo::Type Type)
{
    SelectedRow = Row;
    NavigationStatus.Reset();
}

void SKataGASInspector::SortChanged(EColumnSortPriority::Type Priority, const FName& Column, EColumnSortMode::Type Mode)
{
    SortColumn = Column;
    SortMode = Mode;
    RebuildRows();
    SaveSettings();
}

EColumnSortMode::Type SKataGASInspector::ColumnSort(FName Column) const
{
    return SortColumn == Column ? SortMode : EColumnSortMode::None;
}

FReply SKataGASInspector::Refresh()
{
    Session->RescanTargets();
    Session->Refresh();
    RebuildRows();
    return FReply::Handled();
}

FReply SKataGASInspector::ToggleFrozen()
{
    Session->SetFrozen(!Session->IsFrozen());
    if (!Session->IsFrozen()) { Session->Refresh(); RebuildRows(); }
    return FReply::Handled();
}

FReply SKataGASInspector::FollowEditorSelection()
{
    Session->RescanTargets();
    AActor* Selected = GEditor ? GEditor->GetSelectedActors()->GetTop<AActor>() : nullptr;
    for (const auto& Target : Session->GetTargets())
    {
        UAbilitySystemComponent* ASC = Target->ASC.Get();
        if (ASC && Selected && (ASC->GetOwner() == Selected || ASC->GetOwnerActor() == Selected || ASC->GetAvatarActor_Direct() == Selected))
        {
            Session->SelectTarget(ASC);
            RebuildRows();
            NavigationStatus.Reset();
            return FReply::Handled();
        }
    }
    NavigationStatus = TEXT("The selected actor has no ASC in the selected world.");
    return FReply::Handled();
}

void SKataGASInspector::OpenRowAsset(TSharedPtr<FKataGASInspectionRow> Row)
{
    if (Row.IsValid() && !Row->AssetPath.IsNull() && GEditor)
    {
        UObject* Asset = Row->AssetPath.TryLoad();
        NavigationStatus = IsValid(Asset) ? FString() : TEXT("Source asset unavailable or deleted.");
        if (IsValid(Asset)) { GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Asset); }
    }
}

FReply SKataGASInspector::OpenSelectedAsset()
{
    OpenRowAsset(SelectedRow);
    return FReply::Handled();
}

FReply SKataGASInspector::CopySelected()
{
    if (SelectedRow.IsValid()) { FPlatformApplicationMisc::ClipboardCopy(*SelectedRow->Name); }
    return FReply::Handled();
}

FText SKataGASInspector::WorldLabel() const
{
    for (const auto& World : Session->GetWorlds())
    {
        if (World->Context == Session->GetSelectedWorld()) { return FText::FromString(World->Label); }
    }
    return FText::FromString(TEXT("Select world"));
}

FText SKataGASInspector::TargetLabel() const
{
    for (const auto& Target : Session->GetTargets())
    {
        if (Target->ASC.Get() == Session->GetSelectedTarget()) { return FText::FromString(Target->Label); }
    }
    return FText::FromString(TEXT("Select ASC"));
}

FText SKataGASInspector::Summary() const
{
    if (Page == EKataGASInspectionPage::Triggers)
    {
        return FText::FromString(FString::Printf(TEXT("%d results | Trigger settings; not event history."), VisibleRows.Num()));
    }
    const FKataGASInspectionSnapshot& Snapshot = Session->GetSnapshot();
    if (Snapshot.CapturedAt.GetTicks() == 0) { return FText::FromString(Session->GetStatus()); }
    return FText::FromString(FString::Printf(TEXT("%s | %s | Captured: %s | %d rows\n%s\n%s"),
        Session->IsFrozen() ? TEXT("Frozen snapshot") : TEXT("Live"), *Snapshot.World,
        *Snapshot.CapturedAt.ToString(TEXT("%H:%M:%S")), VisibleRows.Num(), *Snapshot.Target, *Session->GetStatus()));
}

FText SKataGASInspector::Detail() const
{
    if (!SelectedRow.IsValid()) { return FText::FromString(NavigationStatus.IsEmpty() ? TEXT("Select a row to inspect details.") : NavigationStatus); }
    return FText::FromString(SelectedRow->Name + TEXT("\n") + SelectedRow->State + TEXT("\n") + SelectedRow->Value
        + TEXT("\n") + SelectedRow->Tags + TEXT("\n") + SelectedRow->Source + TEXT("\n") + SelectedRow->Detail
        + TEXT("\n") + NavigationStatus);
}

FText SKataGASInspector::ScanStatus() const
{
    return FText::FromString(TriggerIndex ? TriggerIndex->GetStatus() : TEXT("Inspector closed"));
}

void SKataGASInspector::SaveSettings()
{
    if (!GConfig || !Session) { return; }
    const TCHAR* Section = TEXT("KataGASInspector");
    GConfig->SetFloat(Section, TEXT("Interval"), Session->GetInterval(), GEditorPerProjectIni);
    GConfig->SetInt(Section, TEXT("Page"), static_cast<int32>(Page), GEditorPerProjectIni);
    GConfig->SetString(Section, TEXT("SortColumn"), *SortColumn.ToString(), GEditorPerProjectIni);
    GConfig->SetBool(Section, TEXT("Descending"), SortMode == EColumnSortMode::Descending, GEditorPerProjectIni);
    GConfig->SetBool(Section, TEXT("ActiveOnly"), bActiveOnly, GEditorPerProjectIni);
    GConfig->SetBool(Section, TEXT("BlockedOnly"), bBlockedOnly, GEditorPerProjectIni);
    GConfig->SetBool(Section, TEXT("IncludeChildTags"), bIncludeChildTags, GEditorPerProjectIni);
    GConfig->SetString(Section, TEXT("ContentRoot"), *ContentRoot, GEditorPerProjectIni);
    if (Header.IsValid()) { HiddenColumns = Header->GetHiddenColumnIds(); }
    TArray<FString> Columns;
    for (FName Column : HiddenColumns) { Columns.Add(Column.ToString()); }
    GConfig->SetArray(Section, TEXT("HiddenColumns"), Columns, GEditorPerProjectIni);
}

void SKataGASInspector::LoadSettings()
{
    if (!GConfig) { return; }
    const TCHAR* Section = TEXT("KataGASInspector");
    float Interval = 0.2f;
    GConfig->GetFloat(Section, TEXT("Interval"), Interval, GEditorPerProjectIni);
    Session->SetInterval(Interval);
    int32 PageIndex = 0;
    GConfig->GetInt(Section, TEXT("Page"), PageIndex, GEditorPerProjectIni);
    Page = static_cast<EKataGASInspectionPage>(FMath::Clamp(PageIndex, 0, 4));
    FString Column;
    if (GConfig->GetString(Section, TEXT("SortColumn"), Column, GEditorPerProjectIni)) { SortColumn = FName(*Column); }
    bool bDescending = false;
    GConfig->GetBool(Section, TEXT("Descending"), bDescending, GEditorPerProjectIni);
    SortMode = bDescending ? EColumnSortMode::Descending : EColumnSortMode::Ascending;
    GConfig->GetBool(Section, TEXT("ActiveOnly"), bActiveOnly, GEditorPerProjectIni);
    GConfig->GetBool(Section, TEXT("BlockedOnly"), bBlockedOnly, GEditorPerProjectIni);
    GConfig->GetBool(Section, TEXT("IncludeChildTags"), bIncludeChildTags, GEditorPerProjectIni);
    GConfig->GetString(Section, TEXT("ContentRoot"), ContentRoot, GEditorPerProjectIni);
    TArray<FString> Columns;
    GConfig->GetArray(Section, TEXT("HiddenColumns"), Columns, GEditorPerProjectIni);
    for (const FString& Hidden : Columns) { HiddenColumns.Add(FName(*Hidden)); }
}
