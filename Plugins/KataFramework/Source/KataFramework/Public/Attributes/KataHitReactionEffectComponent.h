#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectComponent.h"
#include "KataHitReactionEffectComponent.generated.h"

/**
 * 피해 GE가 적용될 때마다 맞은 쪽의 피격 반응을 한 번 정하고 반응 이벤트를 보내는 GE 컴포넌트.
 *
 * 피해 GE의 Components에 추가해 쓴다. 판정은 Execution과 Attribute 변경이 모두 끝난 뒤 한 번 한다.
 * AttributeSet의 PostGameplayEffectExecute는 바뀐 Attribute마다 따로 불리고, Poise 피해가 0이면 Stance 세트 쪽이 불리지 않으므로 그 자리에서는 한 번만 판정할 수 없다.
 * 판정 순서는 다음과 같다. 태그와 이벤트는 UKataCombatSettings에서 정한다.
 *   1. 맞은 쪽에 UKataAttributeSet_Stance가 없거나 Health가 0이면 보내지 않는다.
 *   2. Groggy가 MaxGroggy에 닿았으면 Groggy 이벤트.
 *   3. SuperArmor 태그가 있거나 Poise가 남아 있으면 Flinch 이벤트.
 *   4. Poise가 무너졌으면 스펙의 Impact 태그에 대응하는 반응 이벤트를 보내고 Poise를 가득 채운다.
 * 무적 등으로 피해 GE 적용이 막히면 이 컴포넌트도 실행되지 않는다.
 * Payload의 ContextHandle은 피해 스펙의 Effect Context(HitResult·공격자 포함), InstigatorTags는 스펙의 Asset Tag(Impact 포함)다.
 */
UCLASS(meta = (DisplayName = "Kata Hit Reaction"))
class KATAFRAMEWORK_API UKataHitReactionGameplayEffectComponent : public UGameplayEffectComponent
{
    GENERATED_BODY()

public:
    virtual void OnGameplayEffectExecuted(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const override;
};
