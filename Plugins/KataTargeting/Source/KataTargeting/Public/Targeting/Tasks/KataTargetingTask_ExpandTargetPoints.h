#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Tasks/TargetingTask.h"
#include "KataTargetingTask_ExpandTargetPoints.generated.h"

/**
 * 액터 결과를 그 액터의 타겟 지점 결과로 펼치는 Targeting 태스크. 액터를 모으는 Selection 태스크 바로 뒤에 둔다.
 *
 * 액터마다 켜져 있고 RequiredRoleTags를 모두 가진 UKataTargetPointComponent를 결과 하나씩으로 만든다.
 * 결과의 HitResult는 액터를 그대로 가리키고 Component에 지점, Location·ImpactPoint에 지점 위치를 담는다.
 * 지점이 없는 액터는 결과에서 빠진다. 이후 필터·정렬 태스크는 같은 액터의 지점들을 각각 후보로 다룬다.
 */
UCLASS(meta = (DisplayName = "Kata Expand Target Points"))
class KATATARGETING_API UKataTargetingTask_ExpandTargetPoints : public UTargetingTask
{
    GENERATED_BODY()

public:
    UKataTargetingTask_ExpandTargetPoints(const FObjectInitializer& ObjectInitializer);

    virtual void Execute(const FTargetingRequestHandle& TargetingHandle) const override;

protected:
    /** 지점이 모두 가져야 하는 역할 태그. 비어 있으면 켜진 지점을 모두 펼친다. */
    UPROPERTY(EditAnywhere, Category = "Kata|Targeting", meta = (Categories = "TargetPoint"))
    FGameplayTagContainer RequiredRoleTags;
};
