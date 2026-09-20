#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphNode.h"
#include "KataEdNodeEdge.generated.h"

class UKataGraphNodeBase;
class UKataGraphEdgeBase;
class UKataEdNode;

UCLASS(MinimalAPI)
class UKataEdNodeEdge : public UEdGraphNode
{
	GENERATED_BODY()

public:
	UKataEdNodeEdge();

	UPROPERTY()
	class UEdGraph* Graph;

	UPROPERTY(VisibleAnywhere, Instanced, Category = "KataGraph")
	UKataGraphEdgeBase* KataEdge;

	void SetEdge(UKataGraphEdgeBase* Edge);

	virtual void AllocateDefaultPins() override;

	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;

	virtual void PinConnectionListChanged(UEdGraphPin* Pin) override;

	virtual void PrepareForCopying() override;

	virtual UEdGraphPin* GetInputPin() const { return Pins[0]; }
	virtual UEdGraphPin* GetOutputPin() const { return Pins[1]; }

	void CreateConnections(UKataEdNode* Start, UKataEdNode* End);

	UKataEdNode* GetStartNode();
	UKataEdNode* GetEndNode();
};
