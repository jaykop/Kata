#pragma once

#include "GameplayEffect.h"
#include "KataCooldownGameplayEffect.generated.h"

/** Kata 쿨다운 시간을 ASC에 보관하기 위한 내부 Duration Gameplay Effect. */
UCLASS(NotBlueprintable)
class UKataCooldownGameplayEffect final : public UGameplayEffect
{
    GENERATED_BODY()

public:
    UKataCooldownGameplayEffect();
};
