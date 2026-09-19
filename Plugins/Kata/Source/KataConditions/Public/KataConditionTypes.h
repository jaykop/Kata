#pragma once

#include "CoreMinimal.h"
#include "KataConditionTypes.generated.h"

class AActor;
class UAbilitySystemComponent;

UENUM(BlueprintType)
enum class EKataConditionStatus : uint8
{
    Pass,
    Fail,
    Invalid
};

UENUM(BlueprintType)
enum class EKataConditionSubject : uint8
{
    Self,
    Target
};

UENUM(BlueprintType)
enum class EKataConditionSpace : uint8
{
    Plane2D UMETA(DisplayName = "2D (XY)"),
    Spatial3D UMETA(DisplayName = "3D")
};

UENUM(BlueprintType)
enum class EKataNumericComparison : uint8
{
    LessThan,
    LessOrEqual,
    GreaterThan,
    GreaterOrEqual,
    Equal,
    NotEqual
};

/** A supplied ASC takes precedence over actor lookup, including ASC-on-PlayerState setups. */
USTRUCT(BlueprintType)
struct KATACONDITIONS_API FKataConditionContext
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kata|Condition")
    TWeakObjectPtr<AActor> SelfActor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kata|Condition")
    TWeakObjectPtr<AActor> TargetActor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kata|Condition")
    TWeakObjectPtr<UAbilitySystemComponent> SelfAbilitySystem;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kata|Condition")
    TWeakObjectPtr<UAbilitySystemComponent> TargetAbilitySystem;

    AActor* GetActor(EKataConditionSubject Subject) const;
    UAbilitySystemComponent* GetAbilitySystem(EKataConditionSubject Subject) const;
};

/** Reason is an extensible diagnostic ID, not a registered Gameplay Tag. Only Pass is satisfied. */
USTRUCT(BlueprintType)
struct KATACONDITIONS_API FKataConditionResult
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kata|Condition")
    EKataConditionStatus Status = EKataConditionStatus::Invalid;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kata|Condition")
    FName Reason = NAME_None;

    bool IsSatisfied() const { return Status == EKataConditionStatus::Pass; }

    static FKataConditionResult FromBool(bool bSatisfied);
    static FKataConditionResult Invalid(FName Reason);
};
