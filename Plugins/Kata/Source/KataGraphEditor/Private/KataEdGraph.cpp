#include "KataEdGraph.h"
#include "KataGraphEditorPrivate.h"
#include "KataGraphBase.h"
#include "KataEdNode.h"
#include "KataEdNodeEdge.h"

UKataEdGraph::UKataEdGraph()
{

}

UKataEdGraph::~UKataEdGraph()
{

}

void UKataEdGraph::RebuildKataGraph()
{
	LOG_INFO(TEXT("UKataEdGraph::RebuildKataGraph has been called"));

	UKataGraphBase* Graph = GetKataGraph();

	Clear();

	for (int i = 0; i < Nodes.Num(); ++i)
	{
		if (UKataEdNode* EdNode = Cast<UKataEdNode>(Nodes[i]))
		{
			if (EdNode->KataNode == nullptr)
				continue;

			UKataGraphNodeBase* GraphNode = EdNode->KataNode;

			NodeMap.Add(GraphNode, EdNode);

			Graph->AllNodes.Add(GraphNode);

			for (int PinIdx = 0; PinIdx < EdNode->Pins.Num(); ++PinIdx)
			{
				UEdGraphPin* Pin = EdNode->Pins[PinIdx];

				if (Pin->Direction != EEdGraphPinDirection::EGPD_Output)
					continue;

				for (int LinkToIdx = 0; LinkToIdx < Pin->LinkedTo.Num(); ++LinkToIdx)
				{
					UKataGraphNodeBase* ChildNode = nullptr;
					if (UKataEdNode* EdNode_Child = Cast<UKataEdNode>(Pin->LinkedTo[LinkToIdx]->GetOwningNode()))
					{
						ChildNode = EdNode_Child->KataNode;
					}
					else if (UKataEdNodeEdge* EdNode_Edge = Cast<UKataEdNodeEdge>(Pin->LinkedTo[LinkToIdx]->GetOwningNode()))
					{
						UKataEdNode* Child = EdNode_Edge->GetEndNode();;
						if (Child != nullptr)
						{
							ChildNode = Child->KataNode;
						}
					}

					if (ChildNode != nullptr)
					{
						GraphNode->ChildrenNodes.Add(ChildNode);

						ChildNode->ParentNodes.Add(GraphNode);
					}
					else
					{
						LOG_ERROR(TEXT("UKataEdGraph::RebuildKataGraph can't find child node"));
					}
				}
			}
		}
		else if (UKataEdNodeEdge* EdgeNode = Cast<UKataEdNodeEdge>(Nodes[i]))
		{
			UKataEdNode* StartNode = EdgeNode->GetStartNode();
			UKataEdNode* EndNode = EdgeNode->GetEndNode();
			UKataGraphEdgeBase* Edge = EdgeNode->KataEdge;

			if (StartNode == nullptr || EndNode == nullptr || Edge == nullptr)
			{
				LOG_ERROR(TEXT("UKataEdGraph::RebuildKataGraph add edge failed."));
				continue;
			}

			EdgeMap.Add(Edge, EdgeNode);

			Edge->Graph = Graph;
			Edge->Rename(nullptr, Graph, REN_DontCreateRedirectors | REN_DoNotDirty);
			Edge->StartNode = StartNode->KataNode;
			Edge->EndNode = EndNode->KataNode;
			Edge->StartNode->AddEdge(Edge->EndNode, Edge);
		}
	}

	for (int i = 0; i < Graph->AllNodes.Num(); ++i)
	{
		UKataGraphNodeBase* Node = Graph->AllNodes[i];
		if (Node->ParentNodes.Num() == 0)
		{
			Graph->RootNodes.Add(Node);

			SortNodes(Node);
		}

		Node->Graph = Graph;
		Node->Rename(nullptr, Graph, REN_DontCreateRedirectors | REN_DoNotDirty);
	}

	Graph->RootNodes.Sort([&](const UKataGraphNodeBase& L, const UKataGraphNodeBase& R)
	{
		UKataEdNode* EdNode_LNode = NodeMap[&L];
		UKataEdNode* EdNode_RNode = NodeMap[&R];
		return EdNode_LNode->NodePosX < EdNode_RNode->NodePosX;
	});
}

UKataGraphBase* UKataEdGraph::GetKataGraph() const
{
	return CastChecked<UKataGraphBase>(GetOuter());
}

bool UKataEdGraph::Modify(bool bAlwaysMarkDirty /*= true*/)
{
	bool Rtn = Super::Modify(bAlwaysMarkDirty);

	GetKataGraph()->Modify();

	for (int32 i = 0; i < Nodes.Num(); ++i)
	{
		Nodes[i]->Modify();
	}

	return Rtn;
}

void UKataEdGraph::Clear()
{
	UKataGraphBase* Graph = GetKataGraph();

	Graph->ClearGraph();
	NodeMap.Reset();
	EdgeMap.Reset();

	for (int i = 0; i < Nodes.Num(); ++i)
	{
		if (UKataEdNode* EdNode = Cast<UKataEdNode>(Nodes[i]))
		{
			UKataGraphNodeBase* GraphNode = EdNode->KataNode;
			if (GraphNode)
			{
				GraphNode->ParentNodes.Reset();
				GraphNode->ChildrenNodes.Reset();
				GraphNode->Edges.Reset();
			}
		}
	}
}

void UKataEdGraph::SortNodes(UKataGraphNodeBase* RootNode)
{
	int Level = 0;
	TArray<UKataGraphNodeBase*> CurrLevelNodes = { RootNode };
	TArray<UKataGraphNodeBase*> NextLevelNodes;
	TSet<UKataGraphNodeBase*> Visited;

	while (CurrLevelNodes.Num() != 0)
	{
		int32 LevelWidth = 0;
		for (int i = 0; i < CurrLevelNodes.Num(); ++i)
		{
			UKataGraphNodeBase* Node = CurrLevelNodes[i];
			Visited.Add(Node);

			auto Comp = [&](const UKataGraphNodeBase& L, const UKataGraphNodeBase& R)
			{
				UKataEdNode* EdNode_LNode = NodeMap[&L];
				UKataEdNode* EdNode_RNode = NodeMap[&R];
				return EdNode_LNode->NodePosX < EdNode_RNode->NodePosX;
			};

			Node->ChildrenNodes.Sort(Comp);
			Node->ParentNodes.Sort(Comp);

			for (int j = 0; j < Node->ChildrenNodes.Num(); ++j)
			{
				UKataGraphNodeBase* ChildNode = Node->ChildrenNodes[j];
				if(!Visited.Contains(ChildNode))
					NextLevelNodes.Add(Node->ChildrenNodes[j]);
			}
		}

		CurrLevelNodes = NextLevelNodes;
		NextLevelNodes.Reset();
		++Level;
	}
}

void UKataEdGraph::PostEditUndo()
{
	Super::PostEditUndo();

	NotifyGraphChanged();
}

