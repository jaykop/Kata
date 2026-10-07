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
        case EKataGASInspectionPage::Effects: return TEXT("Effects");
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
            if (Column == TEXT("Name") && (Data->Page == EKataGASInspectionPage::Abilities || Data->Page == EKataGASInspectionPage::Effects))
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
                .ButtonContent()[SNew(STextBlock).Text(this, &SKataGASInspector::TargetLabel).ToolTipText(this, &SKataGASInspector::SnapshotTooltip)]
            ]
            + SHorizontalBox::Slot().AutoWidth().Padding(6.0f, 0.0f)
            [SNew(SButton).Text(FText::FromString(TEXT("Use selection"))).OnClicked(this, &SKataGASInspector::FollowEditorSelection)]
        ]
        + SVerticalBox::Slot().AutoHeight().Padding(4.0f)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth()
            [
                SNew(SBox).WidthOverride(140.0f)
                [
                    SNew(SComboButton).OnGetMenuContent(this, &SKataGASInspector::PageMenu)
                    .ButtonContent()
                    [
                        SNew(STextBlock).Text_Lambda([this] { return FText::FromString(KataGASInspectorUI::PageName(Page)); })
                    ]
                ]
            ]
            + SHorizontalBox::Slot().AutoWidth().Padding(6.0f, 0.0f)
            [SNew(SButton).Text(FText::FromString(TEXT("Refresh"))).OnClicked(this, &SKataGASInspector::Refresh)]
            + SHorizontalBox::Slot().AutoWidth()
            [SNew(SButton).Text_Lambda([this] { return FText::FromString(Session->IsFrozen() ? TEXT("Resume") : TEXT("Freeze")); })
                .OnClicked(this, &SKataGASInspector::ToggleFrozen)]
            + SHorizontalBox::Slot().AutoWidth().Padding(8.0f, 0.0f).VAlign(VAlign_Center)
            [SNew(STextBlock).Text(FText::FromString(TEXT("Interval (s)")))]
            + SHorizontalBox::Slot().AutoWidth()
            [
                SNew(SBox).WidthOverride(75.0f)
                [
                    SNew(SSpinBox<float>).MinValue(0.1f).MaxValue(1.0f).Delta(0.1f)
                    .Value_Lambda([this] { return Session->GetInterval(); })
                    .OnValueChanged_Lambda([this](float Value) { Session->SetInterval(Value); SaveSettings(); })
                ]
            ]
        ]
        + SVerticalBox::Slot().AutoHeight().Padding(4.0f)
        [SNew(STextBlock).Text(this, &SKataGASInspector::Summary)
            .ToolTipText(this, &SKataGASInspector::SnapshotTooltip).AutoWrapText(true)]
        + SVerticalBox::Slot().FillHeight(1.0f)
        [SAssignNew(PageHost, SBox)[BuildPage()]]
    ];
    Session->Tick(0.0);
    RebuildRows();
}

TSharedRef<SWidget> SKataGASInspector::BuildPage()
{
    Header = SNew(SHeaderRow);
    const uint8 PageIndex = static_cast<uint8>(Page);
    // 너비 변경 콜백을 연결하면 엔진은 열 값을 직접 바꾸지 않으므로, 화면 상태를 유일한 값으로 사용한다.
    const auto AddColumn = [this, PageIndex](FName Id, const TCHAR* Label, float Width)
    {
        Header->AddColumn(SHeaderRow::Column(Id).DefaultLabel(FText::FromString(Label))
            .FillWidth_Lambda([this, PageIndex, Id, Width]
            {
                const float* Saved = PageStates[PageIndex].ColumnWidths.Find(Id);
                return Saved ? *Saved : Width;
            })
            .OnWidthChanged_Lambda([this, PageIndex, Id](float NewWidth) { PageStates[PageIndex].ColumnWidths.Add(Id, NewWidth); })
            .SortMode(this, &SKataGASInspector::ColumnSort, Id).OnSort(this, &SKataGASInspector::SortChanged));
    };
    switch (Page)
    {
    case EKataGASInspectionPage::Tags:
        AddColumn(TEXT("Name"), TEXT("Tag"), 0.65f);
        AddColumn(TEXT("State"), TEXT("Kind"), 0.20f);
        AddColumn(TEXT("Value"), TEXT("Count"), 0.15f);
        break;
    case EKataGASInspectionPage::Attributes:
        AddColumn(TEXT("Name"), TEXT("Attribute"), 0.70f);
        AddColumn(TEXT("Value"), TEXT("Current"), 0.30f);
        break;
    case EKataGASInspectionPage::Abilities:
        AddColumn(TEXT("Name"), TEXT("Ability"), 0.35f);
        AddColumn(TEXT("State"), TEXT("State"), 0.25f);
        AddColumn(TEXT("Value"), TEXT("Execution"), 0.40f);
        break;
    case EKataGASInspectionPage::Effects:
        AddColumn(TEXT("Name"), TEXT("Effect"), 0.35f);
        AddColumn(TEXT("State"), TEXT("State"), 0.20f);
        AddColumn(TEXT("Value"), TEXT("Timing / Stacks"), 0.45f);
        break;
    }
    TSharedRef<SWidget> MainTable = SAssignNew(Tree, STreeView<TSharedPtr<FKataGASInspectionRow>>)
        .TreeItemsSource(&VisibleRows).SelectionMode(ESelectionMode::Single)
        .OnGenerateRow(this, &SKataGASInspector::GenerateRow)
        .OnGetChildren(this, &SKataGASInspector::GetRowChildren)
        .OnSelectionChanged(this, &SKataGASInspector::SelectRow)
        .OnMouseButtonDoubleClick(this, &SKataGASInspector::OpenRowAsset)
        .HeaderRow(Header.ToSharedRef());
    // 화면별로 필요한 열을 구성하고 공통 세션을 유지한다.
    return SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight().Padding(4.0f)
        [SNew(SSearchBox).InitialText(FText::FromString(Search)).HintText(FText::FromString(TEXT("Search this tab")))
            .OnTextChanged_Lambda([this](const FText& Text) { Search = Text.ToString(); RebuildRows(); })]
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
        + SVerticalBox::Slot().FillHeight(1.0f).Padding(4.0f)
        [
            SNew(SSplitter).Orientation(Orient_Vertical)
            // 크기 변경 콜백을 연결한 슬롯은 엔진이 값을 직접 바꾸지 않으므로 화면 상태에서 읽고 쓴다.
            + SSplitter::Slot()
            .Value_Lambda([this, PageIndex] { return PageStates[PageIndex].TableRatio; })
            .OnSlotResized_Lambda([this, PageIndex](float Ratio) { PageStates[PageIndex].TableRatio = Ratio; })
            [
                MainTable
            ]
            + SSplitter::Slot()
            .Value_Lambda([this, PageIndex] { return PageStates[PageIndex].DetailRatio; })
            .OnSlotResized_Lambda([this, PageIndex](float Ratio) { PageStates[PageIndex].DetailRatio = Ratio; })
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight()
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().AutoWidth()
                    [SNew(SButton).Visibility_Lambda([this] { return IsHierarchy() ? EVisibility::Visible : EVisibility::Collapsed; }).Text(FText::FromString(TEXT("Open asset"))).OnClicked(this, &SKataGASInspector::OpenSelectedAsset)
                        .IsEnabled_Lambda([this] { return SelectedRow.IsValid() && !SelectedRow->AssetPath.IsNull(); })]
                    + SHorizontalBox::Slot().AutoWidth().Padding(6.0f, 0.0f)
                    [SNew(SButton).Text_Lambda([this] { return FText::FromString(SelectedRow.IsValid() && SelectedRow->Page == EKataGASInspectionPage::Tags ? TEXT("Copy tag") : TEXT("Copy name")); }).OnClicked(this, &SKataGASInspector::CopySelected)
                        .IsEnabled_Lambda([this] { return SelectedRow.IsValid(); })]
                    + SHorizontalBox::Slot().AutoWidth()
                    [SNew(SButton).Visibility_Lambda([this] { return IsHierarchy() ? EVisibility::Visible : EVisibility::Collapsed; }).Text(FText::FromString(TEXT("Expand all"))).OnClicked_Lambda([this]
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
                    [SNew(SButton).Visibility_Lambda([this] { return IsHierarchy() ? EVisibility::Visible : EVisibility::Collapsed; }).Text(FText::FromString(TEXT("Collapse all"))).OnClicked_Lambda([this]
                        {
                            Tree->ClearExpandedItems();
                            return FReply::Handled();
                        })]
                ]
                + SVerticalBox::Slot().FillHeight(1.0f).Padding(0.0f, 4.0f)
                [SNew(SScrollBox) + SScrollBox::Slot()[SNew(STextBlock).Text(this, &SKataGASInspector::Detail).AutoWrapText(true)]]
            ]
        ]
    ;
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
    Session->Tick(CurrentTime);
    if (LastRevision != Session->GetRevision()) { RebuildRows(); }
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
    // ComboBox 배치는 팝업 폭을 드롭다운 폭 이상으로 맞추므로 폭은 고정하지 않고, 높이는 항목 수에 맞추되 상한만 둔다.
    return SNew(SBox).MaxDesiredHeight(300.0f)
    [
        SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight()
        [SNew(SSearchBox).InitialText(FText::FromString(TargetSearch)).HintText(FText::FromString(TEXT("Find actor")))
            .OnTextChanged_Lambda([this](const FText& Text) { TargetSearch = Text.ToString(); FilterTargets(); })]
        // FillHeight 슬롯도 원하는 높이에는 목록 높이가 반영되며, 상한을 넘으면 남은 공간에서 목록이 스크롤된다.
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
    for (const EKataGASInspectionPage Choice : { EKataGASInspectionPage::Tags, EKataGASInspectionPage::Attributes, EKataGASInspectionPage::Abilities, EKataGASInspectionPage::Effects })
    {
        Menu.AddMenuEntry(FText::FromString(KataGASInspectorUI::PageName(Choice)), FText::GetEmpty(), FSlateIcon(),
            FUIAction(FExecuteAction::CreateSPLambda(this, [this, Choice] { ChangePage(Choice); })));
    }
    return Menu.MakeWidget();
}

bool SKataGASInspector::IsHierarchy() const
{
    return Page == EKataGASInspectionPage::Abilities || Page == EKataGASInspectionPage::Effects;
}

void SKataGASInspector::RememberPage()
{
    // UObject나 이전 화면 위젯 대신 행의 안정적인 값 키만 보관한다.
    FPageState& State = PageStates[static_cast<uint8>(Page)];
    State.Search = Search;
    State.SortColumn = SortColumn;
    State.SortMode = SortMode;
    State.SelectionKey = SelectedRow.IsValid() ? SelectedRow->Key : FString();
    State.ExpandedKeys.Reset();
    if (Tree.IsValid())
    {
        TFunction<void(const TArray<TSharedPtr<FKataGASInspectionRow>>&)> RememberExpanded;
        RememberExpanded = [this, &State, &RememberExpanded](const auto& Rows)
        {
            for (const auto& Row : Rows)
            {
                if (Tree->IsItemExpanded(Row)) { State.ExpandedKeys.Add(Row->Key); }
                RememberExpanded(Row->VisibleChildren);
            }
        };
        RememberExpanded(VisibleRows);
    }
}

void SKataGASInspector::ChangePage(EKataGASInspectionPage Choice)
{
    if (Choice == Page) { return; }
    RememberPage();
    Page = Choice;
    const FPageState& State = PageStates[static_cast<uint8>(Page)];
    Search = State.Search;
    SortColumn = State.SortColumn;
    SortMode = State.SortMode;
    SelectedRow.Reset();
    NavigationStatus.Reset();
    PageHost->SetContent(BuildPage());
    RebuildRows();
    TFunction<void(const TArray<TSharedPtr<FKataGASInspectionRow>>&)> Restore;
    Restore = [this, &State, &Restore](const auto& Rows)
    {
        for (const auto& Row : Rows)
        {
            if (State.ExpandedKeys.Contains(Row->Key)) { Tree->SetItemExpansion(Row, true); }
            if (Row->Key == State.SelectionKey) { Tree->SetSelection(Row); }
            Restore(Row->VisibleChildren);
        }
    };
    Restore(VisibleRows);
    SaveSettings();
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
    if (bStopped)
    {
        return;
    }
    VisibleRows.Reset();
    const auto& Rows = Session->GetSnapshot().Rows;
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
}

TSharedRef<ITableRow> SKataGASInspector::GenerateRow(TSharedPtr<FKataGASInspectionRow> Row, const TSharedRef<STableViewBase>& Table)
{
    return SNew(KataGASInspectorUI::SKataGASInspectorRow, Table).Data(Row);
}

void SKataGASInspector::GetRowChildren(TSharedPtr<FKataGASInspectionRow> Row, TArray<TSharedPtr<FKataGASInspectionRow>>& Children) const
{
    if (IsHierarchy()) { Children = Row->VisibleChildren; }
}

void SKataGASInspector::SelectRow(TSharedPtr<FKataGASInspectionRow> Row, ESelectInfo::Type Type)
{
    // Ctrl+클릭 해제도 반영해야 상세 영역과 버튼이 이전 행을 가리키지 않는다.
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
    const auto& Snapshot = Session->GetSnapshot();
    if (Snapshot.CapturedAt.GetTicks() == 0) { return FText::FromString(Session->GetStatus()); }
    FString Status = FString::Printf(TEXT("%s | %d rows"), Session->IsFrozen() ? TEXT("Frozen snapshot") : TEXT("Live"), VisibleRows.Num());
    if (!Session->GetSelectedTarget()) { Status += TEXT(" | Target unavailable"); }
    else if (Session->IsFrozen() && Session->GetSelectedTarget()->GetPathName() != Snapshot.TargetPath)
    { Status += TEXT(" | Previous target"); }
    if (!Snapshot.bActorInfoReady) { Status += TEXT(" | ActorInfo not initialized"); }
    return FText::FromString(Status);
}

FText SKataGASInspector::SnapshotTooltip() const
{
    const auto& Snapshot = Session->GetSnapshot();
    return FText::FromString(FString::Printf(TEXT("%s\nCaptured: %s\n%s\n%s"), *Snapshot.World,
        *Snapshot.CapturedAt.ToString(TEXT("%H:%M:%S")), *Snapshot.Target, *Session->GetStatus()));
}

FText SKataGASInspector::Detail() const
{
    if (!SelectedRow.IsValid()) { return FText::FromString(NavigationStatus); }
    TArray<FString> Lines;
    Lines.Add(SelectedRow->Name);
    if (SelectedRow->Page != EKataGASInspectionPage::Attributes && !SelectedRow->State.IsEmpty()) { Lines.Add(SelectedRow->State); }
    if (!SelectedRow->Value.IsEmpty()) { Lines.Add(SelectedRow->Value); }
    if (SelectedRow->Page != EKataGASInspectionPage::Tags && !SelectedRow->Tags.IsEmpty()) { Lines.Add(SelectedRow->Tags); }
    if (!SelectedRow->Source.IsEmpty()) { Lines.Add(SelectedRow->Source); }
    if (!SelectedRow->Detail.IsEmpty()) { Lines.Add(SelectedRow->Detail); }
    if (!NavigationStatus.IsEmpty()) { Lines.Add(NavigationStatus); }
    return FText::FromString(FString::Join(Lines, TEXT("\n")));
}

void SKataGASInspector::SaveSettings()
{
    if (!GConfig || !Session) { return; }
    RememberPage();
    const TCHAR* Section = TEXT("KataGASInspector");
    GConfig->SetFloat(Section, TEXT("Interval"), Session->GetInterval(), GEditorPerProjectIni);
    GConfig->SetInt(Section, TEXT("Page"), static_cast<int32>(Page), GEditorPerProjectIni);
    GConfig->SetBool(Section, TEXT("ActiveOnly"), bActiveOnly, GEditorPerProjectIni);
    GConfig->SetBool(Section, TEXT("BlockedOnly"), bBlockedOnly, GEditorPerProjectIni);
    for (int32 Index = 0; Index < 4; ++Index)
    {
        const FString TabSection = FString::Printf(TEXT("KataGASInspector.Tab%d"), Index);
        GConfig->SetString(*TabSection, TEXT("SortColumn"), *PageStates[Index].SortColumn.ToString(), GEditorPerProjectIni);
        GConfig->SetBool(*TabSection, TEXT("Descending"), PageStates[Index].SortMode == EColumnSortMode::Descending, GEditorPerProjectIni);
        GConfig->SetFloat(*TabSection, TEXT("TableRatio"), PageStates[Index].TableRatio, GEditorPerProjectIni);
        GConfig->SetFloat(*TabSection, TEXT("DetailRatio"), PageStates[Index].DetailRatio, GEditorPerProjectIni);
        TArray<FString> Widths;
        for (const TPair<FName, float>& Width : PageStates[Index].ColumnWidths)
        {
            Widths.Add(FString::Printf(TEXT("%s=%g"), *Width.Key.ToString(), Width.Value));
        }
        GConfig->SetString(*TabSection, TEXT("ColumnWidths"), *FString::Join(Widths, TEXT(";")), GEditorPerProjectIni);
    }
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
    Page = static_cast<EKataGASInspectionPage>(PageIndex >= 0 && PageIndex < 4 ? PageIndex : 0);
    GConfig->GetBool(Section, TEXT("ActiveOnly"), bActiveOnly, GEditorPerProjectIni);
    GConfig->GetBool(Section, TEXT("BlockedOnly"), bBlockedOnly, GEditorPerProjectIni);
    for (int32 Index = 0; Index < 4; ++Index)
    {
        const FString TabSection = FString::Printf(TEXT("KataGASInspector.Tab%d"), Index);
        FString Column;
        if (GConfig->GetString(*TabSection, TEXT("SortColumn"), Column, GEditorPerProjectIni)
            && (Column == TEXT("Name") || Column == TEXT("Value") || (Index != 1 && Column == TEXT("State"))))
        { PageStates[Index].SortColumn = FName(*Column); }
        bool bDescending = false;
        GConfig->GetBool(*TabSection, TEXT("Descending"), bDescending, GEditorPerProjectIni);
        PageStates[Index].SortMode = bDescending ? EColumnSortMode::Descending : EColumnSortMode::Ascending;
        // 손상되었거나 0에 가까운 비율은 슬롯을 사라지게 하므로 기본값을 유지한다.
        float TableRatio = 0.0f;
        float DetailRatio = 0.0f;
        if (GConfig->GetFloat(*TabSection, TEXT("TableRatio"), TableRatio, GEditorPerProjectIni)
            && GConfig->GetFloat(*TabSection, TEXT("DetailRatio"), DetailRatio, GEditorPerProjectIni)
            && TableRatio > 0.05f && DetailRatio > 0.05f)
        {
            PageStates[Index].TableRatio = TableRatio;
            PageStates[Index].DetailRatio = DetailRatio;
        }
        FString Widths;
        GConfig->GetString(*TabSection, TEXT("ColumnWidths"), Widths, GEditorPerProjectIni);
        TArray<FString> Entries;
        Widths.ParseIntoArray(Entries, TEXT(";"));
        for (const FString& Entry : Entries)
        {
            FString WidthColumn;
            FString WidthValue;
            if (Entry.Split(TEXT("="), &WidthColumn, &WidthValue) && FCString::Atof(*WidthValue) > 0.01f)
            {
                PageStates[Index].ColumnWidths.Add(FName(*WidthColumn), FCString::Atof(*WidthValue));
            }
        }
    }
    SortColumn = PageStates[static_cast<uint8>(Page)].SortColumn;
    SortMode = PageStates[static_cast<uint8>(Page)].SortMode;
}
