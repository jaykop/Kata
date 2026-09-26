#include "KataAliasNode.h"

#define LOCTEXT_NAMESPACE "KataAliasNode"

UKataAliasNode::UKataAliasNode()
{
#if WITH_EDITORONLY_DATA
    ContextMenuName = LOCTEXT("ContextMenuName", "Alias Node");
    BackgroundColor = FLinearColor(0.22f, 0.09f, 0.17f);

    // 별칭은 출발지이므로 들어오는 연결을 받지 않는다.
    ParentLimitType = EKataGraphNodeLimit::Limited;
    ParentLimit = 0;
#endif
}

bool UKataAliasNode::CoversNode(const UKataNode* Node) const
{
    if (Node == nullptr || Node == this)
    {
        return false;
    }

    if (bAnyState)
    {
        // 머무를 수 있는 노드만 출발지가 된다. 경유 노드와 다른 별칭에는 머무를 수 없다.
        return Node->IsExecutableState();
    }

    for (const TObjectPtr<UKataNode>& Source : SourceNodes.Nodes)
    {
        // 참조하던 노드가 지워지면 항목이 비므로 Get으로 비교한다.
        if (Source.Get() == Node)
        {
            return true;
        }
    }
    return false;
}

FText UKataAliasNode::GetDescription_Implementation() const
{
    if (bAnyState)
    {
        return LOCTEXT("AnyState", "Any State");
    }
    return FText::Format(LOCTEXT("AliasWithCount", "Alias ({0})"), FText::AsNumber(SourceNodes.Nodes.Num()));
}

#undef LOCTEXT_NAMESPACE
