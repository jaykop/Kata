#include "KataSubGraphNode.h"

#include "KataGraph.h"

#define LOCTEXT_NAMESPACE "KataSubGraphNode"

UKataSubGraphNode::UKataSubGraphNode()
{
#if WITH_EDITORONLY_DATA
    bUseEmbeddedSubGraph = true;
    ContextMenuName = LOCTEXT("MenuName", "SubGraph");
    BackgroundColor = FLinearColor(0.08f, 0.20f, 0.16f);
#endif
}

FText UKataSubGraphNode::GetDescription_Implementation() const
{
#if WITH_EDITOR
    if (const UKataGraph* Source = GetReferencedSubGraph())
    {
        return Source->GetGraphDisplayName();
    }
#endif
    return LOCTEXT("Unassigned", "SubGraph");
}

#if WITH_EDITOR
bool UKataSubGraphNode::IsNameEditable() const
{
    return bUseEmbeddedSubGraph && GetReferencedSubGraph() != nullptr;
}
#endif

#undef LOCTEXT_NAMESPACE
