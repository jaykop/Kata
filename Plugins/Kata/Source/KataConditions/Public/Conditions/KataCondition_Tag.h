#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "KataCondition.h"
#include "KataCondition_Tag.generated.h"

UENUM(BlueprintType)
enum class EKataTagMatchMode : uint8
{
    Any,
    All
};

UCLASS(meta = (DisplayName = "Kata Condition: Tag"))
class KATACONDITIONS_API UKataCondition_Tag : public UKataCondition
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tag")
    EKataConditionSubject Subject = EKataConditionSubject::Self;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tag")
    FGameplayTagContainer Tags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tag")
    EKataTagMatchMode MatchMode = EKataTagMatchMode::Any;

    /** false이면 보유한 자식 태그도 검사 대상인 부모 태그와 일치하는 것으로 처리한다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tag")
    bool bExactMatch = false;

protected:
    virtual FKataConditionResult EvaluateCondition_Implementation(const FKataConditionContext& Context) const override;
    virtual FName GetConfigurationError() const override;
};
