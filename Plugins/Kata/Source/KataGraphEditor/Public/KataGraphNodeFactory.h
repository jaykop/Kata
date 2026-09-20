#pragma once
#include <EdGraphUtilities.h>
#include <EdGraph/EdGraphNode.h>

class FKataGraphNodeFactory : public FGraphPanelNodeFactory
{
	virtual TSharedPtr<class SGraphNode> CreateNode(UEdGraphNode* Node) const override;
};