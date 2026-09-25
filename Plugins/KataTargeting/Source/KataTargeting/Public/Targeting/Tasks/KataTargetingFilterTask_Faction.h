#pragma once

#include "CoreMinimal.h"
#include "GenericTeamAgentInterface.h"
#include "Tasks/TargetingFilterTask_BasicFilterTemplate.h"
#include "KataTargetingFilterTask_Faction.generated.h"

/**
 * 실행 주체가 후보를 대하는 팩션 관계로 거르는 Targeting 필터.
 * 관계는 UKataFL_Faction::GetActorAttitude로 판정한다. 팀을 찾지 못한 후보는 중립으로 본다.
 */
UCLASS(meta = (DisplayName = "Kata Filter Faction"))
class KATATARGETING_API UKataTargetingFilterTask_Faction : public UTargetingFilterTask_BasicFilterTemplate
{
    GENERATED_BODY()

public:
    UKataTargetingFilterTask_Faction(const FObjectInitializer& ObjectInitializer);

protected:
    virtual bool ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const override;

    /** 남길 관계. 실행 주체가 후보를 대하는 관계가 목록에 없으면 후보에서 뺀다. 기본값은 적대만 남긴다. */
    UPROPERTY(EditAnywhere, Category = "Kata|Faction")
    TArray<TEnumAsByte<ETeamAttitude::Type>> AllowedAttitudes;
};
