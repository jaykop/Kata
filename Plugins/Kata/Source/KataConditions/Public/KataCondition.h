#pragma once

#include "CoreMinimal.h"
#include "KataConditionTypes.h"
#include "UObject/Object.h"
#include "KataCondition.generated.h"

/** Shared, side-effect-free definition. Never store per-actor evaluation state here. */
UCLASS(Abstract, BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced, CollapseCategories)
class KATACONDITIONS_API UKataCondition : public UObject
{
    GENERATED_BODY()

public:
    /** Reverse a valid result. Missing data and invalid configuration remain Invalid. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Condition")
    bool bInvert = false;

    /** Always call this entry point so configuration validation and Invert are applied once. */
    UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Kata|Conditions")
    FKataConditionResult Evaluate(const FKataConditionContext& Context) const;

    /** Convenience wrapper: returns true only when Evaluate returns Pass. */
    UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Kata|Conditions")
    bool IsSatisfied(const FKataConditionContext& Context) const;

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

protected:
    /** Implement the uninverted result in C++ or Blueprint. Do not mutate gameplay state. */
    UFUNCTION(BlueprintNativeEvent, Category = "Kata|Conditions", meta = (BlueprintProtected))
    FKataConditionResult EvaluateCondition(const FKataConditionContext& Context) const;
    virtual FKataConditionResult EvaluateCondition_Implementation(const FKataConditionContext& Context) const;

    /** Shared editor/runtime configuration check; NAME_None means valid. */
    virtual FName GetConfigurationError() const;
};
