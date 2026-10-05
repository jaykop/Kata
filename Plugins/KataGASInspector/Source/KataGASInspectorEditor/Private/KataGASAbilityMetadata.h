#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "KataGASInspectionTypes.h"

class UGameplayAbility;

/** 보호된 설정은 스키마를 확인한 뒤 값으로 읽는다. 프로퍼티 주소를 캐시하지 않는다. */
class FKataGASAbilityMetadata
{
public:
    static bool ReadTriggers(const UGameplayAbility* Ability, TArray<FAbilityTriggerData>& OutTriggers);
    static bool ReadTasks(UGameplayAbility* Ability, const FString& ParentKey,
        TArray<TSharedPtr<FKataGASInspectionRow>>& OutRows);
    static FSoftObjectPath GetSourceAsset(const UClass* Class);
};
