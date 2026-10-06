#include "KataGraph.h"

#include "KataEdge.h"
#include "KataNode.h"

UKataGraph::UKataGraph()
{
    // 추상 기반을 지정하면 우클릭 메뉴가 파생 노드를 모두 나열한다.
    NodeType = UKataNode::StaticClass();
    EdgeType = UKataEdge::StaticClass();

#if WITH_EDITORONLY_DATA
    // 콤보는 중립 상태로 돌아오는 순환이 필요하다.
    bCanBeCyclical = true;
#endif
}

#if WITH_EDITOR
FText UKataGraph::GetGraphDisplayName() const
{
    if (GetTypedOuter<UKataGraph>() != nullptr && !GraphDisplayName.IsEmpty())
    {
        return GraphDisplayName;
    }

    return FText::FromString(GetName());
}

bool UKataGraph::OwnsEmbeddedSubGraph(const UKataGraph* Candidate) const
{
    return Candidate != nullptr
        && Candidate != this
        && Candidate->GetOuter() == this
        && EmbeddedSubGraphs.Contains(Candidate);
}

bool UKataGraph::ContainsGraph(const UKataGraph* Candidate) const
{
    while (Candidate != nullptr && Candidate != this)
    {
        const UKataGraph* Owner = Cast<UKataGraph>(Candidate->GetOuter());
        if (Owner == nullptr || !Owner->OwnsEmbeddedSubGraph(Candidate))
        {
            return false;
        }
        Candidate = Owner;
    }
    return Candidate == this;
}

const UKataGraph* UKataGraph::GetRootGraph() const
{
    const UKataGraph* Graph = this;
    while (const UKataGraph* Owner = Cast<UKataGraph>(Graph->GetOuter()))
    {
        if (!Owner->OwnsEmbeddedSubGraph(Graph))
        {
            return nullptr;
        }
        Graph = Owner;
    }
    return Graph->IsAsset() ? Graph : nullptr;
}
#endif
