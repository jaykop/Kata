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

    /** When false, owned child tags also match a requested parent tag. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tag")
    bool bExactMatch = false;

protected:
    virtual FKataConditionResult EvaluateCondition_Implementation(const FKataConditionContext& Context) const override;
    virtual FName GetConfigurationError() const override;
};
