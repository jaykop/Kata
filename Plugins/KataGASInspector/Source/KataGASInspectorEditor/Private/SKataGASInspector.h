#pragma once

#include "CoreMinimal.h"
#include "KataGASAbilityTriggerIndex.h"
#include "KataGASInspectionSession.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STreeView.h"

class SHeaderRow;
template <typename ItemType> class SListView;

class SKataGASInspector : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SKataGASInspector) {}
        SLATE_ATTRIBUTE(bool, CanCollect)
    SLATE_END_ARGS()

    void Construct(const FArguments& Args);
    virtual ~SKataGASInspector() override;
    virtual void Tick(const FGeometry& Geometry, double CurrentTime, float DeltaTime) override;
    void Stop();

private:
    TSharedRef<SWidget> WorldMenu();
    TSharedRef<SWidget> TargetMenu();
    TSharedRef<SWidget> PageMenu();
    void FilterTargets();
    void RebuildRows();
    bool FilterRow(const TSharedPtr<FKataGASInspectionRow>& Row, bool bParentMatches);
    void SortRows(TArray<TSharedPtr<FKataGASInspectionRow>>& Rows);
    TSharedRef<ITableRow> GenerateRow(TSharedPtr<FKataGASInspectionRow> Row, const TSharedRef<STableViewBase>& Table);
    void GetRowChildren(TSharedPtr<FKataGASInspectionRow> Row, TArray<TSharedPtr<FKataGASInspectionRow>>& Children) const;
    void SelectRow(TSharedPtr<FKataGASInspectionRow> Row, ESelectInfo::Type Type);
    void SortChanged(EColumnSortPriority::Type Priority, const FName& Column, EColumnSortMode::Type Mode);
    EColumnSortMode::Type ColumnSort(FName Column) const;
    FReply Refresh();
    FReply ToggleFrozen();
    FReply FollowEditorSelection();
    FReply OpenSelectedAsset();
    FReply CopySelected();
    void OpenRowAsset(TSharedPtr<FKataGASInspectionRow> Row);
    FText WorldLabel() const;
    FText TargetLabel() const;
    FText Summary() const;
    FText Detail() const;
    FText ScanStatus() const;
    void SaveSettings();
    void LoadSettings();

    TUniquePtr<FKataGASInspectionSession> Session;
    TUniquePtr<FKataGASAbilityTriggerIndex> TriggerIndex;
    TAttribute<bool> CanCollect;
    TArray<TSharedPtr<FKataGASInspectionRow>> VisibleRows;
    TArray<TSharedPtr<FKataGASInspectionTarget>> TargetOptions;
    TSharedPtr<SListView<TSharedPtr<FKataGASInspectionTarget>>> TargetList;
    TSharedPtr<STreeView<TSharedPtr<FKataGASInspectionRow>>> Tree;
    TSharedPtr<SHeaderRow> Header;
    TSharedPtr<FKataGASInspectionRow> SelectedRow;
    EKataGASInspectionPage Page = EKataGASInspectionPage::Tags;
    FString Search;
    FString TargetSearch;
    FString TriggerTags;
    FString ContentRoot = TEXT("/Game");
    FName SortColumn = TEXT("Name");
    EColumnSortMode::Type SortMode = EColumnSortMode::Ascending;
    TArray<FName> HiddenColumns;
    bool bActiveOnly = false;
    bool bBlockedOnly = false;
    bool bIncludeChildTags = false;
    bool bStopped = false;
    uint64 LastRevision = MAX_uint64;
    uint64 LastTriggerRevision = MAX_uint64;
    FString NavigationStatus;
};
