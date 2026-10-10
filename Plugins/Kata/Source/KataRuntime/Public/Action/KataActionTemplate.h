#pragma once

#include "CoreMinimal.h"
#include "Action/KataActionAssetBase.h"
#include "KataActionTemplate.generated.h"

/**
 * UKataAction이 부모로 참조하는 공통 설정 에셋.
 *
 * UKataAction과 같은 설정과 타임라인을 갖지만 부모를 가질 수 없고 런타임에서 직접 재생하지 않는다.
 * 재생 API는 UKataAction만 받으며, 에디터 프리뷰는 이 Template을 부모로 둔 임시 액션으로 재생한다.
 */
UCLASS(BlueprintType, meta = (IsBlueprintBase = "false", DisplayName = "Kata Action Template"))
class KATARUNTIME_API UKataActionTemplate : public UKataActionAssetBase
{
    GENERATED_BODY()
};
