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
	int Level = 0;
	TArray<UKataGraphNodeBase*> CurrLevelNodes = { RootNode };
	TArray<UKataGraphNodeBase*> NextLevelNodes;

	FBox2D Rtn = GetNodeBound(EdGraph->NodeMap[RootNode]);

	while (CurrLevelNodes.Num() != 0)
	{
		for (int i = 0; i < CurrLevelNodes.Num(); ++i)
		{
			UKataGraphNodeBase* Node = CurrLevelNodes[i];
			check(Node != nullptr);

			Rtn += GetNodeBound(EdGraph->NodeMap[Node]);

			for (int j = 0; j < Node->ChildrenNodes.Num(); ++j)
			{
				NextLevelNodes.Add(Node->ChildrenNodes[j]);
			}
		}

		CurrLevelNodes = NextLevelNodes;
		NextLevelNodes.Reset();
		++Level;
	}
	return Rtn;
}

void UKataGraphLayoutStrategy::RandomLayoutOneTree(UKataGraphNodeBase* RootNode, const FBox2D& Bound)
{
	int Level = 0;
	TArray<UKataGraphNodeBase*> CurrLevelNodes = { RootNode };
	TArray<UKataGraphNodeBase*> NextLevelNodes;

	while (CurrLevelNodes.Num() != 0)
	{
		for (int i = 0; i < CurrLevelNodes.Num(); ++i)
		{
			UKataGraphNodeBase* Node = CurrLevelNodes[i];
			check(Node != nullptr);

			UKataEdNode* EdNode_Node = EdGraph->NodeMap[Node];

			EdNode_Node->NodePosX = UKismetMathLibrary::RandomFloatInRange(Bound.Min.X, Bound.Max.X);
			EdNode_Node->NodePosY = UKismetMathLibrary::RandomFloatInRange(Bound.Min.Y, Bound.Max.Y);

			for (int j = 0; j < Node->ChildrenNodes.Num(); ++j)
			{
				NextLevelNodes.Add(Node->ChildrenNodes[j]);
			}
		}

		CurrLevelNodes = NextLevelNodes;
		NextLevelNodes.Reset();
		++Level;
	}
}

int32 UKataGraphLayoutStrategy::GetNodeWidth(UKataEdNode* EdNode)
{
	return EdNode->SEdNode->GetCachedGeometry().GetLocalSize().X;
}

int32 UKataGraphLayoutStrategy::GetNodeHeight(UKataEdNode* EdNode)
{
	return EdNode->SEdNode->GetCachedGeometry().GetLocalSize().Y;
}

