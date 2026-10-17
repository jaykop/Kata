#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"
#include "KataHitReactionOverride.generated.h"

struct FGameplayEffectSpec;

/** 피격 반응 판정 컴포넌트가 판정 결과 대신 쓸 반응을 맞은 쪽에 묻기 위한 인터페이스다. */
UINTERFACE(MinimalAPI)
class UKataHitReactionOverride : public UInterface
{
    GENERATED_BODY()
};

/**
 * 맞은 쪽 Avatar 액터나 그 컴포넌트가 구현한다. 피해 GE의 UKataHitReactionGameplayEffectComponent가
 * Stance 세트가 없거나 Health가 0인 경우를 제외한 뒤, Groggy·SuperArmor·Poise 판정보다 먼저 이 인터페이스에 묻는다.
 * Avatar 액터가 이 인터페이스를 구현했는지 먼저 확인한다. 구현하지 않았다면 처음 찾은 구현 컴포넌트 하나만 쓴다.
 */
class KATAFRAMEWORK_API IKataHitReactionOverride
{
    GENERATED_BODY()

public:
    /**
     * 이번 피격에 판정 대신 보낼 반응 이벤트 태그를 돌려준다. 빈 태그면 판정 결과를 그대로 쓴다.
     * 태그를 돌려주면 판정 컴포넌트는 Poise를 다시 채우지 않는다. 피해 GE는 이미 실행되어 Poise·Groggy 누적은 반영된 상태다.
     * DamageSpec은 판정 중인 피해 스펙이다. 판정 도중 호출되므로 GE를 적용하거나 Attribute를 바꾸지 않는다.
     */
    virtual FGameplayTag GetForcedHitReactionEventTag(const FGameplayEffectSpec& DamageSpec) const = 0;
};
