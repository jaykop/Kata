#pragma once

#include "CoreMinimal.h"
#include "KataGraphBase.h"
#include "KataGraph.generated.h"

/**
 * 콤보 전이를 담는 그래프 에셋.
 *
 * 노드는 실행할 액션을, 엣지는 트리거 태그와 조건으로 전이를 정의한다.
 * 고유 설정은 아직 없으며 노드·엣지 타입만 고정한다.
 */
UCLASS(BlueprintType, meta = (DisplayName = "Kata Graph"))
class KATAGRAPH_API UKataGraph : public UKataGraphBase
{
    GENERATED_BODY()

public:
    UKataGraph();
};
