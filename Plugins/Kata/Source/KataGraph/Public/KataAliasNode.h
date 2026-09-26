#pragma once

#include "CoreMinimal.h"
#include "KataNode.h"
#include "KataAliasNode.generated.h"

/**
 * 별칭이 대신할 출발지 집합.
 *
 * 노드는 에셋이 아니라 그래프 에셋의 하위 객체라서 기본 오브젝트 피커로는 고를 수 없다.
 * 이 구조체를 두어 편집기가 그래프 안의 후보 노드를 나열하고 토글로 고르게 한다.
 * 편집은 KataGraphEditor의 FKataAliasSourceSetCustomization이 담당한다.
 */
USTRUCT(BlueprintType)
struct KATAGRAPH_API FKataAliasSourceSet
{
    GENERATED_BODY()

    /** 별칭에 포함된 노드. 편집기 커스터마이제이션이 채우므로 기본 배열 UI로 편집하지 않는다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata")
    TArray<TObjectPtr<UKataNode>> Nodes;
};

/**
 * 출발지 여러 개를 하나의 이름으로 묶는 별칭. 액션을 실행하지 않는다.
 *
 * 같은 목적지로 가는 전이를 노드마다 긋지 않고 한 번만 긋기 위한 것이다. 이 노드에서
 * 나가는 엣지는 포함된 노드 각각에서 나가는 엣지로 취급한다. 사망이나 스턴처럼 무엇을
 * 하고 있든 가로채야 하는 전이는 Any State를 켜서 모든 노드를 포함시킨다.
 *
 * 여기에 머무를 수 없고 들어오는 연결도 받지 않는다. 목적지가 아니라 출발지이기 때문이다.
 * 나가는 엣지는 현재 액션을 떠나는 전이이므로 Required Window Tag와 Timing을 그대로
 * 적용한다. 이미 떠난 뒤의 경로인 UKataConduitNode와 다른 점이다.
 */
UCLASS(BlueprintType, meta = (DisplayName = "Kata Alias Node"))
class KATAGRAPH_API UKataAliasNode : public UKataNode
{
    GENERATED_BODY()

public:
    UKataAliasNode();

    /**
     * 이 별칭이 대신하는 출발지. Any State를 켜면 사용하지 않는다.
     *
     * 같은 그래프의 노드만 의미가 있다. 실행 중인 노드는 언제나 자기 그래프의 노드다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata",
        meta = (EditCondition = "!bAnyState", EditConditionHides))
    FKataAliasSourceSet SourceNodes;

    /** 켜면 Source Nodes를 무시하고 실행 중일 수 있는 모든 노드를 출발지로 삼는다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata",
        meta = (DisplayName = "Any State"))
    bool bAnyState = false;

    /** 지정한 노드가 이 별칭의 출발지에 드는지. 자기 자신은 들지 않는다. */
    UFUNCTION(BlueprintPure, Category = "Kata|Alias")
    bool CoversNode(const UKataNode* Node) const;

    virtual FText GetDescription_Implementation() const override;
};
