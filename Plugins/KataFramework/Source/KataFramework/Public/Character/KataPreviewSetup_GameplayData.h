#pragma once

#include "Action/KataPreviewSetup.h"
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "KataPreviewSetup_GameplayData.generated.h"

class UKataGameplayData;

/**
 * 액션 편집기의 프리뷰 액터에 Gameplay Data를 적용하는 준비 설정. 액션 에셋의 Preview Setups에 추가한다.
 *
 * 프리뷰 액터에는 캐릭터 행이 적용되지 않으므로, 비용·Attribute 조건처럼 스탯이 필요한 액션을 미리 볼 때 여기서 같은 에셋을 지정한다.
 * 프리뷰 액터에 ASC가 없으면 경고를 남기고 건너뛴다.
 */
UCLASS(meta = (DisplayName = "Gameplay Data"))
class KATAFRAMEWORK_API UKataPreviewSetup_GameplayData : public UKataPreviewSetup
{
    GENERATED_BODY()

public:
    /** 배열 순서대로 적용할 Gameplay Data. 캐릭터 행의 Gameplay Data와 같은 규칙을 따른다. */
    UPROPERTY(EditAnywhere, Category = "Gameplay Data")
    TArray<TObjectPtr<UKataGameplayData>> GameplayData;

    /** 프리뷰 액터에 더할 Identity 태그. 캐릭터 행의 Identity Tags에 해당한다. */
    UPROPERTY(EditAnywhere, Category = "Gameplay Data", meta = (Categories = "Identity"))
    FGameplayTagContainer IdentityTags;

    /** 초기값의 Curve Table을 평가할 레벨. */
    UPROPERTY(EditAnywhere, Category = "Gameplay Data", meta = (ClampMin = "1.0"))
    float Level = 1.0f;

    virtual void ApplyToPreview(AActor* PreviewActor) const override;
};
