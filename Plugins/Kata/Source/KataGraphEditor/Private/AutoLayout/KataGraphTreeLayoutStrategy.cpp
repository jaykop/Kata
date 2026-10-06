#include "AutoLayout/KataGraphTreeLayoutStrategy.h"
#include "KataGraphEditorPrivate.h"
#include "SKataEdNode.h"

UKataGraphTreeLayoutStrategy::UKataGraphTreeLayoutStrategy()
{
}

UKataGraphTreeLayoutStrategy::~UKataGraphTreeLayoutStrategy()
{

}

void UKataGraphTreeLayoutStrategy::Layout(UEdGraph* _EdGraph)
{
    if (!BuildLayoutGraph(_EdGraph, true))
    {
        return;
    }

	bool bFirstPassOnly = false;

	if (Settings != nullptr)
	{
		OptimalDistance = Settings->OptimalDistance;
		MaxIteration = Settings->MaxIteration;
		bFirstPassOnly = Settings->bFirstPassOnly;
	}

	FVector2D Anchor(0.f, 0.f);
	for (int32 i = 0; i < LayoutRootNodes.Num(); ++i)
	{
		UKataGraphNodeBase* RootNode = LayoutRootNodes[i];
		InitPass(RootNode, Anchor);

		if (!bFirstPassOnly)
		{
			for (int32 j = 0; j < MaxIteration; ++j)
			{
				bool HasConflict = ResolveConflictPass(RootNode);
				if (!HasConflict)
				{
					break;
				}
			}
		}
	}

	for (int32 i = 0; i < LayoutRootNodes.Num(); ++i)
	{
		for (int32 j = 0; j < i; ++j)
		{
			ResolveConflict(LayoutRootNodes[j], LayoutRootNodes[i]);
		}
	}
}

void UKataGraphTreeLayoutStrategy::InitPass(UKataGraphNodeBase* RootNode, const FVector2D& Anchor)
{
	UKataEdNode* EdNode_RootNode = LayoutNodeMap[RootNode];

	FVector2D ChildAnchor(FVector2D(0.f, GetNodeHeight(EdNode_RootNode) + OptimalDistance + Anchor.Y));
	for (int32 i = 0; i < LayoutChildren.FindChecked(RootNode).Num(); ++i)
	{
		UKataGraphNodeBase* Child = LayoutChildren.FindChecked(RootNode)[i];
		UKataEdNode* EdNode_ChildNode = LayoutNodeMap[Child];
		if (i > 0)
		{
			UKataGraphNodeBase* PreChild = LayoutChildren.FindChecked(RootNode)[i - 1];
			UKataEdNode* EdNode_PreChildNode = LayoutNodeMap[PreChild];
			ChildAnchor.X += OptimalDistance + GetNodeWidth(EdNode_PreChildNode) / 2;
		}
		ChildAnchor.X += GetNodeWidth(EdNode_ChildNode) / 2;
		InitPass(Child, ChildAnchor);
	}
	
	float NodeWidth = GetNodeWidth(EdNode_RootNode);

	EdNode_RootNode->NodePosY = Anchor.Y;
	if (LayoutChildren.FindChecked(RootNode).Num() == 0)
	{
		EdNode_RootNode->NodePosX = Anchor.X - NodeWidth / 2;
	}
	else
	{
		UpdateParentNodePosition(RootNode);
	}
}

bool UKataGraphTreeLayoutStrategy::ResolveConflictPass(UKataGraphNodeBase* Node)
{
	bool HasConflict = false;
	for (int32 i = 0; i < LayoutChildren.FindChecked(Node).Num(); ++i)
	{
		UKataGraphNodeBase* Child = LayoutChildren.FindChecked(Node)[i];
		if (ResolveConflictPass(Child))
		{
			HasConflict = true;
		}
	}

	for (int32 i = 0; i < LayoutParents.FindChecked(Node).Num(); ++i)
	{
		UKataGraphNodeBase* ParentNode = LayoutParents.FindChecked(Node)[i];
		for (int32 j = 0; j < LayoutChildren.FindChecked(ParentNode).Num(); ++j)
		{
			UKataGraphNodeBase* LeftSibling = LayoutChildren.FindChecked(ParentNode)[j];
			if (LeftSibling == Node)
				break;
			if (ResolveConflict(LeftSibling, Node))
			{
				HasConflict = true;
			}
		}
	}

	return HasConflict;
}

bool UKataGraphTreeLayoutStrategy::ResolveConflict(UKataGraphNodeBase* LRoot, UKataGraphNodeBase* RRoot)
{
	TArray<UKataEdNode*> RightContour, LeftContour;

	GetRightContour(LRoot, 0, RightContour);
	GetLeftContour(RRoot, 0, LeftContour);

	int32 MaxOverlapDistance = 0;
	int32 Num = FMath::Min(LeftContour.Num(), RightContour.Num());
	for (int32 i = 0; i < Num; ++i)
	{
		if (RightContour.Contains(LeftContour[i]) || LeftContour.Contains(RightContour[i]))
			break;

		int32 RightBound = RightContour[i]->NodePosX + GetNodeWidth(RightContour[i]);
		int32 LeftBound = LeftContour[i]->NodePosX;
		int32 Distance = RightBound + OptimalDistance - LeftBound;
		if (Distance > MaxOverlapDistance)
		{
			MaxOverlapDistance = Distance;
		}
	}

	if (MaxOverlapDistance > 0)
	{
		ShiftSubTree(RRoot, FVector2D(MaxOverlapDistance, 0.f));

		TArray<UKataGraphNodeBase*> ParentNodes = LayoutParents.FindChecked(RRoot);
		TArray<UKataGraphNodeBase*> NextParentNodes;
		while (ParentNodes.Num() != 0)
		{
			for (int32 i = 0; i < ParentNodes.Num(); ++i)
			{
				UpdateParentNodePosition(ParentNodes[i]);

				NextParentNodes.Append(LayoutParents.FindChecked(ParentNodes[i]));
			}

			ParentNodes = NextParentNodes;
			NextParentNodes.Reset();
		}

		return true;
	}
	else
	{
		return false;
	}
}

void UKataGraphTreeLayoutStrategy::GetLeftContour(UKataGraphNodeBase* RootNode, int32 Level, TArray<UKataEdNode*>& Contour)
{
	UKataEdNode* EdNode_Node = LayoutNodeMap[RootNode];
	if (Level >= Contour.Num())
	{
		Contour.Add(EdNode_Node);
	}
	else if (EdNode_Node->NodePosX < Contour[Level]->NodePosX)
	{
		Contour[Level] = EdNode_Node;
	}

	for (int32 i = 0; i < LayoutChildren.FindChecked(RootNode).Num(); ++i)
	{
		GetLeftContour(LayoutChildren.FindChecked(RootNode)[i], Level + 1, Contour);
	}
}

void UKataGraphTreeLayoutStrategy::GetRightContour(UKataGraphNodeBase* RootNode, int32 Level, TArray<UKataEdNode*>& Contour)
{
	UKataEdNode* EdNode_Node = LayoutNodeMap[RootNode];
	if (Level >= Contour.Num())
	{
		Contour.Add(EdNode_Node);
	}
	else if (EdNode_Node->NodePosX + GetNodeWidth(EdNode_Node) > Contour[Level]->NodePosX + GetNodeWidth(Contour[Level]))
	{
		Contour[Level] = EdNode_Node;
	}

	for (int32 i = 0; i < LayoutChildren.FindChecked(RootNode).Num(); ++i)
	{
		GetRightContour(LayoutChildren.FindChecked(RootNode)[i], Level + 1, Contour);
	}
}

void UKataGraphTreeLayoutStrategy::ShiftSubTree(UKataGraphNodeBase* RootNode, const FVector2D& Offset)
{
	UKataEdNode* EdNode_Node = LayoutNodeMap[RootNode];
	EdNode_Node->NodePosX += Offset.X;
	EdNode_Node->NodePosY += Offset.Y;

	for (int32 i = 0; i < LayoutChildren.FindChecked(RootNode).Num(); ++i)
	{
		UKataGraphNodeBase* Child = LayoutChildren.FindChecked(RootNode)[i];

		if (LayoutParents.FindChecked(Child)[0] == RootNode)
		{
			ShiftSubTree(LayoutChildren.FindChecked(RootNode)[i], Offset);
		}
	}
}

void UKataGraphTreeLayoutStrategy::UpdateParentNodePosition(UKataGraphNodeBase* ParentNode)
{
    if (LayoutChildren.FindChecked(ParentNode).IsEmpty())
    {
        return;
    }
	UKataEdNode* EdNode_ParentNode = LayoutNodeMap[ParentNode];
	if (LayoutChildren.FindChecked(ParentNode).Num() % 2 == 0)
	{
		UKataEdNode* FirstChild = LayoutNodeMap[LayoutChildren.FindChecked(ParentNode)[0]];
		UKataEdNode* LastChild = LayoutNodeMap[LayoutChildren.FindChecked(ParentNode).Last()];
		float LeftBound = FirstChild->NodePosX;
		float RightBound = LastChild->NodePosX + GetNodeWidth(LastChild);
		EdNode_ParentNode->NodePosX = (LeftBound + RightBound) / 2 - GetNodeWidth(EdNode_ParentNode) / 2;
	}
	else
	{
		UKataEdNode* MidChild = LayoutNodeMap[LayoutChildren.FindChecked(ParentNode)[LayoutChildren.FindChecked(ParentNode).Num() / 2]];
		EdNode_ParentNode->NodePosX = MidChild->NodePosX + GetNodeWidth(MidChild) / 2 - GetNodeWidth(EdNode_ParentNode) / 2;
	}
}
