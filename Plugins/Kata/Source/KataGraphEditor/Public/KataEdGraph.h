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

	/**
	 * SubGraph 포트가 가리키는 그래프를 이 그래프 안으로 펼친다.
	 *
	 * 같은 서브그래프를 가리키는 포트가 몇 개든 사본은 하나만 만들고 모든 포트가 그 사본으로 이어진다.
	 * Port In은 들어오던 엣지를 사본의 진입 노드로 넘기고, Port Out은 사본을 출발지로 갖는 별칭이 된다.
	 * 펼친 뒤 포트는 그래프에서 사라지므로 실행기는 서브그래프가 있었다는 사실을 모른다.
	 */
	virtual void FlattenSubGraphs();

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
