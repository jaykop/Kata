#pragma once

#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "CoreMinimal.h"
#include "KataAttributeSet.generated.h"

/**
 * Kata AttributeSet의 공통 기반.
 *
 * 각 세트는 ATTRIBUTE_ACCESSORS_BASIC으로 접근자를 만들고, 값 제한 규칙을 ClampAttribute 한 곳에 둔다.
 * 기반은 기본값(Base) 변경과 최종값(Current) 변경 모두에 ClampAttribute를 적용하므로, 버프로 바뀐 최종값도 같은 범위를 지킨다.
 * 세트 인스턴스는 클래스 기본 서브오브젝트가 아니라 UKataGameplayData가 ASC에 추가한다.
 * 싱글플레이 전용이므로 복제 설정을 두지 않는다.
 */
UCLASS(Abstract)
class KATAFRAMEWORK_API UKataAttributeSet : public UAttributeSet
{
    GENERATED_BODY()

public:
    //~ Begin UAttributeSet Interface
    virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
    virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
    //~ End UAttributeSet Interface

protected:
    /** 파생 세트가 Attribute별 허용 범위를 적용한다. 기본 구현은 값을 바꾸지 않는다. */
    virtual void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;

    /**
     * Max 최종값이 바뀐 뒤 짝이 되는 현재값의 기본값을 같은 비율로 맞춘다.
     * 이전 Max가 0 이하이면 비율을 정할 수 없으므로 현재값을 새 Max로 채운다. 그래서 초기화에서 Max만 지정하면 가득 찬 상태로 시작한다.
     * 현재값에는 Duration·Infinite 모디파이어를 걸지 않는다는 전제로 기본값만 조정한다.
     */
    void AdjustCurrentForMaxChange(const FGameplayAttribute& CurrentAttribute, float OldMaxValue, float NewMaxValue) const;
};
