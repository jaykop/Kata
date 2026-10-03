#include "KataSubGraphPortNode.h"

#include "KataGraph.h"
#include "KataGraphBase.h"
#include "KataRuntimeLog.h"

#define LOCTEXT_NAMESPACE "KataSubGraphPortNode"

namespace
{
    /** 포트가 가리키는 서브그래프 이름을 설명으로 만든다. 비어 있으면 저작이 덜 끝난 상태를 드러낸다. */
    FText MakePortDescription(const UKataGraph* SubGraph, const FText& Format)
    {
        if (SubGraph == nullptr)
        {
            return FText::Format(Format, LOCTEXT("NoSubGraph", "(No SubGraph)"));
        }
        return FText::Format(Format, FText::FromString(SubGraph->GetName()));
    }
}

#if WITH_EDITOR
void UKataSubGraphPortNode::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    const FName ChangedName = PropertyChangedEvent.GetPropertyName();
    if (ChangedName != GET_MEMBER_NAME_CHECKED(UKataSubGraphPortNode, SubGraph) || SubGraph == nullptr)
    {
        return;
    }

    // 노드를 막 만들었을 때는 Outer가 편집기 노드이고 그래프를 다시 만든 뒤에는 그래프다.
    // 두 경우 모두 바깥으로 올라가면 소유 그래프를 찾는다.
    const UKataGraphBase* OwningGraph = GetTypedOuter<UKataGraphBase>();
    if (OwningGraph != nullptr && SubGraph == OwningGraph)
    {
        UE_LOG(LogKata, Warning,
            TEXT("A SubGraph port cannot reference its own graph. The assignment was reverted."));
        SubGraph = nullptr;
    }
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
    return MakePortDescription(SubGraph, LOCTEXT("PortInFormat", "{0} ▸ In"));
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
    return MakePortDescription(SubGraph, LOCTEXT("PortOutFormat", "{0} ▸ Out"));
}

#undef LOCTEXT_NAMESPACE
