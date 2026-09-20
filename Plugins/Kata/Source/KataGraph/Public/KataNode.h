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
};
