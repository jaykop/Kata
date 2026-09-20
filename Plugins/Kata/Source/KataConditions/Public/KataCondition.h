#pragma once

#include "CoreMinimal.h"
#include "KataConditionTypes.h"
#include "UObject/Object.h"
#include "KataCondition.generated.h"

/** 부작용 없이 공유하는 조건 정의. 액터별 평가 상태는 이 객체에 저장하지 않는다. */
UCLASS(Abstract, BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced, CollapseCategories)
class KATACONDITIONS_API UKataCondition : public UObject
{
    GENERATED_BODY()

public:
    /** 유효한 판정 결과만 반전한다. 데이터 누락과 잘못된 설정은 Invalid를 유지한다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Condition")
    bool bInvert = false;

    /** 설정 유효성 확인과 Invert가 한 번 적용되도록 항상 이 진입점을 호출한다. */
    UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Kata|Conditions")
    FKataConditionResult Evaluate(const FKataConditionContext& Context) const;

    /** Evaluate 결과가 Pass일 때만 true를 반환하는 편의 함수. */
    UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Kata|Conditions")
    bool IsSatisfied(const FKataConditionContext& Context) const;

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

protected:
    /** C++ 또는 Blueprint에서 반전 전 결과를 구현한다. 게임플레이 상태를 변경하지 않는다. */
    UFUNCTION(BlueprintNativeEvent, Category = "Kata|Conditions", meta = (BlueprintProtected))
    FKataConditionResult EvaluateCondition(const FKataConditionContext& Context) const;
    virtual FKataConditionResult EvaluateCondition_Implementation(const FKataConditionContext& Context) const;

    /** 에디터와 런타임이 공유하는 설정 검사. NAME_None이면 유효한 설정이다. */
    virtual FName GetConfigurationError() const;
};
