#include "KataGraphEdgeBase.h"

UKataGraphEdgeBase::UKataGraphEdgeBase()
{
}

UKataGraphBase* UKataGraphEdgeBase::GetGraph() const
{
    return Graph;
}

#if WITH_EDITOR
void UKataGraphEdgeBase::SetNodeTitle(const FText& NewTitle)
{
    NodeTitle = NewTitle;
}
#endif
