#pragma once

#include "CoreMinimal.h"
#include "ActionGroup/KataActionGroup.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "KataFL_ActionGroup.generated.h"

/** 그룹을 수정하거나 실행하지 않고 가중 선택 결과만 계산한다. */
UCLASS()
class KATAGRAPH_API UKataFL_ActionGroup : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /**
     * 허용한 원본 인덱스 중 유효 에셋·Payload와 양수 Weight를 가진 항목을 선택한다.
     * 중복 인덱스는 한 번만 반영하며 유효 후보가 없으면 false와 초기화된 출력을 반환한다.
     * RandomValue는 호출자가 제공한 유한한 [0, 1) 값이다. 함수는 RNG·실행 상태를 변경하지 않는다.
     * 선택은 실행 성공을 보장하지 않으며 비용·쿨다운·Pressure 판정은 소비자가 담당한다.
     */
    UFUNCTION(BlueprintPure, Category = "Kata|Action Group", meta = (DisplayName = "Select Kata Action Group Entry"))
    static bool TrySelectEntry(const UKataActionGroup* Group, const TArray<int32>& CandidateIndices,
        double RandomValue, FKataActionGroupSelection& OutSelection);
};
