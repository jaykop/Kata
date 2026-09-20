#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraph.h"
#include "KataEdGraph.generated.h"

class UKataGraphBase;
class UKataGraphNodeBase;
class UKataGraphEdgeBase;
class UKataEdNode;
class UKataEdNodeEdge;

UCLASS()
class KATAGRAPHEDITOR_API UKataEdGraph : public UEdGraph
{
	GENERATED_BODY()

public:
	UKataEdGraph();
	virtual ~UKataEdGraph();

	virtual void RebuildKataGraph();

	UKataGraphBase* GetKataGraph() const;

	virtual bool Modify(bool bAlwaysMarkDirty = true) override;
	virtual void PostEditUndo() override;

	UPROPERTY(Transient)
	TMap<UKataGraphNodeBase*, UKataEdNode*> NodeMap;

	UPROPERTY(Transient)
	TMap<UKataGraphEdgeBase*, UKataEdNodeEdge*> EdgeMap;

protected:
	void Clear();

	void SortNodes(UKataGraphNodeBase* RootNode);
};
