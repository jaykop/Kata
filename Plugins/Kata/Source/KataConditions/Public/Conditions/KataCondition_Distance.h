#pragma once

#include "CoreMinimal.h"
#include "KataCondition.h"
#include "KataCondition_Distance.generated.h"

UENUM(BlueprintType)
enum class EKataLocationMode : uint8
{
    ActorLocation,
    Socket
};

/** Socket 모드는 기본적으로 Character::Mesh를 사용하며, 컴포넌트 태그를 지정하면 일치하는 SceneComponent 하나를 사용한다. */
USTRUCT(BlueprintType)
struct KATACONDITIONS_API FKataConditionLocation
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location")
    EKataLocationMode Mode = EKataLocationMode::ActorLocation;

    /** 비어 있으면 Character::Mesh를 사용한다. 지정하면 해당 태그를 가진 SceneComponent가 정확히 하나여야 한다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location", meta = (EditCondition = "Mode == EKataLocationMode::Socket", EditConditionHides))
    FName ComponentTag = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location", meta = (EditCondition = "Mode == EKataLocationMode::Socket", EditConditionHides))
    FName SocketName = NAME_None;
};

UCLASS(meta = (DisplayName = "Kata Condition: Distance"))
class KATACONDITIONS_API UKataCondition_Distance : public UKataCondition
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance")
    FKataConditionLocation SelfLocation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance")
    FKataConditionLocation TargetLocation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance")
    EKataConditionSpace Space = EKataConditionSpace::Plane2D;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Comparison")
    EKataNumericComparison Comparison = EKataNumericComparison::LessOrEqual;

    /** 계산한 거리를 이 기준 거리와 비교한다. 단위는 cm다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Comparison", meta = (ClampMin = "0", Units = "cm"))
    float CompareDistance = 200.0f;

    /** Equal과 NotEqual에만 적용하는 거리 허용 오차. 단위는 cm다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Comparison", meta = (ClampMin = "0", Units = "cm", EditCondition = "Comparison == EKataNumericComparison::Equal || Comparison == EKataNumericComparison::NotEqual", EditConditionHides))
    float EqualityTolerance = 1.0f;

protected:
    virtual FKataConditionResult EvaluateCondition_Implementation(const FKataConditionContext& Context) const override;
    virtual FName GetConfigurationError() const override;
};
