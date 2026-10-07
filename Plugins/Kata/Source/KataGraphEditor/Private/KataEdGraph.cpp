#include "KataEdGraph.h"
#include "KataGraphEditorPrivate.h"
#include "KataGraphDebugger.h"
#include "KataGraphBase.h"
#include "KataEdNode.h"
#include "KataEdNodeEdge.h"
#include "KataAliasNode.h"
#include "KataEntryNode.h"
#include "KataGraph.h"
#include "KataGraphEdgeBase.h"
#include "KataGraphNodeBase.h"
#include "KataNode.h"
#include "KataSubGraphPortNode.h"
#include "KataSubGraphNode.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/UObjectHash.h"

namespace
{
	/**
	 * 그래프 에셋을 Outer로 두지만 그래프에서 더 이상 닿지 않는 노드·엣지를 Transient 패키지로 옮긴다.
	 *
	 * 편집기에서 노드를 지우면 편집기 노드만 사라지고 런타임 노드·엣지는 Undo 기록에 붙잡혀 패키지에 남는다.
	 * 저장은 닿는 오브젝트만 직렬화하지만 현지화 텍스트 수집은 패키지 안의 오브젝트를 모두 훑기 때문에,
	 * 남은 오브젝트의 FText(NodeTitle 등)가 수집 단계에서만 직렬화되어 "Unexpected custom version" 오류로 저장이 실패한다.
	 * Undo로 되살린 노드는 다음 재구성에서 그래프 밑으로 다시 옮겨지므로 여기서 옮겨도 복구된다.
	 */
	void EvictUnreachableGraphObjects(UKataGraphBase* Graph, const TArray<TObjectPtr<UEdGraphNode>>& EdNodes)
	{
		// 저장과 같은 기준으로, 그래프에서 직렬화 참조로 닿는 직속 하위 오브젝트를 모은다.
		TArray<UObject*> Reachable;
		FReferenceFinder Finder(Reachable, Graph, true, true, true, true);
		Finder.FindReferences(Graph);

		TSet<const UObject*> Live(Reachable);

		// 편집기 노드는 그래프 직속이 아니라 위 탐색에서 따라가지 않는다. 화면에 남은 노드는 지우지 않도록 직접 넣는다.
		for (const TObjectPtr<UEdGraphNode>& EdGraphNode : EdNodes)
		{
			if (const UKataEdNode* EdNode = Cast<UKataEdNode>(EdGraphNode))
			{
				Live.Add(EdNode->KataNode);
			}
			else if (const UKataEdNodeEdge* EdEdge = Cast<UKataEdNodeEdge>(EdGraphNode))
			{
				Live.Add(EdEdge->KataEdge);
			}
		}

		TArray<UObject*> Children;
		GetObjectsWithOuter(Graph, Children, EGetObjectsFlags::None);
		for (UObject* Child : Children)
		{
			const bool bGraphElement = Child->IsA<UKataGraphNodeBase>() || Child->IsA<UKataGraphEdgeBase>();
			if (bGraphElement && !Live.Contains(Child))
			{
				Child->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_DoNotDirty);
			}
		}
	}
}

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

			// 디버거가 실행 노드를 이 저작 노드로 되짚는다. 이 페이지의 노드이므로 SubGraph 경로는 비운다.
			GraphNode->DebugSourceNodeGuid = EdNode->NodeGuid;
			GraphNode->DebugSubGraphPath.Reset();

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
						// 병렬 엣지는 위상 관계를 중복시키지 않는다. 실제 엣지 목록은 아래에서 모두 보관한다.
						GraphNode->ChildrenNodes.AddUnique(ChildNode);

						ChildNode->ParentNodes.AddUnique(GraphNode);
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

	// 별칭의 실행용 출발지는 저장할 때마다 편집한 목록에서 새로 만든다. 편집한 목록에 들어 있는
	// SubGraph Port Out은 여기서 빼고, 그 자리는 펼칠 때 사본으로 채운다.
	for (const TObjectPtr<UKataGraphNodeBase>& Node : Graph->AllNodes)
	{
		UKataAliasNode* Alias = Cast<UKataAliasNode>(Node);
		if (Alias == nullptr)
		{
			continue;
		}
		Alias->ResolvedSourceNodes.Reset();
		for (const TObjectPtr<UKataNode>& Source : Alias->SourceNodes.Nodes)
		{
			if (Source != nullptr && !Source->IsA<UKataSubGraphPortNode>())
			{
				Alias->ResolvedSourceNodes.Add(Source);
			}
		}
	}

	// 포트를 녹이는 단계는 위 구조가 완성된 뒤에 돈다. 사본은 편집기 노드가 없어 NodeMap에 들어가지
	// 않으므로 RootNodes 정렬보다 앞서면 정렬이 사본을 찾지 못한다.
	FlattenSubGraphs();

	// 펼침까지 끝나야 사본을 포함한 최종 구조가 정해지므로 정리는 마지막에 한다.
	EvictUnreachableGraphObjects(Graph, Nodes);
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

	// ClearGraph는 이전 SubGraph 실행 사본의 연결까지 지운다. PIE 중 저장하면 실행 중인 인스턴스가
	// 연결이 끊긴 사본에 남으므로, 목록만 비우고 연결은 아래에서 저작 노드만 초기화한다.
	// 이전 사본은 재구성 끝에 Transient 패키지로 옮겨지고 붙잡는 인스턴스가 없으면 GC가 회수한다.
	Graph->AllNodes.Reset();
	Graph->RootNodes.Reset();
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

namespace
{
	/** 복사한 노드와 그 노드가 가진 엣지를 모두 대상 그래프 소유로 옮긴다. */
	void AdoptCopiedNode(UKataGraphNodeBase* Node, UKataGraphBase* Graph)
	{
		Node->Graph = Graph;
		Node->Rename(nullptr, Graph, REN_DontCreateRedirectors | REN_DoNotDirty);

        // 실행 사본의 별칭에는 해석된 목록만 필요하다. 복제한 저작 포트로 향하는 참조를 남기지 않는다.
        if (UKataAliasNode* Alias = Cast<UKataAliasNode>(Node))
        {
            Alias->SourceNodes.Nodes.Reset();
        }

		for (TPair<TObjectPtr<UKataGraphNodeBase>, FKataGraphEdgeList>& EdgePair : Node->Edges)
		{
			for (TObjectPtr<UKataGraphEdgeBase>& Edge : EdgePair.Value.Edges)
			{
				if (Edge == nullptr)
				{
					continue;
				}
				Edge->Graph = Graph;
				Edge->Rename(nullptr, Graph, REN_DontCreateRedirectors | REN_DoNotDirty);
			}
		}
	}

	/** 노드 사이의 연결을 양쪽에서 끊는다. 엣지 객체는 호출한 쪽이 옮기거나 버린다. */
	void DisconnectNodes(UKataGraphNodeBase* Parent, UKataGraphNodeBase* Child)
	{
		Parent->ChildrenNodes.Remove(Child);
		Parent->Edges.Remove(Child);
		Child->ParentNodes.Remove(Parent);
	}
}

void UKataEdGraph::FlattenSubGraphs()
{
	UKataGraphBase* Graph = GetKataGraph();

	// 같은 서브그래프를 가리키는 포트는 사본 하나를 공유한다.
	TMap<UKataGraph*, TArray<UKataSubGraphPortNode*>> PortsBySubGraph;
    TArray<UKataGraphNodeBase*> ConsumedPorts;
	for (const TObjectPtr<UKataGraphNodeBase>& Node : Graph->AllNodes)
	{
		UKataSubGraphPortNode* Port = Cast<UKataSubGraphPortNode>(Node);
		if (Port == nullptr)
		{
			continue;
		}
        ConsumedPorts.AddUnique(Port);
        UKataGraph* Source = Port->GetReferencedSubGraph();
		if (Source == nullptr)
		{
			LOG_WARNING(TEXT("UKataEdGraph::FlattenSubGraphs omitted an unassigned or invalid SubGraph port."));
			continue;
		}
		if (Source == Graph)
		{
			LOG_ERROR(TEXT("UKataEdGraph::FlattenSubGraphs refused a port that references its own graph."));
			continue;
		}
		PortsBySubGraph.FindOrAdd(Source).Add(Port);
	}

	for (const TPair<UKataGraph*, TArray<UKataSubGraphPortNode*>>& Pair : PortsBySubGraph)
	{
		// 그래프 객체를 통째로 복제한다. 노드와 엣지가 모두 그 아래 소유돼 있어 서로를 가리키는
		// 참조가 사본끼리 자동으로 이어진다. 노드와 엣지를 따로 복사하면 그 연결을 손으로 다시 맺어야 한다.
		UKataGraphBase* Copied = DuplicateObject<UKataGraphBase>(Pair.Key, Graph);
		if (Copied == nullptr)
		{
			LOG_ERROR(TEXT("UKataEdGraph::FlattenSubGraphs failed to duplicate a SubGraph."));
			continue;
		}

		const FGuid SourcePageId = KataGraphDebugIds::GetPageId(Pair.Key);
		TArray<UKataGraphNodeBase*> CopiedEntries;
		TArray<TObjectPtr<UKataNode>> CopiedSources;
		for (const TObjectPtr<UKataGraphNodeBase>& Node : Copied->AllNodes)
		{
			if (Node == nullptr)
			{
				continue;
			}
			AdoptCopiedNode(Node, Graph);
			Graph->AllNodes.Add(Node);
			// 원본 노드의 출처는 복제로 유지된다. 이 그래프에서 보면 원본 SubGraph를 한 단계 더 거친다.
			Node->DebugSubGraphPath.Insert(SourcePageId, 0);

            if (UKataEntryNode* Entry = Cast<UKataEntryNode>(Node))
			{
                // 원본 그래프의 진입점만 연결해 이미 펼쳐진 하위 그래프의 진입점으로 건너뛰지 않는다.
                if (!Entry->bIsSubGraphEntry)
                {
                    CopiedEntries.Add(Entry);
                }
                // 부모 시작점 탐색에서는 제외하고 들어오는 전이로 도달했을 때 조건을 평가한다.
                Entry->bIsSubGraphEntry = true;
			}
			if (UKataNode* KataNode = Cast<UKataNode>(Node))
			{
				if (KataNode->IsExecutableState())
				{
					CopiedSources.Add(KataNode);
				}
			}
		}

		// Any State는 실행 시 그래프 전체의 실행 가능 노드를 덮는다. 펼친 뒤에는 부모 노드와 다른 SubGraph
		// 사본까지 한 그래프에 섞이므로, SubGraph 안의 Any State 사본은 이 사본의 노드 목록으로 한정한다.
		// 사본만 바꾸므로 원본 페이지의 설정은 유지된다. 중첩 사본은 CopiedSources에 포함되어 함께 덮인다.
		for (const TObjectPtr<UKataGraphNodeBase>& Node : Copied->AllNodes)
		{
			UKataAliasNode* CopiedAlias = Cast<UKataAliasNode>(Node);
			if (CopiedAlias != nullptr && CopiedAlias->bAnyState)
			{
				CopiedAlias->bAnyState = false;
				CopiedAlias->ResolvedSourceNodes = CopiedSources;
			}
		}

		// 펼친 결과는 편집기 화면에 드러나지 않는다. 저장할 때마다 눈으로 확인할 수 있도록 남긴다.
		LOG_INFO(TEXT("UKataEdGraph::FlattenSubGraphs expanded %d node(s) from '%s' for %d port(s)."),
			Copied->AllNodes.Num(), *Pair.Key->GetName(), Pair.Value.Num());

		// 노드를 모두 옮겼으므로 복제한 껍데기를 치운다. ClearGraph는 옮긴 노드의 연결까지 지우므로 쓰지 않는다.
		// 껍데기를 그대로 두면 서브그래프의 편집기 그래프까지 함께 복제된 채로 남는다. 그 안의 편집기
		// 노드가 옮긴 노드를 계속 가리켜, 저장할 때마다 에셋에 쌓이고 에셋을 지울 때 내부 참조로 걸린다.
		Copied->AllNodes.Reset();
		Copied->RootNodes.Reset();
#if WITH_EDITORONLY_DATA
        if (UKataGraph* CopiedKataGraph = Cast<UKataGraph>(Copied))
        {
            for (UKataGraph* CopiedSource : CopiedKataGraph->EmbeddedSubGraphs)
            {
                if (CopiedSource != nullptr)
                {
                    // 전체 복제에 따라온 저작 원본만 버린다. 부모에 옮긴 실행 노드는 유지한다.
                    CopiedSource->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_DoNotDirty);
                    CopiedSource->MarkAsGarbage();
                }
            }
            CopiedKataGraph->EmbeddedSubGraphs.Reset();
        }
		if (UEdGraph* CopiedEdGraph = Copied->EdGraph)
		{
			Copied->EdGraph = nullptr;
			CopiedEdGraph->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_DoNotDirty);
			CopiedEdGraph->MarkAsGarbage();
		}
#endif
		Copied->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_DoNotDirty);
		Copied->MarkAsGarbage();

		for (UKataSubGraphPortNode* Port : Pair.Value)
		{
            const bool bCombinedPort = Port->IsA<UKataSubGraphNode>();
			if (Port->IsA<UKataSubGraphPortInNode>() || bCombinedPort)
			{
				if (CopiedEntries.IsEmpty())
				{
					LOG_ERROR(TEXT("UKataEdGraph::FlattenSubGraphs found no entry node in a SubGraph used by a Port In."));
					continue;
				}

				// 들어오던 엣지를 진입 노드로 넘긴다. 진입 노드는 머무를 수 없으므로 전이 해석이
				// 그대로 통과해 서브그래프 안의 액션까지 내려간다.
				TArray<TObjectPtr<UKataGraphNodeBase>> Parents = Port->ParentNodes;
				for (const TObjectPtr<UKataGraphNodeBase>& Parent : Parents)
				{
					TArray<UKataGraphEdgeBase*> IncomingEdges;
					Parent->GetEdgesTo(Port, IncomingEdges);
					DisconnectNodes(Parent, Port);

					for (UKataGraphEdgeBase* Edge : IncomingEdges)
					{
						// 진입 노드가 여럿이면 엣지를 입구마다 하나씩 둔다. 어느 쪽으로 갈지는
						// 각 진입 노드의 조건과 엣지 우선순위가 실행 중에 가른다.
						for (int32 EntryIndex = 0; EntryIndex < CopiedEntries.Num(); ++EntryIndex)
						{
							UKataGraphEdgeBase* TargetEdge = Edge;
							if (EntryIndex > 0)
							{
								TargetEdge = DuplicateObject<UKataGraphEdgeBase>(Edge, Graph);
								TargetEdge->Graph = Graph;
							}
							TargetEdge->StartNode = Parent;
							TargetEdge->EndNode = CopiedEntries[EntryIndex];

							Parent->ChildrenNodes.AddUnique(CopiedEntries[EntryIndex]);
							CopiedEntries[EntryIndex]->ParentNodes.AddUnique(Parent);
							Parent->AddEdge(CopiedEntries[EntryIndex], TargetEdge);
						}
					}
				}
			}
            if (Port->IsA<UKataSubGraphPortOutNode>() || bCombinedPort)
			{
				if (CopiedSources.IsEmpty())
				{
					LOG_ERROR(TEXT("UKataEdGraph::FlattenSubGraphs found no executable node in a SubGraph used by a Port Out."));
					continue;
				}

				// 편집한 별칭이 이 포트를 담고 있으면 실행용 목록에만 사본을 더한다.
				// 편집한 목록을 고치면 다음 저장에서 포트 지정이 사라지고 버려진 사본을 가리키게 된다.
				for (const TObjectPtr<UKataGraphNodeBase>& Node : Graph->AllNodes)
				{
					UKataAliasNode* UserAlias = Cast<UKataAliasNode>(Node);
					if (UserAlias == nullptr)
					{
						continue;
					}
					const bool bReferencesPort = UserAlias->SourceNodes.Nodes.ContainsByPredicate(
						[Port](const TObjectPtr<UKataNode>& Source) { return Source.Get() == Port; });
					if (bReferencesPort)
					{
						UserAlias->ResolvedSourceNodes.Append(CopiedSources);
					}
				}

				// 나가는 엣지가 없으면 옮길 것이 없다. 아무 일도 하지 않는 별칭을 만들지 않는다.
				if (Port->ChildrenNodes.IsEmpty())
				{
					ConsumedPorts.AddUnique(Port);
					continue;
				}

				// 나가는 엣지를 서브그래프 사본 전체를 출발지로 삼는 별칭으로 옮긴다.
				UKataAliasNode* Alias = NewObject<UKataAliasNode>(Graph);
				Alias->Graph = Graph;
				Alias->ResolvedSourceNodes = CopiedSources;
				Graph->AllNodes.Add(Alias);

				TArray<TObjectPtr<UKataGraphNodeBase>> Children = Port->ChildrenNodes;
				for (const TObjectPtr<UKataGraphNodeBase>& Child : Children)
				{
					TArray<UKataGraphEdgeBase*> OutgoingEdges;
					Port->GetEdgesTo(Child, OutgoingEdges);
					DisconnectNodes(Port, Child);

					for (UKataGraphEdgeBase* Edge : OutgoingEdges)
					{
						Edge->StartNode = Alias;
						Edge->EndNode = Child;
						Alias->ChildrenNodes.AddUnique(Child);
						Child->ParentNodes.AddUnique(Alias);
						Alias->AddEdge(Child, Edge);
					}
				}
			}

			ConsumedPorts.AddUnique(Port);
		}
	}

	// 포트는 그래프 편집과 서브그래프 펼침에만 쓰인다. 실행하는 그래프에는 남기지 않는다.
	for (UKataGraphNodeBase* Port : ConsumedPorts)
	{
        // 미선택·진입점 부족 포트도 제거해 내부를 건너뛰고 다음 액션으로 가는 경로를 막는다.
        const TArray<TObjectPtr<UKataGraphNodeBase>> Parents = Port->ParentNodes;
        for (UKataGraphNodeBase* Parent : Parents)
        {
            DisconnectNodes(Parent, Port);
        }
        const TArray<TObjectPtr<UKataGraphNodeBase>> Children = Port->ChildrenNodes;
        for (UKataGraphNodeBase* Child : Children)
        {
            DisconnectNodes(Port, Child);
        }
		Graph->AllNodes.Remove(Port);
		Graph->RootNodes.Remove(Port);
	}
}
