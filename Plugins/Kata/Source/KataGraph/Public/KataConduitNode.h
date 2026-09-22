#pragma once

#include "CoreMinimal.h"
#include "KataNode.h"
#include "KataConduitNode.generated.h"

/**
 * 전이만 갈아타는 경유지. 액션을 실행하지 않는다.
 *
 * 여러 노드에서 같은 곳으로 가는 전이를 하나로 모으고, 다시 여러 곳으로 퍼뜨린다.
 * 출발지 N개와 목적지 M개를 직접 이으면 엣지가 N×M개지만, 이 노드를 거치면 N+M개다.
 * 공통으로 요구하는 조건도 엣지마다 반복하지 않고 이 노드의 Entry Condition에 한 번만 적는다.
 *
 * 여기에 머무를 수 없다. 전이는 이 노드를 지나 실행 가능한 노드까지 한 번에 해석하며,
 * 도중에 조건이 막히거나 나가는 엣지가 없으면 전이 자체가 성립하지 않는다.
 * 나가는 엣지의 Required Window Tag와 Timing은 떠나는 액션이 없어 무시한다.
 * 트리거는 처음 들어온 엣지에서 이미 받았으므로, 나가는 엣지는 트리거를 비워 두거나
 * 같은 트리거를 적어야 통과한다.
 */
UCLASS(BlueprintType, meta = (DisplayName = "Kata Conduit Node"))
class KATAGRAPH_API UKataConduitNode : public UKataNode
{
    GENERATED_BODY()

public:
    UKataConduitNode();

    virtual FText GetDescription_Implementation() const override;
};
