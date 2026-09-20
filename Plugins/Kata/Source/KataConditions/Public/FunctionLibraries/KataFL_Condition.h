#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "KataConditionTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "KataFL_Condition.generated.h"

class AActor;
class UAbilitySystemComponent;

/** 조건 UObject와 Blueprint에서 함께 사용하는 부작용 없는 판정 함수 모음이다. */
UCLASS()
class KATACONDITIONS_API UKataFL_Condition : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /** 두 수치를 지정한 비교 방식으로 판정한다. 유효하지 않은 입력은 OutError로 구분한다. */
    UFUNCTION(BlueprintPure, Category = "Kata|Condition", meta = (DisplayName = "Compare Value"))
    static bool CompareValue(
        double Value,
        double ReferenceValue,
        EKataNumericComparison Comparison,
        double EqualityTolerance,
        FName& OutError);

    /** 두 월드 위치 사이의 거리를 기준값과 비교한다. 입력이 유효하지 않으면 false와 OutError를 반환한다. */
    UFUNCTION(BlueprintPure, Category = "Kata|Condition", meta = (DisplayName = "Check Distance"))
    static bool CheckDistance(
        const FVector& FirstLocation,
        const FVector& SecondLocation,
        EKataConditionSpace Space,
        double ReferenceDistance,
        EKataNumericComparison Comparison,
        double EqualityTolerance,
        FName& OutError);

    /** SourceActor의 전방 기준으로 TargetActor가 반각 안에 있는지 판정한다. 입력이 유효하지 않으면 false와 OutError를 반환한다. */
    UFUNCTION(BlueprintPure, Category = "Kata|Condition", meta = (DisplayName = "Check Angle"))
    static bool CheckAngle(
        AActor* SourceActor,
        AActor* TargetActor,
        EKataConditionSpace Space,
        double HalfAngleDegrees,
        double YawOffsetDegrees,
        FName& OutError);

    /** AbilitySystem이 Gameplay Tag 조건을 만족하는지 판정한다. 입력이 유효하지 않으면 false와 OutError를 반환한다. */
    UFUNCTION(BlueprintPure, Category = "Kata|Condition", meta = (DisplayName = "Check Tag", AutoCreateRefTerm = "Tags"))
    static bool CheckTag(
        UAbilitySystemComponent* AbilitySystem,
        const FGameplayTagContainer& Tags,
        EKataTagMatchMode MatchMode,
        bool bExactMatch,
        FName& OutError);

    /** 수치 비교 방식과 허용 오차 설정을 검증한다. */
    static FName ValidateComparison(EKataNumericComparison Comparison, double EqualityTolerance);
};
