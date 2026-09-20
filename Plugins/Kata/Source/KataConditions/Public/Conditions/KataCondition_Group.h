#pragma once

#include "CoreMinimal.h"
#include "KataCondition.h"
#include "KataCondition_Group.generated.h"

/** 그룹에 담긴 자식 조건을 묶는 방식. */
UENUM(BlueprintType)
enum class EKataConditionGroupMode : uint8
{
    /** 자식 조건이 모두 Pass일 때만 Pass. */
    All,
    /** 자식 조건 중 하나라도 Pass면 Pass. */
    Any
};

/**
 * 여러 조건을 배열로 묶어 All 또는 Any로 평가하는 합성 조건.
 * 자식이 Invalid를 반환하면 설정 오류로 보고 그대로 전파한다.
 */
UCLASS(meta = (DisplayName = "Kata Condition: Group"))
class KATACONDITIONS_API UKataCondition_Group : public UKataCondition
{
    GENERATED_BODY()

public:
    /** 인라인으로 생성해 소유하는 자식 조건. 비어 있거나 null 항목이 있으면 설정 오류다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, Category = "Group")
    TArray<TObjectPtr<UKataCondition>> Conditions;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Group")
    EKataConditionGroupMode Mode = EKataConditionGroupMode::All;

protected:
    virtual FKataConditionResult EvaluateCondition_Implementation(const FKataConditionContext& Context) const override;
    virtual FName GetConfigurationError() const override;

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
