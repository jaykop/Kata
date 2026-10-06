#include "KataEmbeddedSubGraphEditor.h"

#include "KataEdGraph.h"
#include "KataEdNode.h"
#include "KataEdNodeEdge.h"
#include "KataAliasNode.h"
#include "KataEntryNode.h"
#include "KataGraph.h"
#include "KataGraphEdgeBase.h"
#include "KataGraphSchema.h"
#include "KataSubGraphPortNode.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "UObject/Package.h"
#include "UObject/UObjectHash.h"
#include "UObject/UObjectGlobals.h"

#define LOCTEXT_NAMESPACE "KataEmbeddedSubGraphEditor"

UKataGraph* KataEmbeddedSubGraphEditor::Create(UKataGraph* Owner)
{
    if (Owner == nullptr || Owner->GetRootGraph() == nullptr)
    {
        return nullptr;
    }

    FString Name(TEXT("SubGraph"));
    int32 Suffix = 2;
    while (Owner->EmbeddedSubGraphs.ContainsByPredicate([&Name](const TObjectPtr<UKataGraph>& Existing)
        {
            return Existing != nullptr && Existing->GetGraphDisplayName().ToString().Equals(Name, ESearchCase::IgnoreCase);
        }))
    {
        Name = FString::Printf(TEXT("SubGraph %d"), Suffix++);
    }

    Owner->Modify();
    UKataGraph* Graph = NewObject<UKataGraph>(Owner, NAME_None, RF_Transactional);
    Graph->Modify();
    Graph->GraphDisplayName = FText::FromString(Name);
    Graph->EdGraph = FBlueprintEditorUtils::CreateNewGraph(Graph, NAME_None,
        UKataEdGraph::StaticClass(), UKataGraphSchema::StaticClass());
    Graph->EdGraph->SetFlags(RF_Transactional);
    Graph->EdGraph->bAllowDeletion = false;
    Graph->EdGraph->GetSchema()->CreateDefaultNodesForGraph(*Graph->EdGraph);
    Owner->EmbeddedSubGraphs.Add(Graph);
    return Graph;
}

namespace
{
    bool CanCopyTree(const UKataGraph* Graph, TSet<const UKataGraph*>& Visited)
    {
        if (Graph == nullptr || Graph->EdGraph == nullptr || Visited.Contains(Graph))
        {
            return false;
        }
        Visited.Add(Graph);
        for (const UKataGraph* Child : Graph->EmbeddedSubGraphs)
        {
            if (!Graph->OwnsEmbeddedSubGraph(Child) || !CanCopyTree(Child, Visited))
            {
                return false;
            }
        }
        return true;
    }

    // 전체 소유 트리를 한 번 복제해 내부 참조의 재매핑을 유지한 뒤 각 페이지의 실행 사본만 비운다.
    void NormalizeCopy(UKataGraph* Copy)
    {
        Copy->ClearFlags(RF_Public | RF_Standalone);
        Copy->CompiledGeneration.Invalidate();
        Copy->CompiledDependencies.Reset();
        for (UKataGraph* Child : Copy->EmbeddedSubGraphs)
        {
            NormalizeCopy(Child);
        }
        Copy->AllNodes.Reset();
        Copy->RootNodes.Reset();
        if (UKataEdGraph* EdGraph = Cast<UKataEdGraph>(Copy->EdGraph))
        {
            EdGraph->NodeMap.Reset();
            EdGraph->EdgeMap.Reset();
        }
        TSet<UObject*> AuthoredObjects;
        TSet<UKataNode*> AuthoredNodes;
        for (UEdGraphNode* Node : Copy->EdGraph->Nodes)
        {
            if (UKataEdNode* EdNode = Cast<UKataEdNode>(Node))
            {
                EdNode->ClipboardSubGraph = nullptr;
                EdNode->ClipboardSubGraphSource.Reset();
                if (UKataGraphNodeBase* KataNode = EdNode->KataNode)
                {
                    AuthoredObjects.Add(KataNode);
                    if (UKataNode* TypedNode = Cast<UKataNode>(KataNode))
                    {
                        AuthoredNodes.Add(TypedNode);
                    }
                    KataNode->Graph = Copy;
                    KataNode->ParentNodes.Reset();
                    KataNode->ChildrenNodes.Reset();
                    KataNode->Edges.Reset();
                    if (UKataEntryNode* Entry = Cast<UKataEntryNode>(KataNode))
                    {
                        Entry->bIsSubGraphEntry = false;
                    }
                    if (UKataAliasNode* Alias = Cast<UKataAliasNode>(KataNode))
                    {
                        Alias->ResolvedSourceNodes.Reset();
                    }
                }
            }
            else if (UKataEdNodeEdge* EdEdge = Cast<UKataEdNodeEdge>(Node))
            {
                if (UKataGraphEdgeBase* Edge = EdEdge->KataEdge)
                {
                    AuthoredObjects.Add(Edge);
                    Edge->Graph = Copy;
                    const UKataEdNode* Start = EdEdge->GetStartNode();
                    const UKataEdNode* End = EdEdge->GetEndNode();
                    Edge->StartNode = Start != nullptr ? Start->KataNode : nullptr;
                    Edge->EndNode = End != nullptr ? End->KataNode : nullptr;
                }
            }
        }
        for (UKataNode* Node : AuthoredNodes)
        {
            if (UKataAliasNode* Alias = Cast<UKataAliasNode>(Node))
            {
                Alias->SourceNodes.Nodes.RemoveAll([&AuthoredNodes](const TObjectPtr<UKataNode>& SourceNode)
                {
                    return !AuthoredNodes.Contains(SourceNode.Get());
                });
            }
        }
        // 저장으로 생성된 실행 사본은 클립보드에서 제외하고 패키지의 텍스트 수집 대상에서도 빼낸다.
        TArray<UObject*> Children;
        GetObjectsWithOuter(Copy, Children, EGetObjectsFlags::None);
        for (UObject* Child : Children)
        {
            if ((Child->IsA<UKataGraphNodeBase>() || Child->IsA<UKataGraphEdgeBase>()) && !AuthoredObjects.Contains(Child))
            {
                Child->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_DoNotDirty);
            }
        }
    }
}

UKataGraph* KataEmbeddedSubGraphEditor::CopyForClipboard(UKataGraph* Source, UObject* Outer)
{
    TSet<const UKataGraph*> Visited;
    if (Outer == nullptr || !CanCopyTree(Source, Visited))
    {
        return nullptr;
    }
    UKataGraph* Copy = DuplicateObject<UKataGraph>(Source, Outer);
    NormalizeCopy(Copy);
    return Copy;
}

UKataGraph* KataEmbeddedSubGraphEditor::PasteCopy(UKataGraph* Owner, UKataGraph* ClipboardGraph)
{
    if (Owner == nullptr || Owner->GetRootGraph() == nullptr)
    {
        return nullptr;
    }
    UKataGraph* Copy = CopyForClipboard(ClipboardGraph, Owner);
    if (Copy == nullptr)
    {
        return nullptr;
    }
    FString BaseName = ClipboardGraph->GraphDisplayName.ToString().TrimStartAndEnd();
    if (BaseName.IsEmpty())
    {
        BaseName = TEXT("SubGraph");
    }
    BaseName += TEXT(" Copy");
    FString Name = BaseName;
    int32 Suffix = 2;
    while (Owner->EmbeddedSubGraphs.ContainsByPredicate([&Name](const TObjectPtr<UKataGraph>& Existing)
        {
            return Existing != nullptr && Existing->GetGraphDisplayName().ToString().Equals(Name, ESearchCase::IgnoreCase);
        }))
    {
        Name = FString::Printf(TEXT("%s %d"), *BaseName, Suffix++);
    }
    Owner->Modify();
    TArray<UObject*> Objects;
    GetObjectsWithOuter(Copy, Objects, EGetObjectsFlags::IncludeNestedObjects);
    Objects.Add(Copy);
    for (UObject* Object : Objects)
    {
        Object->SetFlags(RF_Transactional);
        Object->Modify();
    }
    Copy->GraphDisplayName = FText::FromString(Name);
    for (UObject* Object : Objects)
    {
        UEdGraphNode* Node = Cast<UEdGraphNode>(Object);
        if (Node == nullptr)
        {
            continue;
        }
        Node->CreateNewGuid();
        const UKataEdNode* EdNode = Cast<UKataEdNode>(Node);
        UKataSubGraphPortNode* Port = EdNode != nullptr ? Cast<UKataSubGraphPortNode>(EdNode->KataNode) : nullptr;
        if (Port != nullptr)
        {
            if (Port->bUseEmbeddedSubGraph)
            {
                Port->SubGraph = nullptr;
                if (Port->GetReferencedSubGraph() == nullptr)
                {
                    Port->EmbeddedSubGraph.Graph = nullptr;
                }
            }
            else
            {
                Port->EmbeddedSubGraph.Graph = nullptr;
                if (Port->GetReferencedSubGraph() == nullptr)
                {
                    Port->SubGraph = nullptr;
                }
            }
        }
    }
    Owner->EmbeddedSubGraphs.Add(Copy);
    return Copy;
}

bool KataEmbeddedSubGraphEditor::ValidateName(const UKataGraph* Owner, const UKataGraph* Graph,
    const FText& Name, FText& OutError)
{
    OutError = FText::GetEmpty();
    if (Owner == nullptr || !Owner->OwnsEmbeddedSubGraph(Graph))
    {
        OutError = LOCTEXT("InvalidOwner", "This subgraph is no longer owned by this graph page.");
        return false;
    }
    const FString TrimmedName = Name.ToString().TrimStartAndEnd();
    if (TrimmedName.IsEmpty())
    {
        OutError = LOCTEXT("EmptyName", "Enter a subgraph name.");
        return false;
    }
    for (const UKataGraph* Other : Owner->EmbeddedSubGraphs)
    {
        if (Other != nullptr && Other != Graph && Other->GetGraphDisplayName().ToString().Equals(TrimmedName, ESearchCase::IgnoreCase))
        {
            OutError = LOCTEXT("DuplicateName", "Another subgraph already uses this name.");
            return false;
        }
    }
    return true;
}

bool KataEmbeddedSubGraphEditor::Rename(UKataGraph* Owner, UKataGraph* Graph, const FText& Name, FText& OutError)
{
    if (!ValidateName(Owner, Graph, Name, OutError))
    {
        return false;
    }
    const FString TrimmedName = Name.ToString().TrimStartAndEnd();
    if (Graph->GraphDisplayName.ToString() != TrimmedName)
    {
        Graph->Modify();
        Graph->GraphDisplayName = FText::FromString(TrimmedName);
    }
    return true;
}

bool KataEmbeddedSubGraphEditor::RemoveUnused(UKataGraph* Owner)
{
    if (Owner == nullptr || Owner->EdGraph == nullptr)
    {
        return false;
    }

    // 평탄화한 AllNodes와 삭제 노드의 Undo 참조는 현재 저작 사용처로 세지 않는다.
    TSet<UKataGraph*> ReferencedGraphs;
    for (UEdGraphNode* Node : Owner->EdGraph->Nodes)
    {
        const UKataEdNode* EdNode = Cast<UKataEdNode>(Node);
        const UKataSubGraphPortNode* Port = EdNode != nullptr ? Cast<UKataSubGraphPortNode>(EdNode->KataNode) : nullptr;
        if (Port != nullptr && Port->bUseEmbeddedSubGraph && Port->EmbeddedSubGraph.Graph != nullptr)
        {
            ReferencedGraphs.Add(Port->EmbeddedSubGraph.Graph);
        }
    }

    TArray<UKataGraph*> UnusedGraphs;
    for (UKataGraph* Graph : Owner->EmbeddedSubGraphs)
    {
        if (Owner->OwnsEmbeddedSubGraph(Graph) && !ReferencedGraphs.Contains(Graph))
        {
            UnusedGraphs.Add(Graph);
        }
    }
    if (UnusedGraphs.IsEmpty())
    {
        return false;
    }

    Owner->Modify();
    for (UKataGraph* Graph : UnusedGraphs)
    {
        Graph->Modify();
        Owner->EmbeddedSubGraphs.Remove(Graph);
        // 패키지의 텍스트 수집에서 제외하되 Undo가 내부 노드까지 되살릴 수 있도록 남긴다.
        Graph->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_DoNotDirty);
    }
    return true;
}

#undef LOCTEXT_NAMESPACE
