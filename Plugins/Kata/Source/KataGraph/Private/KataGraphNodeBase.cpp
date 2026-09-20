#include "KataGraphNodeBase.h"

#include "KataGraphBase.h"
#include "KataGraphEdgeBase.h"

#define LOCTEXT_NAMESPACE "KataGraphNode"

UKataGraphNodeBase::UKataGraphNodeBase()
{
#if WITH_EDITORONLY_DATA
    CompatibleGraphType = UKataGraphBase::StaticClass();
    BackgroundColor = FLinearColor::Black;
#endif
}

UKataGraphEdgeBase* UKataGraphNodeBase::GetEdge(UKataGraphNodeBase* ChildNode) const
{
    if (const FKataGraphEdgeList* List = Edges.Find(ChildNode))
    {
        for (const TObjectPtr<UKataGraphEdgeBase>& Edge : List->Edges)
        {
            if (Edge != nullptr)
            {
                return Edge;
            }
        }
    }
    return nullptr;
}

void UKataGraphNodeBase::GetEdgesTo(UKataGraphNodeBase* ChildNode, TArray<UKataGraphEdgeBase*>& OutEdges) const
{
    OutEdges.Reset();
    if (const FKataGraphEdgeList* List = Edges.Find(ChildNode))
    {
        for (const TObjectPtr<UKataGraphEdgeBase>& Edge : List->Edges)
        {
            if (Edge != nullptr)
            {
                OutEdges.Add(Edge);
            }
        }
    }
}

void UKataGraphNodeBase::GetOutgoingEdges(TArray<UKataGraphEdgeBase*>& OutEdges) const
{
    OutEdges.Reset();
    for (const TPair<TObjectPtr<UKataGraphNodeBase>, FKataGraphEdgeList>& Pair : Edges)
    {
        for (const TObjectPtr<UKataGraphEdgeBase>& Edge : Pair.Value.Edges)
        {
            if (Edge != nullptr)
            {
                OutEdges.Add(Edge);
            }
        }
    }
}

void UKataGraphNodeBase::AddEdge(UKataGraphNodeBase* ChildNode, UKataGraphEdgeBase* Edge)
{
    if (ChildNode == nullptr || Edge == nullptr)
    {
        return;
    }
    Edges.FindOrAdd(ChildNode).Edges.AddUnique(Edge);
}

FText UKataGraphNodeBase::GetDescription_Implementation() const
{
    return LOCTEXT("NodeDesc", "Kata Graph Node");
}

bool UKataGraphNodeBase::IsLeafNode() const
{
    return ChildrenNodes.Num() == 0;
}

UKataGraphBase* UKataGraphNodeBase::GetGraph() const
{
    return Graph;
}

#if WITH_EDITOR
bool UKataGraphNodeBase::IsNameEditable() const
{
    return true;
}

FLinearColor UKataGraphNodeBase::GetBackgroundColor() const
{
    return BackgroundColor;
}

FText UKataGraphNodeBase::GetNodeTitle() const
{
    return NodeTitle.IsEmpty() ? GetDescription() : NodeTitle;
}

void UKataGraphNodeBase::SetNodeTitle(const FText& NewTitle)
{
    NodeTitle = NewTitle;
}

bool UKataGraphNodeBase::CanCreateConnection(UKataGraphNodeBase* Other, FText& ErrorMessage)
{
    return true;
}

bool UKataGraphNodeBase::CanCreateConnectionTo(UKataGraphNodeBase* Other, int32 NumberOfChildrenNodes, FText& ErrorMessage)
{
    if (ChildrenLimitType == EKataGraphNodeLimit::Limited && NumberOfChildrenNodes >= ChildrenLimit)
    {
        ErrorMessage = FText::FromString(TEXT("Children limit exceeded"));
        return false;
    }
    return CanCreateConnection(Other, ErrorMessage);
}

bool UKataGraphNodeBase::CanCreateConnectionFrom(UKataGraphNodeBase* Other, int32 NumberOfParentNodes, FText& ErrorMessage)
{
    if (ParentLimitType == EKataGraphNodeLimit::Limited && NumberOfParentNodes >= ParentLimit)
    {
        ErrorMessage = FText::FromString(TEXT("Parent limit exceeded"));
        return false;
    }
    return true;
}
#endif

#undef LOCTEXT_NAMESPACE
