#include "KataNode.h"

#include "KataGraph.h"

#define LOCTEXT_NAMESPACE "KataNode"

UKataNode::UKataNode()
{
#if WITH_EDITORONLY_DATA
    // Kata 그래프에만 놓을 수 있다.
    CompatibleGraphType = UKataGraph::StaticClass();
#endif
}

#undef LOCTEXT_NAMESPACE
