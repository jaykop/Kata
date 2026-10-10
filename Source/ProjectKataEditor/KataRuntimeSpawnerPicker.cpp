#include "KataRuntimeSpawnerPicker.h"

#include "EdGraph/EdGraphSchema.h"
#include "SGraphActionMenu.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

namespace KataRuntimeSpawnerPickerPrivate
{
    /** 메뉴 항목 하나. 고르면 SourceIndex를 선택으로 남긴다. */
    struct FPickerAction : public FEdGraphSchemaAction
    {
        FPickerAction(FText InCategory, FText InLabel, FText InToolTip, int32 InSourceIndex)
            : FEdGraphSchemaAction(MoveTemp(InCategory), MoveTemp(InLabel), MoveTemp(InToolTip), 0)
            , SourceIndex(InSourceIndex)
        {
        }

        int32 SourceIndex = INDEX_NONE;
    };
}

TSharedRef<SWidget> FKataRuntimeSpawnerPicker::GetWidget()
{
    if (!ComboButton.IsValid())
    {
        TWeakPtr<FKataRuntimeSpawnerPicker> WeakThis = AsShared();
        SAssignNew(ComboButton, SComboButton)
            .OnGetMenuContent(FOnGetContent::CreateSP(this, &FKataRuntimeSpawnerPicker::MakeMenu))
            .ButtonContent()
            [
                SNew(STextBlock)
                .Text_Lambda([WeakThis]()
                {
                    const TSharedPtr<FKataRuntimeSpawnerPicker> Pinned = WeakThis.Pin();
                    return FText::FromString(Pinned.IsValid() && !Pinned->CurrentLabel.IsEmpty() ? Pinned->CurrentLabel : TEXT("Select..."));
                })
            ];
    }
    return ComboButton.ToSharedRef();
}

int32 FKataRuntimeSpawnerPicker::ConsumeSelection()
{
    const int32 Selection = PendingSelection;
    PendingSelection = INDEX_NONE;
    return Selection;
}

TSharedRef<SWidget> FKataRuntimeSpawnerPicker::MakeMenu()
{
    TSharedRef<SGraphActionMenu> Menu = SNew(SGraphActionMenu)
        .OnCollectAllActions(SGraphActionMenu::FOnCollectAllActions::CreateSP(this, &FKataRuntimeSpawnerPicker::CollectActions))
        .OnActionSelected(SGraphActionMenu::FOnActionSelected::CreateSP(this, &FKataRuntimeSpawnerPicker::HandleActionSelected))
        .AutoExpandActionMenu(true)
        .ShowFilterTextBox(true);
    // 메뉴가 열리면 바로 입력할 수 있게 검색창에 포커스를 준다.
    ComboButton->SetMenuContentWidgetToFocus(Menu->GetFilterTextBox());
    return SNew(SBox).WidthOverride(360.f).HeightOverride(420.f)
    [
        Menu
    ];
}

void FKataRuntimeSpawnerPicker::CollectActions(FGraphActionListBuilderBase& OutActions)
{
    for (const FKataRuntimeSpawnerPickerItem& Item : Items)
    {
        OutActions.AddAction(MakeShared<KataRuntimeSpawnerPickerPrivate::FPickerAction>(FText::FromString(Item.Category),
            FText::FromString(Item.Label), FText::FromString(Item.ToolTip), Item.SourceIndex));
    }
}

void FKataRuntimeSpawnerPicker::HandleActionSelected(const TArray<TSharedPtr<FEdGraphSchemaAction>>& Actions, ESelectInfo::Type SelectionType)
{
    // 키보드로 목록을 훑는 선택은 무시하고 클릭이나 Enter로 고른 항목만 받는다.
    if (SelectionType != ESelectInfo::OnMouseClick && SelectionType != ESelectInfo::OnKeyPress)
    {
        return;
    }
    for (const TSharedPtr<FEdGraphSchemaAction>& Action : Actions)
    {
        // 이 메뉴에는 FPickerAction만 넣으므로 그대로 내려 변환한다.
        if (Action.IsValid())
        {
            PendingSelection = StaticCastSharedPtr<KataRuntimeSpawnerPickerPrivate::FPickerAction>(Action)->SourceIndex;
            break;
        }
    }
    if (ComboButton.IsValid())
    {
        ComboButton->SetIsOpen(false);
    }
}
