#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Tasks/TargetingFilterTask_BasicFilterTemplate.h"
#include "KataTargetingFilterTask_OwnedTags.generated.h"

/**
 * 후보 액터의 ASC가 가진 태그로 거르는 Targeting 필터.
 * 사망·무력화처럼 대상이 될 수 없는 상태의 후보를 뺄 때 쓴다. 후보에 ASC가 없으면 태그가 없는 것으로 보고 남긴다.
 * 후보 지점을 펼친 뒤에도 지점의 소유 액터로 판정하므로 Kata Expand Target Points 앞뒤 어디에 두어도 된다.
 */
UCLASS(meta = (DisplayName = "Kata Filter Owned Tags"))
class KATATARGETING_API UKataTargetingFilterTask_OwnedTags : public UTargetingFilterTask_BasicFilterTemplate
{
    GENERATED_BODY()

protected:
    virtual bool ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const override;

    /**
     * 후보 ASC에 이 태그 중 하나라도 있으면 후보에서 뺀다. 비어 있으면 아무것도 거르지 않는다.
     * 어떤 상태를 제외할지는 게임마다 다르므로 루트를 좁히지 않는다. 보통 Status 태그를 넣는다.
     */
    UPROPERTY(EditAnywhere, Category = "Kata|Tags")
    FGameplayTagContainer ExcludedTags;
};
