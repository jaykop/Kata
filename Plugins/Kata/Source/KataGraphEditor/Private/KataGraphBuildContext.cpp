#include "KataGraphBuildContext.h"

#include "KataEdGraph.h"
#include "KataEdNode.h"
#include "KataEntryNode.h"
#include "KataGraph.h"
#include "KataGraphEditorPrivate.h"
#include "KataSubGraphPortNode.h"
#include "KataSubGraphNode.h"
#include "UObject/Package.h"

#define LOCTEXT_NAMESPACE "KataGraphBuildContext"

namespace
{
    struct FKataGraphBuildTraversal
    {
        UKataGraphBase* Root = nullptr;
        TSet<UKataGraphBase*> Visited;
        TArray<UKataGraphBase*> Visiting;
        TArray<UKataEdGraph*> BuildOrder;
        FText Error;
        TMap<UKataGraph*, FGuid> ExternalGenerations;

        bool IsLocal(const UKataGraphBase* Graph) const
        {
            const UKataGraph* KataRoot = Cast<UKataGraph>(Root);
            return Graph == Root || (KataRoot != nullptr && KataRoot->ContainsGraph(Cast<UKataGraph>(Graph)));
        }

        bool Visit(UKataGraphBase* Graph)
        {
            if (Visiting.Contains(Graph))
            {
                Error = FText::Format(LOCTEXT("Cycle", "SubGraph reference cycle at '{0}'."), FText::FromString(Graph->GetName()));
                return false;
            }
            if (Visited.Contains(Graph))
            {
                return true;
            }

            UKataEdGraph* EdGraph = Cast<UKataEdGraph>(Graph->EdGraph);
            if (EdGraph == nullptr)
            {
                Error = FText::Format(LOCTEXT("NoAuthoring", "Open and save '{0}' first: authoring data is missing."), FText::FromString(Graph->GetName()));
                return false;
            }
            Visiting.Add(Graph);

            if (UKataGraph* KataGraph = Cast<UKataGraph>(Graph))
            {
                for (UKataGraph* Embedded : KataGraph->EmbeddedSubGraphs)
                {
                    if (!KataGraph->OwnsEmbeddedSubGraph(Embedded))
                    {
                        Error = LOCTEXT("InvalidOwner", "An embedded SubGraph has an invalid owner.");
                        return false;
                    }
                    if (!Visit(Embedded))
                    {
                        return false;
                    }
                }
            }

            // 평탄화 결과에는 포트가 없으므로 의존 검사는 항상 저작 노드를 읽는다.
            for (UEdGraphNode* Node : EdGraph->Nodes)
            {
                const UKataEdNode* EdNode = Cast<UKataEdNode>(Node);
                const UKataSubGraphPortNode* Port = EdNode != nullptr ? Cast<UKataSubGraphPortNode>(EdNode->KataNode) : nullptr;
                if (Port == nullptr)
                {
                    continue;
                }
                UKataGraph* Source = Port->GetReferencedSubGraph();
                if (Source == nullptr)
                {
                    const bool bHasAssignment = Port->bUseEmbeddedSubGraph
                        ? Port->EmbeddedSubGraph.Graph != nullptr : Port->SubGraph != nullptr;
                    if (bHasAssignment)
                    {
                        Error = LOCTEXT("InvalidReference", "A SubGraph assignment is invalid. Fix its reference before saving.");
                        return false;
                    }
                    // 미선택 포트는 미완성 저작으로 남길 수 있다. 다른 경로의 저장을 막지 않는다.
                    continue;
                }
                if (!Visit(Source))
                {
                    return false;
                }
                if (Port->IsA<UKataSubGraphPortInNode>() || Port->IsA<UKataSubGraphNode>())
                {
                    const bool bHasEntry = Source->EdGraph->Nodes.ContainsByPredicate([](const UEdGraphNode* SourceNode)
                    {
                        const UKataEdNode* SourceEdNode = Cast<UKataEdNode>(SourceNode);
                        return SourceEdNode != nullptr && SourceEdNode->KataNode != nullptr
                            && SourceEdNode->KataNode->IsA<UKataEntryNode>();
                    });
                    if (!bHasEntry)
                    {
                        Error = FText::Format(LOCTEXT("NoEntry", "SubGraph '{0}' needs an Entry node."), Source->GetGraphDisplayName());
                        return false;
                    }
                }
            }

            Visiting.Pop();
            Visited.Add(Graph);
            if (IsLocal(Graph))
            {
                BuildOrder.Add(EdGraph);
            }
            else if (Graph->AllNodes.IsEmpty() && !EdGraph->Nodes.IsEmpty())
            {
                Error = FText::Format(LOCTEXT("NoRuntime", "Save external graph '{0}' first: compiled data is missing."), FText::FromString(Graph->GetName()));
                return false;
            }
            if (!IsLocal(Graph))
            {
                UKataGraph* External = Cast<UKataGraph>(Graph);
                if (External != nullptr && External->IsAsset())
                {
                    if (External->GetOutermost()->IsDirty() || !External->CompiledGeneration.IsValid())
                    {
                        Error = FText::Format(LOCTEXT("SaveExternal", "Save external graph '{0}' first."), External->GetGraphDisplayName());
                        return false;
                    }
                    ExternalGenerations.Add(External, External->CompiledGeneration);
                }
            }
            return true;
        }

        void CollectExternal(UKataGraph* Graph, TSet<UKataGraph*>& Seen, TMap<UKataGraph*, FGuid>& Result) const
        {
            if (Graph == nullptr || Seen.Contains(Graph))
            {
                return;
            }
            Seen.Add(Graph);
            for (UKataGraph* Child : Graph->EmbeddedSubGraphs)
            {
                CollectExternal(Child, Seen, Result);
            }
            if (Graph->EdGraph == nullptr)
            {
                return;
            }
            for (UEdGraphNode* Node : Graph->EdGraph->Nodes)
            {
                const UKataEdNode* EdNode = Cast<UKataEdNode>(Node);
                const UKataSubGraphPortNode* Port = EdNode != nullptr ? Cast<UKataSubGraphPortNode>(EdNode->KataNode) : nullptr;
                UKataGraph* Source = Port != nullptr ? Port->GetReferencedSubGraph() : nullptr;
                if (Source != nullptr && !Port->bUseEmbeddedSubGraph)
                {
                    Result.Add(Source, Source->CompiledGeneration);
                }
                CollectExternal(Source, Seen, Result);
            }
        }

        bool MatchesSavedDependencies(const UKataGraph* Graph, const TMap<UKataGraph*, FGuid>& Current) const
        {
            if (Graph->CompiledDependencies.Num() != Current.Num())
            {
                return false;
            }
            for (const FKataGraphDependencyGeneration& Dependency : Graph->CompiledDependencies)
            {
                const FGuid* Generation = Current.Find(Dependency.Graph.Get());
                if (Generation == nullptr || *Generation != Dependency.Generation)
                {
                    return false;
                }
            }
            return true;
        }

        bool Prepare()
        {
            if (Root == nullptr || !Visit(Root))
            {
                return false;
            }
            // 외장 B가 C의 예전 사본을 담고 있으면 최종 부모 A도 이를 최신으로 받아들이지 않는다.
            for (const TPair<UKataGraph*, FGuid>& External : ExternalGenerations)
            {
                TSet<UKataGraph*> Seen;
                TMap<UKataGraph*, FGuid> Current;
                CollectExternal(External.Key, Seen, Current);
                if (!MatchesSavedDependencies(External.Key, Current))
                {
                    Error = FText::Format(LOCTEXT("StaleExternal", "Save external graph '{0}' after its dependencies."), External.Key->GetGraphDisplayName());
                    return false;
                }
            }
            return true;
        }
    };
}

bool FKataGraphBuildContext::Rebuild(UKataGraphBase* Root, bool bForSave)
{
    if (Root == nullptr)
    {
        return false;
    }
    FKataGraphBuildTraversal Traversal;
    Traversal.Root = Root;
    if (!Traversal.Prepare())
    {
        if (bForSave)
        {
            LOG_ERROR(TEXT("%s Existing runtime data was retained."), *Traversal.Error.ToString());
        }
        return false;
    }
    for (UKataEdGraph* Graph : Traversal.BuildOrder)
    {
        Graph->RebuildKataGraph();
    }
    if (bForSave)
    {
        if (UKataGraph* KataRoot = Cast<UKataGraph>(Root))
        {
            KataRoot->CompiledGeneration = FGuid::NewGuid();
            KataRoot->CompiledDependencies.Reset();
            for (const TPair<UKataGraph*, FGuid>& External : Traversal.ExternalGenerations)
            {
                FKataGraphDependencyGeneration& Dependency = KataRoot->CompiledDependencies.AddDefaulted_GetRef();
                Dependency.Graph = External.Key;
                Dependency.Generation = External.Value;
            }
        }
    }
    return true;
}

FText FKataGraphBuildContext::GetDependencyStatus(UKataGraphBase* Root, bool& bOutNeedsSave)
{
    bOutNeedsSave = false;
    FKataGraphBuildTraversal Traversal;
    Traversal.Root = Root;
    if (!Traversal.Prepare())
    {
        return Traversal.Error;
    }
    const UKataGraph* KataRoot = Cast<UKataGraph>(Root);
    if (KataRoot != nullptr && (!KataRoot->CompiledGeneration.IsValid()
        || !Traversal.MatchesSavedDependencies(KataRoot, Traversal.ExternalGenerations)))
    {
        bOutNeedsSave = true;
        return LOCTEXT("SaveParent", "SubGraph data needs updating. Save this graph.");
    }
    return FText::GetEmpty();
}

#undef LOCTEXT_NAMESPACE
