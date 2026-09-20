#include "KataActionNode.h"

#include "Action/KataAction.h"

#define LOCTEXT_NAMESPACE "KataActionNode"

UKataActionNode::UKataActionNode()
{
#if WITH_EDITORONLY_DATA
    ContextMenuName = LOCTEXT("ContextMenuName", "Action Node");
    BackgroundColor = FLinearColor(0.05f, 0.18f, 0.25f);
#endif
}

FText UKataActionNode::GetDescription_Implementation() const
{
    // 그래프에서 어떤 액션인지 바로 보이도록 에셋 이름을 쓴다.
    if (Action != nullptr)
    {
        return FText::FromString(Action->GetName());
    }
    return LOCTEXT("NoAction", "(No Action)");
}

#undef LOCTEXT_NAMESPACE
