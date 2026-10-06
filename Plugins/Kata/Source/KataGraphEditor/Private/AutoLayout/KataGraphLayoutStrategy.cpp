#include "AutoLayout/KataGraphLayoutStrategy.h"
#include "Kismet/KismetMathLibrary.h"
#include "KataEdNode.h"
#include "SKataEdNode.h"

UKataGraphLayoutStrategy::UKataGraphLayoutStrategy()
{
	Settings = nullptr;
	MaxIteration = 50;
	OptimalDistance = 150;
}

UKataGraphLayoutStrategy::~UKataGraphLayoutStrategy()
{

}

bool UKataGraphLayoutStrategy::BuildLayoutGraph(UEdGraph* SourceGraph, bool bUseSpanningTree)
{
    EdGraph = Cast<UKataEdGraph>(SourceGraph);
    LayoutNodeMap.Reset();
    LayoutNodes.Reset();
    LayoutRootNodes.Reset();
    LayoutChildren.Reset();
    LayoutParents.Reset();
    if (EdGraph == nullptr)
    {
        return false;
    }
    for (UEdGraphNode* Node : EdGraph->Nodes)
    {
        UKataEdNode* EdNode = Cast<UKataEdNode>(Node);
        if (EdNode != nullptr && EdNode->KataNode != nullptr)
        {
            LayoutNodeMap.Add(EdNode->KataNode, EdNode);
            LayoutNodes.Add(EdNode->KataNode);
            LayoutChildren.Add(EdNode->KataNode);
            LayoutParents.Add(EdNode->KataNode);
        }
    }
    auto ComparePosition = [this](UKataGraphNodeBase& Left, UKataGraphNodeBase& Right)
    {
        const UKataEdNode* LeftNode = LayoutNodeMap.FindChecked(&Left);
        const UKataEdNode* RightNode = LayoutNodeMap.FindChecked(&Right);
        return LeftNode->NodePosX == RightNode->NodePosX
            ? LeftNode->NodePosY < RightNode->NodePosY : LeftNode->NodePosX < RightNode->NodePosX;
    };
    LayoutNodes.StableSort(ComparePosition);
    for (UKataGraphNodeBase* Node : LayoutNodes)
    {
        for (const UEdGraphPin* Pin : LayoutNodeMap.FindChecked(Node)->Pins)
        {
            if (Pin == nullptr || Pin->Direction != EGPD_Output)
            {
                continue;
            }
            for (const UEdGraphPin* LinkedPin : Pin->LinkedTo)
            {
                UKataEdNode* Child = Cast<UKataEdNode>(LinkedPin->GetOwningNode());
                if (UKataEdNodeEdge* Edge = Cast<UKataEdNodeEdge>(LinkedPin->GetOwningNode()))
                {
                    Child = Edge->GetEndNode();
                }
                if (Child != nullptr && LayoutNodeMap.Contains(Child->KataNode))
                {
                    LayoutChildren.FindChecked(Node).AddUnique(Child->KataNode);
                    LayoutParents.FindChecked(Child->KataNode).AddUnique(Node);
                }
            }
        }
    }
    TArray<UKataGraphNodeBase*> Candidates;
    for (UKataGraphNodeBase* Node : LayoutNodes)
    {
        LayoutChildren.FindChecked(Node).StableSort(ComparePosition);
        LayoutParents.FindChecked(Node).StableSort(ComparePosition);
        if (LayoutParents.FindChecked(Node).IsEmpty())
        {
            Candidates.Add(Node);
        }
    }
    // 진입점이 없는 순환 성분도 배치한다. 각 노드는 한 번만 숲에 편입한다.
    Candidates.Append(LayoutNodes);
    TSet<UKataGraphNodeBase*> Visited;
    TMap<UKataGraphNodeBase*, TArray<UKataGraphNodeBase*>> TreeChildren, TreeParents;
    for (UKataGraphNodeBase* Node : LayoutNodes)
    {
        TreeChildren.Add(Node);
        TreeParents.Add(Node);
    }
    for (UKataGraphNodeBase* Candidate : Candidates)
    {
        if (Visited.Contains(Candidate))
        {
            continue;
        }
        LayoutRootNodes.Add(Candidate);
        Visited.Add(Candidate);
        TArray<UKataGraphNodeBase*> Pending = { Candidate };
        for (int32 Index = 0; Index < Pending.Num(); ++Index)
        {
            UKataGraphNodeBase* Parent = Pending[Index];
            for (UKataGraphNodeBase* Child : LayoutChildren.FindChecked(Parent))
            {
                if (!Visited.Contains(Child))
                {
                    Visited.Add(Child);
                    Pending.Add(Child);
                    TreeChildren.FindChecked(Parent).Add(Child);
                    TreeParents.FindChecked(Child).Add(Parent);
                }
            }
        }
    }
    if (bUseSpanningTree)
    {
        LayoutChildren = MoveTemp(TreeChildren);
        LayoutParents = MoveTemp(TreeParents);
    }
    return !LayoutNodes.IsEmpty();
}

TArray<UKataGraphNodeBase*> UKataGraphLayoutStrategy::CollectConnectedNodes(UKataGraphNodeBase* RootNode) const
{
    TArray<UKataGraphNodeBase*> Nodes;
    TSet<UKataGraphNodeBase*> Visited;
    if (!LayoutNodeMap.Contains(RootNode))
    {
        return Nodes;
    }
    Nodes.Add(RootNode);
    Visited.Add(RootNode);
    for (int32 Index = 0; Index < Nodes.Num(); ++Index)
    {
        const auto AddUnvisited = [&Nodes, &Visited](const TArray<UKataGraphNodeBase*>& Neighbours)
        {
            for (UKataGraphNodeBase* Node : Neighbours)
            {
                if (!Visited.Contains(Node))
                {
                    Visited.Add(Node);
                    Nodes.Add(Node);
                }
            }
        };
        UKataGraphNodeBase* Node = Nodes[Index];
        AddUnvisited(LayoutChildren.FindChecked(Node));
        AddUnvisited(LayoutParents.FindChecked(Node));
    }
    return Nodes;
}

FBox2D UKataGraphLayoutStrategy::GetNodeBound(UEdGraphNode* EdNode)
{
	int32 NodeWidth = GetNodeWidth(Cast<UKataEdNode>(EdNode));
	int32 NodeHeight = GetNodeHeight(Cast<UKataEdNode>(EdNode));
	FVector2D Min(EdNode->NodePosX, EdNode->NodePosY);
	FVector2D Max(EdNode->NodePosX + NodeWidth, EdNode->NodePosY + NodeHeight);
	return FBox2D(Min, Max);
}

FBox2D UKataGraphLayoutStrategy::GetActualBounds(UKataGraphNodeBase* RootNode)
{
    FBox2D Bounds(ForceInit);
    for (UKataGraphNodeBase* Node : CollectConnectedNodes(RootNode))
    {
        Bounds += GetNodeBound(LayoutNodeMap.FindChecked(Node));
    }
    return Bounds;
}

void UKataGraphLayoutStrategy::RandomLayoutOneTree(UKataGraphNodeBase* RootNode, const FBox2D& Bound)
{
    for (UKataGraphNodeBase* Node : CollectConnectedNodes(RootNode))
    {
        UKataEdNode* EdNode = LayoutNodeMap.FindChecked(Node);
        EdNode->NodePosX = UKismetMathLibrary::RandomFloatInRange(Bound.Min.X, Bound.Max.X);
        EdNode->NodePosY = UKismetMathLibrary::RandomFloatInRange(Bound.Min.Y, Bound.Max.Y);
    }
}

int32 UKataGraphLayoutStrategy::GetNodeWidth(UKataEdNode* EdNode)
{
    return EdNode->SEdNode != nullptr ? FMath::Max(1, int32(EdNode->SEdNode->GetCachedGeometry().GetLocalSize().X)) : 160;
}

int32 UKataGraphLayoutStrategy::GetNodeHeight(UKataEdNode* EdNode)
{
    return EdNode->SEdNode != nullptr ? FMath::Max(1, int32(EdNode->SEdNode->GetCachedGeometry().GetLocalSize().Y)) : 80;
}

