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

/** Socket mode uses Character::Mesh by default, or a uniquely tagged SceneComponent. */
USTRUCT(BlueprintType)
struct KATACONDITIONS_API FKataConditionLocation
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location")
    EKataLocationMode Mode = EKataLocationMode::ActorLocation;

    /** Empty selects Character::Mesh. Otherwise exactly one SceneComponent must have this component tag. */
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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance", meta = (ClampMin = "0", Units = "cm"))
    float MinDistance = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance", meta = (ClampMin = "0", Units = "cm"))
    float MaxDistance = 200.0f;

protected:
    virtual FKataConditionResult EvaluateCondition_Implementation(const FKataConditionContext& Context) const override;
    virtual FName GetConfigurationError() const override;
};
