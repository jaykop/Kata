#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphNode.h"
#include "KataGraphNodeBase.h"
#include "KataEdNode.generated.h"

class UKataEdNodeEdge;
class UKataEdGraph;
class SKataEdNode;

UCLASS(MinimalAPI)
class UKataEdNode : public UEdGraphNode
{
	GENERATED_BODY()

public:
	UKataEdNode();
	virtual ~UKataEdNode();

	UPROPERTY(VisibleAnywhere, Instanced, Category = "KataGraph")
	UKataGraphNodeBase* KataNode;

	void SetKataNode(UKataGraphNodeBase* InNode);
	UKataEdGraph* GetKataEdGraph();

	SKataEdNode* SEdNode;

	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual void PrepareForCopying() override;
	virtual void AutowireNewNode(UEdGraphPin* FromPin) override;

	virtual FLinearColor GetBackgroundColor() const;
	virtual UEdGraphPin* GetInputPin() const;
	virtual UEdGraphPin* GetOutputPin() const;

#if WITH_EDITOR
	virtual void PostEditUndo() override;
#endif

};
