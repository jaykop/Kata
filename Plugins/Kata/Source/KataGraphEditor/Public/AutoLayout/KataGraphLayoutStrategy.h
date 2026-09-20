#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraph.h"
#include "KataGraphBase.h"
#include "KataEdGraph.h"
#include "KataEdNode.h"
#include "KataEdNodeEdge.h"
#include "KataGraphEditorSettings.h"
#include "KataGraphLayoutStrategy.generated.h"

UCLASS(abstract)
class KATAGRAPHEDITOR_API UKataGraphLayoutStrategy : public UObject
{
	GENERATED_BODY()
public:
	UKataGraphLayoutStrategy();
	virtual ~UKataGraphLayoutStrategy();

	virtual void Layout(UEdGraph* G) {};

	class UKataGraphEditorSettings* Settings;

protected:
	int32 GetNodeWidth(UKataEdNode* EdNode);

	int32 GetNodeHeight(UKataEdNode* EdNode);

	FBox2D GetNodeBound(UEdGraphNode* EdNode);

	FBox2D GetActualBounds(UKataGraphNodeBase* RootNode);

	virtual void RandomLayoutOneTree(UKataGraphNodeBase* RootNode, const FBox2D& Bound);

protected:
	UKataGraphBase* Graph;
	UKataEdGraph* EdGraph;
	int32 MaxIteration;
	int32 OptimalDistance;
};
