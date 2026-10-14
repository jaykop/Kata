#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "KataDummyComponent.generated.h"

class AKataCharacter;
class AKataCharacterSpawner;
class UKataDeathComponent;

/** 더미에 거는 보호. 게임플레이 태그로 걸며 치트 태그와는 별개다. */
UENUM(BlueprintType)
enum class EKataDummyProtection : uint8
{
    /** 보호하지 않는다. */
    None,
    /**
     * Kata Combat의 Invincible Tag를 붙인다. 피해 GE의 Target Tag Requirements에 같은 태그가 있어야 피해 GE 적용이 막힌다.
     * GE를 쓰지 않는 처리와 사망 컴포넌트의 직접 Kill은 막지 않는다.
     */
    Invincible,
    /** Kata Combat의 Ignore Damage Tag를 붙인다. 피해 GE는 적용되어 피격 반응과 Poise·Groggy 누적은 그대로이고 Health만 줄지 않는다. */
    IgnoreDamage UMETA(DisplayName = "Ignore Damage")
};

/** 더미 역할 설정. 스포너의 Dummy 설정이 들고 있다가 생성한 NPC의 더미 컴포넌트에 값으로 복사한다. */
USTRUCT(BlueprintType)
struct KATAFRAMEWORK_API FKataDummySettings
{
    GENERATED_BODY()

    /** 생성 직후 거는 보호. 컴포넌트가 사라질 때 붙인 태그를 뗀다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dummy")
    EKataDummyProtection Protection = EKataDummyProtection::None;

    /** 죽으면 Respawn Delay 뒤 처음 생성된 Transform에 다시 생성한다. 스포너가 만든 NPC에서만 동작하며 횟수 제한은 없다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dummy")
    bool bRespawnOnDeath = false;

    /** 사망부터 재생성까지의 시간(초). 시체 제거보다 짧으면 시체와 겹친 채 생성된다. 0이면 다음 틱에 생성한다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dummy",
        meta = (EditCondition = "bRespawnOnDeath", ClampMin = "0.0", UIMin = "0.0", Units = "s"))
    float RespawnDelay = 5.f;
};

/**
 * 더미 NPC 하나의 실행 상태. 스포너의 Dummy 설정이 생성 직후 붙이며 설정은 값으로 복사해 배치가 끝난 뒤에도 유지한다.
 * BeginPlay에서 보호 태그를 붙이고 사망을 구독한다. 죽으면 재생성 설정에 따라 생성한 스포너에 시작 Transform의 재생성을 예약한다.
 * EndPlay에서 붙인 보호 태그와 사망 구독을 정리한다. 스포너는 약한 참조로 들고 있어 스포너가 먼저 사라지면 재생성하지 않는다.
 */
UCLASS(ClassGroup = (Kata), BlueprintType, meta = (DisplayName = "Kata Dummy"))
class KATAFRAMEWORK_API UKataDummyComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UKataDummyComponent();

    /**
     * Character에 더미 컴포넌트를 붙이고 Settings를 적용한다. 시작 Transform은 호출 시점의 캐릭터 Transform이다.
     * Spawner가 null이면 재생성 설정은 동작하지 않는다. 이미 더미 컴포넌트가 있으면 경고 후 기존 컴포넌트를 바꾸지 않고 돌려준다.
     * Character가 유효하지 않으면 null이다.
     */
    static UKataDummyComponent* AddDummyComponent(AKataCharacter* Character, const FKataDummySettings& InSettings, AKataCharacterSpawner* InSpawner);

    /** 붙일 때 복사한 더미 설정. */
    UFUNCTION(BlueprintPure, Category = "Kata|Dummy")
    FKataDummySettings GetDummySettings() const { return Settings; }

    /** 붙일 때 기록한 캐릭터 Transform. 사망 뒤 재생성 위치다. */
    UFUNCTION(BlueprintPure, Category = "Kata|Dummy")
    FTransform GetStartTransform() const { return StartTransform; }

protected:
    //~ Begin UActorComponent Interface
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    //~ End UActorComponent Interface

private:
    void ApplyProtection();
    void RemoveProtection();
    void HandleOwnerDied(UKataDeathComponent* DeathComponent);
    UKataDeathComponent* FindOwnerDeathComponent() const;

    UPROPERTY(VisibleInstanceOnly, Category = "Dummy")
    FKataDummySettings Settings;

    UPROPERTY(VisibleInstanceOnly, Category = "Dummy")
    FTransform StartTransform = FTransform::Identity;

    TWeakObjectPtr<AKataCharacterSpawner> Spawner;

    /** BeginPlay에서 실제로 붙인 보호 태그. EndPlay에서 같은 태그만 뗀다. */
    FGameplayTag AppliedProtectionTag;
};
