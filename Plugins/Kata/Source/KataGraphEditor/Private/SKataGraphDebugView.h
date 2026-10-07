#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

class FKataGraphDebugger;
class ITableRow;
class STableViewBase;
class UKataGraphInstance;
class UKataGraphNodeBase;

DECLARE_DELEGATE_OneParam(FOnKataGraphDebugJump, const UKataGraphNodeBase*);

/** Debug 탭 목록의 한 줄. 표시 문자열은 기록을 받을 때 한 번 만든다. */
struct FKataGraphDebugRow
{
    FText Time;
    FText Event;
    FText Detail;
    /** 더블클릭 시 이동할 노드. 버려진 실행 사본은 붙잡지 않는다. */
    TWeakObjectPtr<const UKataGraphNodeBase> Node;
};

/**
 * 디버그 대상 그래프 인스턴스의 상태 요약과 최근 실행 기록을 보여 준다.
 *
 * 인스턴스의 기록 일련번호가 바뀔 때만 목록을 다시 만든다. 최신 기록이 위에 온다.
 */
class SKataGraphDebugView : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SKataGraphDebugView) {}
        SLATE_EVENT(FOnKataGraphDebugJump, OnJumpToNode)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs, const TSharedRef<FKataGraphDebugger>& InDebugger);

    virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
    void RebuildRows(const UKataGraphInstance* Instance);
    TSharedRef<ITableRow> GenerateRow(TSharedPtr<FKataGraphDebugRow> Row, const TSharedRef<STableViewBase>& OwnerTable);
    void HandleRowDoubleClicked(TSharedPtr<FKataGraphDebugRow> Row);

    FText GetTargetText() const;
    FText GetStateText() const;
    FText GetCurrentText() const;
    FText GetPendingText() const;

    TWeakPtr<FKataGraphDebugger> Debugger;
    FOnKataGraphDebugJump OnJumpToNode;
    TArray<TSharedPtr<FKataGraphDebugRow>> Rows;
    TSharedPtr<SListView<TSharedPtr<FKataGraphDebugRow>>> ListView;
    TWeakObjectPtr<const UKataGraphInstance> ShownInstance;
    uint32 ShownSerial = 0;
};
