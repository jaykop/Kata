#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GameplayTagContainer.h"
#include "KataCombatSettings.generated.h"

/**
 * 피해 계산의 프로젝트 설정. 프로젝트 설정의 Kata Combat에서 편집하고 DefaultGame.ini에 저장한다.
 *
 * 플러그인은 태그를 정의하지 않으므로 공격 계수를 넘길 SetByCaller 태그는 프로젝트가 정해 여기에서 고른다.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Kata Combat"))
class KATAFRAMEWORK_API UKataCombatSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UKataCombatSettings();

    static const UKataCombatSettings* Get();

    /**
     * UKataDamageExecution이 공격 계수를 읽는 SetByCaller 태그.
     * 히트 처리기나 GE 태스크의 Set By Caller Magnitudes에 이 태그로 공격 계수를 넣는다. 비어 있으면 피해가 0이 된다.
     */
    UPROPERTY(Config, EditAnywhere, Category = "Damage", meta = (Categories = "SetByCaller"))
    FGameplayTag DamageSetByCallerTag;

    /**
     * 방어 식 Final = Raw × K / (K + Defense)의 K.
     * Defense가 K와 같으면 피해가 절반이 된다. 값이 클수록 같은 방어력의 감쇠 효과가 작아진다.
     */
    UPROPERTY(Config, EditAnywhere, Category = "Damage", meta = (ClampMin = "1.0", UIMin = "1.0"))
    float DefenseConstant = 100.0f;
};
