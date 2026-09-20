#include "KataEntryNode.h"

#define LOCTEXT_NAMESPACE "KataEntryNode"

UKataEntryNode::UKataEntryNode()
{
#if WITH_EDITORONLY_DATA
    ContextMenuName = LOCTEXT("ContextMenuName", "Entry Node");
    BackgroundColor = FLinearColor(0.10f, 0.22f, 0.08f);

    // 진입점은 들어오는 연결을 가질 수 없다.
    ParentLimitType = EKataGraphNodeLimit::Limited;
    ParentLimit = 0;
#endif
}

FText UKataEntryNode::GetDescription_Implementation() const
{
    return LOCTEXT("NodeDesc", "Entry");
}

#undef LOCTEXT_NAMESPACE
