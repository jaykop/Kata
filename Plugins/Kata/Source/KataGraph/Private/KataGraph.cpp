#include "KataGraph.h"

UKataGraph::UKataGraph()
{
    // 콤보는 중립 상태로 돌아오는 순환이 필요하다.
#if WITH_EDITORONLY_DATA
    bCanBeCyclical = true;
#endif
}
