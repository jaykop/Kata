#pragma once

#include "CoreMinimal.h"
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
    TSharedRef<SWidget> BuildPage();
    TSharedRef<SWidget> PageMenu();
    void ChangePage(EKataGASInspectionPage Choice);
    void RememberPage();
    bool IsHierarchy() const;
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
    FText SnapshotTooltip() const;
    void SaveSettings();
    void LoadSettings();

    TUniquePtr<FKataGASInspectionSession> Session;
    struct FPageState
    {
        FString Search;
        FName SortColumn = TEXT("Name");
        EColumnSortMode::Type SortMode = EColumnSortMode::Ascending;
        FString SelectionKey;
        TSet<FString> ExpandedKeys;
        // 화면을 다시 만들 때 사용자가 조절한 배치를 유지하도록 표·상세 비율과 열 너비를 화면별로 보관한다.
        float TableRatio = 0.75f;
        float DetailRatio = 0.25f;
        TMap<FName, float> ColumnWidths;
    };
    FPageState PageStates[4];
    TSharedPtr<class SBox> PageHost;
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
    FName SortColumn = TEXT("Name");
    EColumnSortMode::Type SortMode = EColumnSortMode::Ascending;
    bool bActiveOnly = false;
    bool bBlockedOnly = false;
    bool bStopped = false;
    uint64 LastRevision = MAX_uint64;
    FString NavigationStatus;
};
