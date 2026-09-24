#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GenericTeamAgentInterface.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "KataFL_Faction.generated.h"

class AActor;

/**
 * 두 액터 또는 두 팩션의 관계를 판정하는 부작용 없는 함수 모음이다.
 *
 * 액터의 팀은 액터 자신의 IGenericTeamAgentInterface에서 먼저 찾고, 없거나 NoTeam이면 폰의 컨트롤러에서 찾는다.
 * 엔진 FGenericTeamId::GetAttitude(const AActor*, const AActor*)는 액터 자신만 보므로 이 함수들이 더 넓게 판정한다.
 * 관계는 전역 팀 관계 판정 함수를 따른다. KataTargeting 모듈이 이를 Kata Factions 설정으로 등록한다.
 */
UCLASS()
class KATATARGETING_API UKataFL_Faction : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /** 액터의 팀 번호. 팀을 찾지 못하면 NoTeam이다. */
    UFUNCTION(BlueprintPure, Category = "Kata|Faction")
    static FGenericTeamId GetActorTeamId(const AActor* Actor);

    /** 액터의 팩션 태그. 팀이 없거나 Kata Factions에 등록되지 않은 팀이면 빈 태그다. */
    UFUNCTION(BlueprintPure, Category = "Kata|Faction")
    static FGameplayTag GetActorFaction(const AActor* Actor);

    /** Source가 Target을 대하는 관계. 어느 한쪽이라도 없거나 팀이 없으면 중립이다. */
    UFUNCTION(BlueprintPure, Category = "Kata|Faction", meta = (DisplayName = "Get Attitude"))
    static TEnumAsByte<ETeamAttitude::Type> GetActorAttitude(const AActor* Source, const AActor* Target);

    UFUNCTION(BlueprintPure, Category = "Kata|Faction")
    static bool IsFriendly(const AActor* Source, const AActor* Target);

    UFUNCTION(BlueprintPure, Category = "Kata|Faction")
    static bool IsNeutral(const AActor* Source, const AActor* Target);

    UFUNCTION(BlueprintPure, Category = "Kata|Faction")
    static bool IsHostile(const AActor* Source, const AActor* Target);

    /** 두 팩션 태그의 관계. 액터 없이 Kata Factions 관계표 규칙만으로 계산한다. */
    UFUNCTION(BlueprintPure, Category = "Kata|Faction")
    static TEnumAsByte<ETeamAttitude::Type> GetFactionAttitude(FGameplayTag FactionA, FGameplayTag FactionB);

    /** 팩션 태그에 부여된 팀 번호. 등록되지 않은 팩션이면 NoTeam이다. */
    UFUNCTION(BlueprintPure, Category = "Kata|Faction")
    static FGenericTeamId GetFactionTeamId(FGameplayTag Faction);
};
