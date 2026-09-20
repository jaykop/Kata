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

UENUM(BlueprintType)
enum class EKataTagMatchMode : uint8
{
    Any,
    All
};

/** 명시적으로 전달한 ASC를 액터 조회보다 우선 사용한다. ASC가 별도 PlayerState에 있으면 명시 필드로 전달한다. */
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

/** Reason은 등록된 Gameplay Tag가 아닌 확장 가능한 진단 ID다. Pass만 조건 충족으로 처리한다. */
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
