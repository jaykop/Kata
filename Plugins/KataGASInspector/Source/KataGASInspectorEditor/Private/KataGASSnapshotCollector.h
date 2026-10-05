#pragma once

#include "CoreMinimal.h"
#include "KataGASInspectionTypes.h"

class FKataGASSnapshotCollector
{
public:
    /** 호출은 게임 스레드에서만 수행하며 결과는 live UObject를 참조하지 않는다. */
    static FKataGASInspectionSnapshot Collect(UAbilitySystemComponent& ASC, const FString& WorldLabel);
};
