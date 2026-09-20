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
