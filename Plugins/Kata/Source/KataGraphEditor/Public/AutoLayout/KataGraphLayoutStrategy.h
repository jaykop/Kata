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
    /** 현재 화면의 저작 연결을 모은다. 트리 배치는 순환과 합류를 끊은 임시 숲을 사용한다. */
    bool BuildLayoutGraph(UEdGraph* SourceGraph, bool bUseSpanningTree);
    TArray<UKataGraphNodeBase*> CollectConnectedNodes(UKataGraphNodeBase* RootNode) const;

	int32 GetNodeWidth(UKataEdNode* EdNode);

	int32 GetNodeHeight(UKataEdNode* EdNode);

	FBox2D GetNodeBound(UEdGraphNode* EdNode);

	FBox2D GetActualBounds(UKataGraphNodeBase* RootNode);

	virtual void RandomLayoutOneTree(UKataGraphNodeBase* RootNode, const FBox2D& Bound);

protected:
	UKataEdGraph* EdGraph;
    // 동기 배치 동안만 사용하는 저작 연결이다. 실행 노드의 연결과 저장 결과를 변경하지 않는다.
    TMap<UKataGraphNodeBase*, UKataEdNode*> LayoutNodeMap;
    TArray<UKataGraphNodeBase*> LayoutNodes;
    TArray<UKataGraphNodeBase*> LayoutRootNodes;
    TMap<UKataGraphNodeBase*, TArray<UKataGraphNodeBase*>> LayoutChildren;
    TMap<UKataGraphNodeBase*, TArray<UKataGraphNodeBase*>> LayoutParents;
	int32 MaxIteration;
	int32 OptimalDistance;
};
