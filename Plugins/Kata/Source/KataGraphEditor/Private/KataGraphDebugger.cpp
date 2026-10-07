#include "KataGraphDebugger.h"

#include "Editor.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "KataActionNode.h"
#include "KataEdGraph.h"
#include "KataEdNode.h"
#include "KataGraph.h"
#include "KataGraphBase.h"
#include "KataGraphComponent.h"
#include "KataGraphInstance.h"
#include "KataGraphNodeBase.h"
#include "KataSubGraphPortNode.h"
#include "UObject/UObjectIterator.h"

#define LOCTEXT_NAMESPACE "KataGraphDebugger"

FGuid KataGraphDebugIds::GetPageId(const UKataGraphBase* Page)
{
    return Page != nullptr ? FGuid::NewDeterministicGuid(Page->GetPathName()) : FGuid();
}

TArray<FGuid> KataGraphDebugIds::GetPagePath(const UKataGraphBase* Page)
{
    TArray<FGuid> Path;
    const UKataGraph* Graph = Cast<UKataGraph>(Page);
    while (Graph != nullptr)
    {
        const UKataGraph* Owner = Cast<UKataGraph>(Graph->GetOuter());
        if (Owner == nullptr || !Owner->OwnsEmbeddedSubGraph(Graph))
        {
            break;
        }
        Path.Insert(GetPageId(Graph), 0);
        Graph = Owner;
    }
    return Path;
}

namespace
{
    /** 실행 노드의 출처 경로가 페이지 경로로 시작하는지 확인한다. 출처가 더 깊으면 다음 단계 식별자를 준다. */
    bool MatchPagePath(const UKataGraphNodeBase* Node, const TArray<FGuid>& PagePath, bool& bOutDirect, FGuid& OutNextPage)
    {
        const TArray<FGuid>& Origin = Node->DebugSubGraphPath;
        if (Origin.Num() < PagePath.Num())
        {
            return false;
        }
        for (int32 Index = 0; Index < PagePath.Num(); ++Index)
        {
            if (Origin[Index] != PagePath[Index])
            {
                return false;
            }
        }
        bOutDirect = Origin.Num() == PagePath.Num();
        OutNextPage = bOutDirect ? FGuid() : Origin[PagePath.Num()];
        return true;
    }

    /** 루트 페이지의 저작 노드는 실행 노드와 같은 객체다. 출처가 없던 기존 저장본도 포인터로 맞춘다. */
    bool IsSourceEdNode(const UKataEdNode* EdNode, const UKataGraphNodeBase* Node)
    {
        return EdNode->KataNode == Node
            || (Node->DebugSourceNodeGuid.IsValid() && EdNode->NodeGuid == Node->DebugSourceNodeGuid);
    }

    bool IsPortTo(const UKataEdNode* EdNode, const FGuid& PageId)
    {
        const UKataSubGraphPortNode* Port = Cast<UKataSubGraphPortNode>(EdNode->KataNode);
        const UKataGraph* Source = Port != nullptr ? Port->GetReferencedSubGraph() : nullptr;
        return Source != nullptr && KataGraphDebugIds::GetPageId(Source) == PageId;
    }

    /** 이미 더 강한 강조가 있는 노드는 덮어쓰지 않는다. 열거 순서가 곧 우선순위다. */
    void ApplyHighlight(UEdGraph* EdGraph, const TArray<FGuid>& PagePath, const UKataGraphNodeBase* Node,
        EKataGraphDebugHighlight Direct, EKataGraphDebugHighlight Inside)
    {
        bool bDirect = false;
        FGuid NextPage;
        if (Node == nullptr || !MatchPagePath(Node, PagePath, bDirect, NextPage))
        {
            return;
        }
        for (UEdGraphNode* GraphNode : EdGraph->Nodes)
        {
            UKataEdNode* EdNode = Cast<UKataEdNode>(GraphNode);
            if (EdNode == nullptr || EdNode->KataNode == nullptr || EdNode->DebugHighlight != EKataGraphDebugHighlight::None)
            {
                continue;
            }
            if (bDirect ? IsSourceEdNode(EdNode, Node) : IsPortTo(EdNode, NextPage))
            {
                EdNode->DebugHighlight = bDirect ? Direct : Inside;
            }
        }
    }
}

FKataGraphDebugger::FKataGraphDebugger(UKataGraphBase* InRootGraph)
    : RootGraph(InRootGraph)
{
    EndPIEHandle = FEditorDelegates::EndPIE.AddRaw(this, &FKataGraphDebugger::HandleEndPIE);
}

FKataGraphDebugger::~FKataGraphDebugger()
{
    FEditorDelegates::EndPIE.Remove(EndPIEHandle);
    ClearHighlights(LastPage.Get());
}

void FKataGraphDebugger::GatherCandidates(TArray<UKataGraphComponent*>& OutComponents) const
{
    OutComponents.Reset();
    const UKataGraphBase* Root = RootGraph.Get();
    if (Root == nullptr)
    {
        return;
    }
    for (TObjectIterator<UKataGraphComponent> It; It; ++It)
    {
        UKataGraphComponent* Component = *It;
        if (!IsValid(Component) || Component->IsTemplate())
        {
            continue;
        }
        const UWorld* World = Component->GetWorld();
        const UKataGraphInstance* Instance = Component->GetActiveGraphInstance();
        if (World != nullptr && World->WorldType == EWorldType::PIE
            && IsValid(Instance) && Instance->GetGraph() == Root)
        {
            OutComponents.Add(Component);
        }
    }
}

void FKataGraphDebugger::SetDebugTarget(UKataGraphComponent* Component)
{
    DebugTarget = Component;
    bTargetClearedByUser = Component == nullptr;
}

UKataGraphInstance* FKataGraphDebugger::GetDebugInstance() const
{
    const UKataGraphComponent* Component = DebugTarget.Get();
    UKataGraphInstance* Instance = Component != nullptr ? Component->GetActiveGraphInstance() : nullptr;
    return IsValid(Instance) && Instance->GetGraph() == RootGraph.Get() ? Instance : nullptr;
}

FText FKataGraphDebugger::GetComponentLabel(const UKataGraphComponent* Component)
{
    const AActor* Owner = Component != nullptr ? Component->GetOwner() : nullptr;
    if (Owner == nullptr)
    {
        return LOCTEXT("NoOwner", "(Invalid)");
    }
    return FText::FromString(Owner->GetActorNameOrLabel());
}

FText FKataGraphDebugger::GetDebugTargetLabel() const
{
    const UKataGraphComponent* Component = DebugTarget.Get();
    return Component != nullptr ? GetComponentLabel(Component) : LOCTEXT("NoDebugObject", "No Debug Object");
}

void FKataGraphDebugger::Tick(UKataGraphBase* Page)
{
    if (LastPage.Get() != Page)
    {
        ClearHighlights(LastPage.Get());
        LastPage = Page;
    }
    ClearHighlights(Page);

    if (GEditor == nullptr || !GEditor->IsPlaySessionInProgress())
    {
        return;
    }

    // 블루프린트 디버그 필터처럼 대상이 하나뿐이면 고르지 않아도 바로 보여 준다.
    if (!DebugTarget.IsValid() && !bTargetClearedByUser)
    {
        TArray<UKataGraphComponent*> Candidates;
        GatherCandidates(Candidates);
        if (Candidates.Num() == 1)
        {
            DebugTarget = Candidates[0];
        }
    }

    const UKataGraphInstance* Instance = GetDebugInstance();
    if (Page == nullptr || Page->EdGraph == nullptr || Instance == nullptr || !Instance->IsRunning())
    {
        return;
    }

    const TArray<FGuid> PagePath = KataGraphDebugIds::GetPagePath(Page);
    ApplyHighlight(Page->EdGraph, PagePath, Instance->GetCurrentNode(),
        EKataGraphDebugHighlight::Active, EKataGraphDebugHighlight::ActiveInside);
    ApplyHighlight(Page->EdGraph, PagePath, Instance->GetPendingTargetNode(),
        EKataGraphDebugHighlight::Pending, EKataGraphDebugHighlight::PendingInside);
}

bool FKataGraphDebugger::ResolveNode(const UKataGraphNodeBase* Node, UKataGraphBase*& OutPage, UKataEdNode*& OutEdNode) const
{
    OutPage = nullptr;
    OutEdNode = nullptr;
    UKataGraphBase* Page = RootGraph.Get();
    if (Node == nullptr || Page == nullptr)
    {
        return false;
    }

    for (const FGuid& PageId : Node->DebugSubGraphPath)
    {
        UKataGraph* KataPage = Cast<UKataGraph>(Page);
        UKataGraph* Child = nullptr;
        if (KataPage != nullptr)
        {
            for (UKataGraph* Embedded : KataPage->EmbeddedSubGraphs)
            {
                if (KataPage->OwnsEmbeddedSubGraph(Embedded) && KataGraphDebugIds::GetPageId(Embedded) == PageId)
                {
                    Child = Embedded;
                    break;
                }
            }
        }
        if (Child == nullptr)
        {
            // 외장 SubGraph 안쪽은 이 편집기에서 열 수 없으므로 그것을 가리키는 포트에서 멈춘다.
            if (Page->EdGraph != nullptr)
            {
                for (UEdGraphNode* GraphNode : Page->EdGraph->Nodes)
                {
                    UKataEdNode* EdNode = Cast<UKataEdNode>(GraphNode);
                    if (EdNode != nullptr && IsPortTo(EdNode, PageId))
                    {
                        OutPage = Page;
                        OutEdNode = EdNode;
                        return true;
                    }
                }
            }
            return false;
        }
        Page = Child;
    }

    if (Page->EdGraph == nullptr)
    {
        return false;
    }
    for (UEdGraphNode* GraphNode : Page->EdGraph->Nodes)
    {
        UKataEdNode* EdNode = Cast<UKataEdNode>(GraphNode);
        if (EdNode != nullptr && IsSourceEdNode(EdNode, Node))
        {
            OutPage = Page;
            OutEdNode = EdNode;
            return true;
        }
    }
    return false;
}

void FKataGraphDebugger::HandleEndPIE(bool bIsSimulating)
{
    DebugTarget.Reset();
    bTargetClearedByUser = false;
    ClearHighlights(LastPage.Get());
}

void FKataGraphDebugger::ClearHighlights(UKataGraphBase* Page) const
{
    if (Page == nullptr || Page->EdGraph == nullptr)
    {
        return;
    }
    for (UEdGraphNode* GraphNode : Page->EdGraph->Nodes)
    {
        if (UKataEdNode* EdNode = Cast<UKataEdNode>(GraphNode))
        {
            EdNode->DebugHighlight = EKataGraphDebugHighlight::None;
        }
    }
}

#undef LOCTEXT_NAMESPACE
