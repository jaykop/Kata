#include "KataSubGraphPortNode.h"

#include "KataGraph.h"
#include "KataGraphBase.h"
#include "KataRuntimeLog.h"

#define LOCTEXT_NAMESPACE "KataSubGraphPortNode"

namespace
{
#if WITH_EDITOR
    /** 외장 모드는 에셋만 받으며 포트의 자기·조상 그래프를 참조하지 않는다. */
    bool IsValidExternalSubGraph(const UKataGraph* Candidate, const UKataGraphBase* OwningGraph)
    {
        return Candidate != nullptr
            && Candidate->IsAsset()
            && Candidate != OwningGraph
            && (OwningGraph == nullptr || !OwningGraph->IsIn(Candidate));
    }
#endif

    /** 포트가 가리키는 서브그래프 이름을 설명으로 만든다. 비어 있으면 저작이 덜 끝난 상태를 드러낸다. */
    FText MakePortDescription(const UKataGraph* SubGraph, const FText& Format)
    {
        if (SubGraph == nullptr)
        {
            return FText::Format(Format, LOCTEXT("NoSubGraph", "(No SubGraph)"));
        }
#if WITH_EDITOR
        return FText::Format(Format, SubGraph->GetGraphDisplayName());
#else
        return FText::Format(Format, FText::FromString(SubGraph->GetName()));
#endif
    }
}

#if WITH_EDITOR
UKataGraph* UKataSubGraphPortNode::GetReferencedSubGraph() const
{
    // 저장 전에는 편집기 노드가 Outer이고 저장 후에는 그래프가 Outer다. 두 경로 모두 실제 소유를 따른다.
    const UKataGraphBase* OwningGraph = GetTypedOuter<UKataGraphBase>();
    if (bUseEmbeddedSubGraph)
    {
        const UKataGraph* OwningKataGraph = Cast<UKataGraph>(OwningGraph);
        return OwningKataGraph != nullptr && OwningKataGraph->OwnsEmbeddedSubGraph(EmbeddedSubGraph.Graph)
            ? EmbeddedSubGraph.Graph.Get()
            : nullptr;
    }

    return IsValidExternalSubGraph(SubGraph, OwningGraph) ? SubGraph.Get() : nullptr;
}

void UKataSubGraphPortNode::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    const FName ChangedName = PropertyChangedEvent.GetPropertyName();
    const FName MemberName = PropertyChangedEvent.GetMemberPropertyName();
    const bool bReferenceChanged = ChangedName == GET_MEMBER_NAME_CHECKED(UKataSubGraphPortNode, SubGraph)
        || ChangedName == GET_MEMBER_NAME_CHECKED(UKataSubGraphPortNode, bUseEmbeddedSubGraph)
        || ChangedName == GET_MEMBER_NAME_CHECKED(UKataSubGraphPortNode, EmbeddedSubGraph)
        || MemberName == GET_MEMBER_NAME_CHECKED(UKataSubGraphPortNode, EmbeddedSubGraph);

    if (bReferenceChanged)
    {
        // 숨김 메타데이터는 값을 지우지 않는다. 모드와 실제 참조가 한 대상을 뜻하도록 함께 정리한다.
        if (bUseEmbeddedSubGraph)
        {
            SubGraph = nullptr;
            if (EmbeddedSubGraph.Graph != nullptr && GetReferencedSubGraph() == nullptr)
            {
                UE_LOG(LogKata, Warning,
                    TEXT("An embedded SubGraph port must reference a direct subgraph owned by its graph. The assignment was cleared."));
                EmbeddedSubGraph.Graph = nullptr;
            }
        }
        else
        {
            EmbeddedSubGraph.Graph = nullptr;
            if (SubGraph != nullptr && GetReferencedSubGraph() == nullptr)
            {
                UE_LOG(LogKata, Warning,
                    TEXT("An external SubGraph port must reference a graph asset other than its own graph or an ancestor. The assignment was cleared."));
                SubGraph = nullptr;
            }
        }
    }

    // 변경 알림을 받는 쪽에는 검증을 마친 일관된 참조를 전달한다.
    Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

UKataSubGraphPortInNode::UKataSubGraphPortInNode()
{
#if WITH_EDITORONLY_DATA
    ContextMenuName = LOCTEXT("ContextMenuName", "SubGraph Port In");
    BackgroundColor = FLinearColor(0.08f, 0.20f, 0.16f);

    // 목적지가 서브그래프 안이라 이 그래프에서 나가는 엣지를 그릴 곳이 없다.
    ChildrenLimitType = EKataGraphNodeLimit::Limited;
    ChildrenLimit = 0;
#endif
}

FText UKataSubGraphPortInNode::GetDescription_Implementation() const
{
#if WITH_EDITOR
    return MakePortDescription(GetReferencedSubGraph(), LOCTEXT("PortInFormat", "{0} ▸ In"));
#else
    return MakePortDescription(SubGraph, LOCTEXT("PortInFormat", "{0} ▸ In"));
#endif
}

UKataSubGraphPortOutNode::UKataSubGraphPortOutNode()
{
#if WITH_EDITORONLY_DATA
    ContextMenuName = LOCTEXT("ContextMenuName", "SubGraph Port Out");
    BackgroundColor = FLinearColor(0.20f, 0.14f, 0.06f);

    // 출발지이지 목적지가 아니다. 서브그래프로 들어갈 때는 Port In을 쓴다.
    ParentLimitType = EKataGraphNodeLimit::Limited;
    ParentLimit = 0;
#endif
}

FText UKataSubGraphPortOutNode::GetDescription_Implementation() const
{
#if WITH_EDITOR
    return MakePortDescription(GetReferencedSubGraph(), LOCTEXT("PortOutFormat", "{0} ▸ Out"));
#else
    return MakePortDescription(SubGraph, LOCTEXT("PortOutFormat", "{0} ▸ Out"));
#endif
}

#undef LOCTEXT_NAMESPACE
