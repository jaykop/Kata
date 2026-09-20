#pragma once

#include "CoreMinimal.h"
#include "KataNode.h"
#include "KataEntryNode.generated.h"

/**
 * 그래프의 진입점. 액션을 실행하지 않는다.
 *
 * 나가는 엣지의 트리거가 곧 "이 그래프를 시작하는 조건"이다.
 * 그래프 안의 전이와 같은 평가 경로를 쓰므로 진입점을 여러 개 둘 수 있다.
 * 기준이 될 현재 액션이 없어 진입 엣지는 Required Window Tag와 Timing을 무시한다.
 */
UCLASS(BlueprintType, meta = (DisplayName = "Kata Entry Node"))
class KATAGRAPH_API UKataEntryNode : public UKataNode
{
    GENERATED_BODY()

public:
    UKataEntryNode();

    virtual FText GetDescription_Implementation() const override;
};
