#pragma once

#include "CoreMinimal.h"
#include "KataGraphNodeBase.h"
#include "KataNode.generated.h"

class UKataCondition;

/**
 * Kata 그래프 노드의 공통 기반.
 *
 * 들어오는 엣지와 무관하게 이 노드가 요구하는 조건만 담당한다.
 * 실제로 무엇을 하는지는 파생 클래스가 정한다.
 * 그래프의 Node Type이 이 클래스이므로 파생 노드가 모두 우클릭 메뉴에 나타난다.
 */
UCLASS(Abstract, BlueprintType)
class KATAGRAPH_API UKataNode : public UKataGraphNodeBase
{
    GENERATED_BODY()

public:
    UKataNode();

    /**
     * 어느 엣지로 들어오든 공통으로 요구하는 조건.
     *
     * 엣지마다 같은 조건을 중복 선언하지 않기 위한 값이다. 비우면 통과한다.
     * 평가 순서는 엣지의 조건이 먼저, 이 조건이 나중이다. 평가에 부작용이 없어야 한다.
     */
    UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Kata|Node")
    TObjectPtr<UKataCondition> EntryCondition;

    /**
     * 이 노드에 머무를 수 있는지. 머무를 수 있는 노드는 액션 하나를 실행한다.
     *
     * 실행하지 않는 노드는 전이를 해석하는 동안만 지나가는 지점이다. 진입 노드와
     * 경유 노드가 여기에 해당한다. 전이는 이런 노드에서 멈추지 않고 실행 가능한
     * 노드까지 한 번에 해석하며, 도중에 막히면 전이 자체가 성립하지 않는다.
     */
    virtual bool IsExecutableState() const { return false; }
};
