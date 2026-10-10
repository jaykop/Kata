#pragma once

#include "CoreMinimal.h"
#include "Types/SlateEnums.h"

class SComboButton;
class SWidget;
struct FEdGraphSchemaAction;
struct FGraphActionListBuilderBase;

/** 선택 메뉴의 항목 하나. SourceIndex는 호출자의 원본 목록 인덱스다. */
struct FKataRuntimeSpawnerPickerItem
{
    FString Label;
    /** 비우면 최상위에 놓는다. 같은 문자열끼리 하나의 접히는 묶음이 된다. */
    FString Category;
    FString ToolTip;
    int32 SourceIndex = INDEX_NONE;
};

/**
 * 버튼을 누르면 검색창과 카테고리 트리가 있는 메뉴(SGraphActionMenu)를 여는 선택 위젯이다. Kata 액션 에디터의 Add Task 메뉴와 같은 위젯이다.
 * SlateIM::Widget으로 매 프레임 같은 인스턴스를 넘겨 쓴다. 메뉴는 열 때마다 현재 항목으로 다시 만들며,
 * 고른 항목은 ConsumeSelection으로 한 번만 꺼낸다.
 */
class FKataRuntimeSpawnerPicker : public TSharedFromThis<FKataRuntimeSpawnerPicker>
{
public:
    void SetItems(TArray<FKataRuntimeSpawnerPickerItem>&& InItems) { Items = MoveTemp(InItems); }

    /** 버튼에 보일 현재 선택 이름. 비우면 "Select..."를 보인다. */
    void SetCurrentLabel(const FString& InLabel) { CurrentLabel = InLabel; }

    TSharedRef<SWidget> GetWidget();

    /** 지난 프레임 이후 사용자가 고른 항목의 SourceIndex. 없으면 INDEX_NONE이다. */
    int32 ConsumeSelection();

private:
    TSharedRef<SWidget> MakeMenu();
    void CollectActions(FGraphActionListBuilderBase& OutActions);
    void HandleActionSelected(const TArray<TSharedPtr<FEdGraphSchemaAction>>& Actions, ESelectInfo::Type SelectionType);

    TArray<FKataRuntimeSpawnerPickerItem> Items;
    FString CurrentLabel;
    int32 PendingSelection = INDEX_NONE;
    TSharedPtr<SComboButton> ComboButton;
};
