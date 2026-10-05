#pragma once

#include "AttributeSet.h"
#include "Containers/ArrayView.h"
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ScalableFloat.h"
#include "Templates/SubclassOf.h"
#include "KataGameplayData.generated.h"

class UAbilitySystemComponent;

/** Gameplay Data가 ASC에 넣을 Attribute 초기값 하나. */
USTRUCT(BlueprintType)
struct KATAFRAMEWORK_API FKataAttributeInitValue
{
    GENERATED_BODY()

    /** 초기값을 넣을 Attribute. 이 Attribute를 가진 세트가 ASC에 있어야 적용된다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attributes")
    FGameplayAttribute Attribute;

    /** 초기 기본값. Curve Table 행을 지정하면 적용 레벨로 평가한 값을 쓰고, 비워 두면 Value를 그대로 쓴다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attributes")
    FScalableFloat Value = 0.0f;
};

/**
 * Gameplay Data를 적용한 결과. 적용한 쪽이 보관한다.
 * 지금은 새로 추가한 AttributeSet만 기록한다. 부여한 Ability·Effect의 핸들은 같은 구조체에 더한다.
 */
USTRUCT(BlueprintType)
struct KATAFRAMEWORK_API FKataGameplayDataHandles
{
    GENERATED_BODY()

    /** 적용으로 새로 추가한 AttributeSet. 이미 ASC에 있던 세트는 넣지 않는다. */
    UPROPERTY(Transient)
    TArray<TObjectPtr<UAttributeSet>> AddedAttributeSets;
};

/**
 * 캐릭터가 쓰는 GAS 데이터 묶음. 캐릭터 데이터 테이블 행의 Gameplay Data 배열에 지정한다.
 *
 * 한 행에 여러 에셋을 지정해 조합한다. 예를 들어 여러 캐릭터가 공유하는 능력 구성과 캐릭터별 스탯을 서로 다른 에셋으로 나눌 수 있다.
 * 지금은 Attributes 섹션(추가할 AttributeSet과 초기값)만 가진다.
 * 공유 에셋이므로 실행 상태를 저장하지 않는다. 적용 결과는 ApplyAll이 돌려주는 FKataGameplayDataHandles에 담긴다.
 */
UCLASS(BlueprintType, Const, meta = (DisplayName = "Kata Gameplay Data"))
class KATAFRAMEWORK_API UKataGameplayData : public UDataAsset
{
    GENERATED_BODY()

public:
    /** ASC에 추가할 AttributeSet 클래스. ASC에 같은 클래스의 세트가 이미 있으면 새로 만들지 않는다. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attributes")
    TArray<TSubclassOf<UAttributeSet>> AttributeSets;

    /**
     * Attribute 초기값. 순서와 관계없이 최대값과 현재값을 함께 넣을 수 있다.
     * 현재값을 비워 두면 최대값을 넣을 때 현재값이 가득 찬 상태가 된다(UKataAttributeSet 계열 세트).
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attributes", meta = (TitleProperty = "Attribute"))
    TArray<FKataAttributeInitValue> InitialValues;

    /**
     * 여러 Gameplay Data를 배열 순서대로 ASC에 적용한다.
     *
     * 모든 에셋의 AttributeSet을 먼저 추가한 뒤 초기값을 넣는다. 같은 Attribute의 초기값이 여러 에셋에 있으면 배열 뒤쪽 값을 쓰고 경고를 남긴다.
     * ASC의 Actor Info가 초기화된 뒤 호출해야 한다. nullptr 항목은 건너뛴다.
     *
     * @param AbilitySystem 적용할 ASC. nullptr이면 아무것도 하지 않는다.
     * @param DataList 적용할 에셋 목록.
     * @param Level 초기값의 Curve Table을 평가할 레벨.
     * @return 적용 결과. 호출한 쪽이 보관한다.
     */
    static FKataGameplayDataHandles ApplyAll(UAbilitySystemComponent* AbilitySystem, TConstArrayView<TObjectPtr<UKataGameplayData>> DataList,
        float Level = 1.0f);

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};
