#include "KataConduitNode.h"

#define LOCTEXT_NAMESPACE "KataConduitNode"

UKataConduitNode::UKataConduitNode()
{
#if WITH_EDITORONLY_DATA
    ContextMenuName = LOCTEXT("ContextMenuName", "Conduit Node");
    BackgroundColor = FLinearColor(0.24f, 0.16f, 0.06f);
#endif
}

FText UKataConduitNode::GetDescription_Implementation() const
{
    return LOCTEXT("NodeDesc", "Conduit");
}

#undef LOCTEXT_NAMESPACE
