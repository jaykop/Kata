#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "KataPreviewSetup.generated.h"

/**
 * 액션 편집기의 프리뷰 액터를 준비하는 확장 지점.
 *
 * 액션 에셋의 Preview Setups 배열에 인라인으로 추가하며, 액션 편집기가 프리뷰 액터를 스폰하고 ASC를 준비한 직후 배열 순서로 ApplyToPreview를 부른다.
 * 코어는 장비 같은 위성 기능을 모르므로, 위성·통합 플러그인이 이 클래스를 상속해 자기 기능으로 프리뷰 액터를 꾸민다.
 * 게임 월드에서는 호출하지 않는다. 설정만 담고 실행 상태를 저장하지 않는다.
 */
UCLASS(Abstract, EditInlineNew, DefaultToInstanced, CollapseCategories)
class KATARUNTIME_API UKataPreviewSetup : public UObject
{
    GENERATED_BODY()

public:
    /**
     * 스폰한 프리뷰 액터에 이 설정을 적용한다. 프리뷰를 다시 만들 때마다 새 액터로 다시 불린다.
     * 프리뷰 액터는 BeginPlay를 받지 않으므로 BeginPlay에 기대는 초기화는 여기서 직접 해야 한다.
     */
    virtual void ApplyToPreview(AActor* PreviewActor) const
    {
    }
};
