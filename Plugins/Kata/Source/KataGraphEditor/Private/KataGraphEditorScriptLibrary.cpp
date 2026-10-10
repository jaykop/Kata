#include "KataGraphEditorScriptLibrary.h"

#include "KataEdGraph.h"
#include "KataEdNode.h"
#include "KataEdNodeEdge.h"
#include "KataGraph.h"
#include "KataGraphBase.h"
#include "KataGraphEdgeBase.h"
#include "KataGraphNodeBase.h"
#include "KataGraphSchema.h"
#include "KataSubGraphPortNode.h"
#include "ScopedTransaction.h"
#include "UObject/Package.h"

#define LOCTEXT_NAMESPACE "KataGraphEditorScriptLibrary"

DEFINE_LOG_CATEGORY_STATIC(LogKataGraphEditing, Log, All);

namespace
{
    UKataEdGraph* GetEdGraph(UKataGraphBase* Graph)
    {
        return Graph != nullptr ? Cast<UKataEdGraph>(Graph->EdGraph) : nullptr;
    }

    UKataEdNode* FindEdNode(UKataEdGraph* EdGraph, const UKataGraphNodeBase* Node)
    {
        if (EdGraph == nullptr || Node == nullptr)
        {
            return nullptr;
        }
        for (UEdGraphNode* EdGraphNode : EdGraph->Nodes)
        {
            UKataEdNode* EdNode = Cast<UKataEdNode>(EdGraphNode);
            if (EdNode != nullptr && EdNode->KataNode == Node)
            {
                return EdNode;
            }
        }
        return nullptr;
    }

    /** FromNode의 출력 핀에서 ToNode로 이어지는 엣지 노드를 모두 모은다. 병렬 엣지가 있을 수 있다. */
    void CollectEdgeNodes(UKataEdNode* FromNode, const UKataEdNode* ToNode, TArray<UKataEdNodeEdge*>& OutEdges)
    {
        UEdGraphPin* OutputPin = FromNode->GetOutputPin();
        if (OutputPin == nullptr)
        {
            return;
        }
        for (UEdGraphPin* LinkedPin : OutputPin->LinkedTo)
        {
            UKataEdNodeEdge* EdgeNode = LinkedPin != nullptr ? Cast<UKataEdNodeEdge>(LinkedPin->GetOwningNode()) : nullptr;
            if (EdgeNode != nullptr && EdgeNode->GetEndNode() == ToNode)
            {
                OutEdges.Add(EdgeNode);
            }
        }
    }
}

TArray<UKataGraphNodeBase*> UKataGraphEditorScriptLibrary::GetAuthoredGraphNodes(UKataGraphBase* Graph)
{
    TArray<UKataGraphNodeBase*> Result;
    if (UKataEdGraph* EdGraph = GetEdGraph(Graph))
    {
        for (UEdGraphNode* EdGraphNode : EdGraph->Nodes)
        {
            if (const UKataEdNode* EdNode = Cast<UKataEdNode>(EdGraphNode); EdNode != nullptr && EdNode->KataNode != nullptr)
            {
                Result.Add(EdNode->KataNode);
            }
        }
    }
    return Result;
}

UKataGraphNodeBase* UKataGraphEditorScriptLibrary::AddGraphNode(UKataGraphBase* Graph, TSubclassOf<UKataGraphNodeBase> NodeClass, FVector2D Position)
{
    UKataEdGraph* EdGraph = GetEdGraph(Graph);
    if (EdGraph == nullptr || NodeClass == nullptr || NodeClass->HasAnyClassFlags(CLASS_Abstract))
    {
        UE_LOG(LogKataGraphEditing, Warning, TEXT("AddGraphNode rejected: graph '%s' has no authoring graph or node class '%s' is not placeable"),
            *GetNameSafe(Graph), *GetNameSafe(NodeClass.Get()));
        return nullptr;
    }

    // 편집기 메뉴가 노드를 제안하는 기준과 같게 거른다.
    const UKataGraphNodeBase* NodeDefaults = NodeClass.GetDefaultObject();
    const bool bMatchesNodeType = Graph->NodeType == nullptr || NodeClass->IsChildOf(Graph->NodeType);
    const bool bMatchesGraphType = NodeDefaults->CompatibleGraphType == nullptr || Graph->GetClass()->IsChildOf(NodeDefaults->CompatibleGraphType);
    if (!bMatchesNodeType || !bMatchesGraphType)
    {
        UE_LOG(LogKataGraphEditing, Warning, TEXT("AddGraphNode rejected: node class '%s' is not compatible with graph '%s'"),
            *NodeClass->GetName(), *Graph->GetName());
        return nullptr;
    }

    FKataGraphSchemaAction_NewNode Action;
    Action.NodeTemplate = NewObject<UKataEdNode>(GetTransientPackage());
    Action.NodeTemplate->KataNode = NewObject<UKataGraphNodeBase>(Action.NodeTemplate, NodeClass);
    Action.NodeTemplate->KataNode->Graph = Graph;

    // 스키마 액션이 트랜잭션, 저작 그래프 등록, 핀 생성, 서브그래프 원본 준비를 편집기와 같은 순서로 처리한다.
    const UKataEdNode* EdNode = Cast<UKataEdNode>(Action.PerformAction(EdGraph, nullptr, Position, false));
    return EdNode != nullptr ? EdNode->KataNode : nullptr;
}

UKataGraphEdgeBase* UKataGraphEditorScriptLibrary::ConnectGraphNodes(UKataGraphBase* Graph, UKataGraphNodeBase* FromNode, UKataGraphNodeBase* ToNode)
{
    UKataEdGraph* EdGraph = GetEdGraph(Graph);
    UKataEdNode* EdFrom = FindEdNode(EdGraph, FromNode);
    UKataEdNode* EdTo = FindEdNode(EdGraph, ToNode);
    if (EdFrom == nullptr || EdTo == nullptr || EdFrom->GetOutputPin() == nullptr || EdTo->GetInputPin() == nullptr)
    {
        UE_LOG(LogKataGraphEditing, Warning, TEXT("ConnectGraphNodes rejected: '%s' or '%s' is not an authored node of graph '%s'"),
            *GetNameSafe(FromNode), *GetNameSafe(ToNode), *GetNameSafe(Graph));
        return nullptr;
    }

    const UEdGraphSchema* Schema = EdGraph->GetSchema();
    const FPinConnectionResponse Response = Schema->CanCreateConnection(EdFrom->GetOutputPin(), EdTo->GetInputPin());
    if (Response.Response == CONNECT_RESPONSE_DISALLOW)
    {
        UE_LOG(LogKataGraphEditing, Warning, TEXT("ConnectGraphNodes rejected by schema: %s"), *Response.Message.ToString());
        return nullptr;
    }

    TArray<UKataEdNodeEdge*> ExistingEdges;
    CollectEdgeNodes(EdFrom, EdTo, ExistingEdges);

    const FScopedTransaction Transaction(LOCTEXT("ConnectGraphNodes", "Kata Graph Editing: Connect Nodes"));
    EdGraph->Modify();
    if (!Schema->TryCreateConnection(EdFrom->GetOutputPin(), EdTo->GetInputPin()))
    {
        UE_LOG(LogKataGraphEditing, Warning, TEXT("ConnectGraphNodes failed: '%s' -> '%s'"), *FromNode->GetName(), *ToNode->GetName());
        return nullptr;
    }

    // 엣지 그래프는 연결마다 엣지 노드가 새로 생긴다. 연결 전에 없던 것이 이번에 만든 엣지다.
    TArray<UKataEdNodeEdge*> CurrentEdges;
    CollectEdgeNodes(EdFrom, EdTo, CurrentEdges);
    for (UKataEdNodeEdge* EdgeNode : CurrentEdges)
    {
        if (!ExistingEdges.Contains(EdgeNode))
        {
            return EdgeNode->KataEdge;
        }
    }
    return nullptr;
}

bool UKataGraphEditorScriptLibrary::RemoveGraphNode(UKataGraphBase* Graph, UKataGraphNodeBase* Node)
{
    UKataEdGraph* EdGraph = GetEdGraph(Graph);
    UKataEdNode* EdNode = FindEdNode(EdGraph, Node);
    if (EdNode == nullptr || !EdNode->CanUserDeleteNode())
    {
        UE_LOG(LogKataGraphEditing, Warning, TEXT("RemoveGraphNode rejected: '%s' is not a deletable authored node of graph '%s'"),
            *GetNameSafe(Node), *GetNameSafe(Graph));
        return false;
    }

    const FScopedTransaction Transaction(LOCTEXT("RemoveGraphNode", "Kata Graph Editing: Remove Node"));
    EdGraph->Modify();
    EdNode->Modify();
    // 핀 연결을 끊으면 양 끝을 잃은 엣지 노드가 스스로 제거된다(UKataEdNodeEdge::PinConnectionListChanged).
    if (const UEdGraphSchema* Schema = EdNode->GetSchema())
    {
        Schema->BreakNodeLinks(*EdNode);
    }
    EdNode->DestroyNode();
    return true;
}

UKataGraphBase* UKataGraphEditorScriptLibrary::GetEmbeddedSubGraph(UKataGraphNodeBase* Node)
{
    const UKataSubGraphPortNode* PortNode = Cast<UKataSubGraphPortNode>(Node);
    return PortNode != nullptr && PortNode->bUseEmbeddedSubGraph ? PortNode->EmbeddedSubGraph.Graph.Get() : nullptr;
}

bool UKataGraphEditorScriptLibrary::SetEmbeddedSubGraphDisplayName(UKataGraphBase* EmbeddedGraph, const FText& DisplayName)
{
    UKataGraph* Graph = Cast<UKataGraph>(EmbeddedGraph);
    const UKataGraph* Owner = Graph != nullptr ? Cast<UKataGraph>(Graph->GetOuter()) : nullptr;
    if (Owner == nullptr || !Owner->OwnsEmbeddedSubGraph(Graph) || FText::TrimPrecedingAndTrailing(DisplayName).IsEmpty())
    {
        return false;
    }

    const FScopedTransaction Transaction(LOCTEXT("SetEmbeddedSubGraphDisplayName", "Kata Graph Editing: Rename SubGraph"));
    Graph->Modify();
    Graph->GraphDisplayName = FText::TrimPrecedingAndTrailing(DisplayName);
    return true;
}

FVector2D UKataGraphEditorScriptLibrary::GetGraphNodePosition(UKataGraphBase* Graph, UKataGraphNodeBase* Node)
{
    const UKataEdNode* EdNode = FindEdNode(GetEdGraph(Graph), Node);
    return EdNode != nullptr ? FVector2D(EdNode->NodePosX, EdNode->NodePosY) : FVector2D::ZeroVector;
}

bool UKataGraphEditorScriptLibrary::RebuildGraph(UKataGraphBase* Graph)
{
    UKataEdGraph* EdGraph = GetEdGraph(Graph);
    if (EdGraph == nullptr)
    {
        return false;
    }
    EdGraph->RebuildKataGraph();
    EdGraph->NotifyGraphChanged();
    Graph->MarkPackageDirty();
    return true;
}

#undef LOCTEXT_NAMESPACE
