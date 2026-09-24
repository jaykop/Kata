#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GameplayTagContainer.h"
#include "GenericTeamAgentInterface.h"
#include "KataFactionSettings.generated.h"

/** 두 팩션 사이의 관계 한 줄. FactionA와 FactionB의 순서는 구분하지 않는다. */
USTRUCT(BlueprintType)
struct KATATARGETING_API FKataFactionRelation
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Faction")
    FGameplayTag FactionA;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Faction")
    FGameplayTag FactionB;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Faction")
    TEnumAsByte<ETeamAttitude::Type> Attitude = ETeamAttitude::Neutral;
};

/**
 * 팩션 목록과 팩션 사이의 관계표. 프로젝트 설정의 Kata Factions에서 편집하고 DefaultGame.ini에 저장한다.
 *
 * 팩션은 Gameplay Tag로 표현하며 태그 정의는 프로젝트가 소유한다.
 * Factions 목록의 순서가 엔진 FGenericTeamId 번호가 되므로, 순서를 바꾸면 이미 부여된 팀 번호의 뜻도 바뀐다.
 *
 * 두 팩션 X, Y의 관계는 다음 순서로 정한다.
 * 1. Relations에서 태그 계층까지 포함해 X·Y와 맞는 행을 방향과 무관하게 찾고, 두 태그의 계층 깊이 합이 가장 큰 행을 쓴다.
 * 2. 맞는 행이 없고 X와 Y가 같은 팩션이면 우호다.
 * 3. 그 밖에는 중립이다. 적대 관계는 Relations에 적어야 생긴다.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Kata Factions"))
class KATATARGETING_API UKataFactionSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UKataFactionSettings();

    /** 등록한 팩션. 인덱스가 팀 번호다. 엔진의 NoTeam(255)과 겹치지 않도록 앞의 255개만 사용한다. */
    UPROPERTY(Config, EditAnywhere, Category = "Faction")
    TArray<FGameplayTag> Factions;

    /** 팩션 사이의 관계. 부모 태그로 적으면 그 아래 팩션 모두에 적용한다. */
    UPROPERTY(Config, EditAnywhere, Category = "Faction")
    TArray<FKataFactionRelation> Relations;

    static const UKataFactionSettings* Get();

    /** 팩션의 팀 번호. 등록되지 않았거나 255번째 이후 항목이면 NoTeam을 돌려준다. 태그는 정확히 일치해야 한다. */
    FGenericTeamId GetTeamId(const FGameplayTag& Faction) const;

    /** 팀 번호의 팩션. 등록되지 않은 번호면 빈 태그를 돌려준다. */
    FGameplayTag GetFaction(FGenericTeamId TeamId) const;

    /** 두 팩션의 관계를 관계표 규칙으로 계산한다. 어느 한쪽이 빈 태그면 중립이다. */
    ETeamAttitude::Type GetFactionAttitude(const FGameplayTag& FactionA, const FGameplayTag& FactionB) const;

    /** 두 팀 번호의 관계. 계산해 둔 행렬을 읽으며, 한쪽이라도 NoTeam이거나 등록되지 않은 번호면 중립이다. */
    ETeamAttitude::Type GetTeamAttitude(FGenericTeamId TeamA, FGenericTeamId TeamB) const;

    /** FGenericTeamId::SetAttitudeSolver에 등록하는 전역 관계 판정 함수. */
    static ETeamAttitude::Type SolveTeamAttitude(FGenericTeamId TeamA, FGenericTeamId TeamB);

    virtual void PostInitProperties() override;
    virtual void PostReloadConfig(FProperty* PropertyThatWasLoaded) override;
#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
    /** 관계 행렬이 설정과 맞지 않으면 다시 계산한다. 설정 로드 순서와 무관하도록 첫 조회 때 계산한다. */
    void EnsureAttitudeCache() const;

    /** 팀 번호 쌍의 관계. 행 우선이며 크기는 CachedTeamCount의 제곱이다. 설정이 아니라 계산 결과이므로 저장하지 않는다. */
    mutable TArray<uint8> AttitudeCache;

    mutable int32 CachedTeamCount = 0;

    mutable bool bAttitudeCacheDirty = true;
};
