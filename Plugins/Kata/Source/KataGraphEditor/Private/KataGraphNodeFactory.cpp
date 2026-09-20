#include "KataGraphNodeFactory.h"
#include <EdGraph/EdGraphNode.h>
#include "SKataEdNodeEdge.h"
#include "KataEdNode.h"
#include "SKataEdNode.h"
#include "KataEdNodeEdge.h"

TSharedPtr<class SGraphNode> FKataGraphNodeFactory::CreateNode(UEdGraphNode* Node) const
{
	if (UKataEdNode* EdNode_GraphNode = Cast<UKataEdNode>(Node))
	{
		return SNew(SKataEdNode, EdNode_GraphNode);
	}
	else if (UKataEdNodeEdge* EdNode_Edge = Cast<UKataEdNodeEdge>(Node))
	{
		return SNew(SKataEdNodeEdge, EdNode_Edge);
	}
	return nullptr;
}

