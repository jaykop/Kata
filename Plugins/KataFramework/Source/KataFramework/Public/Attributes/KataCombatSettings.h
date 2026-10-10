#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GameplayTagContainer.h"
#include "KataCombatSettings.generated.h"

/**
 * Impact 등급 하나에 대응하는 반응 이벤트와 피격 연출 Cue.
 *
 * 키와 값에 서로 다른 태그 루트를 지정해야 하므로 TMap 대신 행 구조체로 둔다.
 */
USTRUCT(BlueprintType)
struct KATAFRAMEWORK_API FKataImpactResponse
{
    GENERATED_BODY()

    /** 공격이 피해 스펙에 넣는 Impact 태그. 정확히 같은 태그만 이 행에 대응하며 부모·자식 태그는 대응하지 않는다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Impact", meta = (Categories = "Impact"))
    FGameplayTag ImpactTag;

    /** Poise가 무너졌을 때 맞은 쪽 ASC로 보내는 반응 이벤트. 비어 있으면 이 등급으로는 반응하지 않는다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Impact", meta = (Categories = "Event"))
    FGameplayTag ReactionEventTag;

    /** 피해가 적용됐을 때 실행하는 피격 연출 Gameplay Cue. 비어 있으면 연출을 재생하지 않는다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Impact", meta = (Categories = "GameplayCue"))
    FGameplayTag HitCueTag;
};

/**
 * 피해 계산과 피격 반응의 프로젝트 설정. 프로젝트 설정의 Kata Combat에서 편집하고 DefaultGame.ini에 저장한다.
 *
 * 플러그인은 태그를 정의하지 않으므로 SetByCaller·상태·Impact·반응 이벤트·Cue 태그는 프로젝트가 정해 여기에서 고른다.
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

    /** Stance 판정이 Poise 피해를 읽는 SetByCaller 태그. 피해 스펙에 이 값이 없으면 Poise 피해는 0이다. */
    UPROPERTY(Config, EditAnywhere, Category = "Hit Reaction", meta = (Categories = "SetByCaller"))
    FGameplayTag PoiseDamageSetByCallerTag;

    /** Stance 판정이 Groggy 피해를 읽는 SetByCaller 태그. 피해 스펙에 이 값이 없으면 Groggy 피해는 0이다. */
    UPROPERTY(Config, EditAnywhere, Category = "Hit Reaction", meta = (Categories = "SetByCaller"))
    FGameplayTag GroggyDamageSetByCallerTag;

    /** 맞은 쪽이 이 태그를 가지면 Poise가 무너져도 경직하지 않고 흔들림(Flinch)만 보인다. 피해와 Poise 누적은 그대로 받는다. */
    UPROPERTY(Config, EditAnywhere, Category = "Hit Reaction", meta = (Categories = "Status"))
    FGameplayTag SuperArmorTag;

    /** 피해 스펙에 Impact 태그가 없는 공격이 쓸 등급. 비어 있으면 그런 공격으로는 Poise가 무너져도 반응하지 않는다. */
    UPROPERTY(Config, EditAnywhere, Category = "Hit Reaction", meta = (Categories = "Impact"))
    FGameplayTag DefaultImpactTag;

    /** Impact 등급별 반응 이벤트와 피격 연출 Cue. 새 등급은 행을 추가해 지원하며 코드는 바꾸지 않는다. */
    UPROPERTY(Config, EditAnywhere, Category = "Hit Reaction", meta = (TitleProperty = "ImpactTag"))
    TArray<FKataImpactResponse> ImpactResponses;

    /** Poise가 버티거나 SuperArmor일 때 보내는 반응 이벤트. 비어 있으면 흔들림 반응을 보내지 않는다. */
    UPROPERTY(Config, EditAnywhere, Category = "Hit Reaction", meta = (Categories = "Event"))
    FGameplayTag FlinchEventTag;

    /** Groggy가 가득 찼을 때 보내는 반응 이벤트. Poise 판정보다 먼저 확인한다. */
    UPROPERTY(Config, EditAnywhere, Category = "Hit Reaction", meta = (Categories = "Event"))
    FGameplayTag GroggyEventTag;

    /**
     * 무기 이동 방향으로 피격 방향을 고를 때 필요한 최소 수평 성분 비율(수평 이동량 ÷ 전체 이동량).
     * 이보다 작으면 내려찍기처럼 좌우를 판단할 수 없는 공격으로 보고 공격자 위치를 기준으로 고른다.
     */
    UPROPERTY(Config, EditAnywhere, Category = "Hit Reaction", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
    float MinHorizontalDirectionRatio = 0.5f;

    /**
     * Impact 태그에 대응하는 행을 찾는다. ImpactTag가 비어 있으면 DefaultImpactTag로 찾는다.
     *
     * @return 정확히 같은 Impact 태그의 행. 찾을 태그가 비어 있거나 행이 없으면 nullptr. 설정 객체가 소유하므로 보관하지 않는다.
     */
    const FKataImpactResponse* FindImpactResponse(const FGameplayTag& ImpactTag) const;
};
