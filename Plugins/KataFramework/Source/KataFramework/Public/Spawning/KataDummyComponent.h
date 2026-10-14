#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "KataDummyComponent.generated.h"

class AKataCharacter;
class AKataCharacterSpawner;
class UKataAction;
class UKataDeathComponent;
class UKataGraph;
struct FGameplayEventData;
struct FHitResult;
struct FOnAttributeChangeData;

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

/** 더미가 플레이어를 대상으로 반복할 행동. */
UENUM(BlueprintType)
enum class EKataDummyRepeatMode : uint8
{
    /** 반복하지 않는다. */
    None,
    /** Repeat Action을 반복한다. 캐릭터에 Kata Action Component가 있어야 한다. */
    Action,
    /** Repeat Graph를 반복한다. 캐릭터에 Kata Graph Component가 있어야 한다. */
    Graph
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

    /**
     * 첫 플레이어 Pawn을 대상으로 반복할 행동. AI를 바꾸지 않으므로 AI Override의 Disable AI와 함께 쓴다.
     * AI가 켜져 있으면 StateTree의 행동과 겹친다. 지정한 에셋이 없거나 필요한 컴포넌트가 없으면 경고 후 반복하지 않는다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dummy|Repeat")
    EKataDummyRepeatMode RepeatMode = EKataDummyRepeatMode::None;

    /** Repeat Mode가 Action일 때 실행할 Action. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dummy|Repeat",
        meta = (EditCondition = "RepeatMode == EKataDummyRepeatMode::Action", EditConditionHides))
    TObjectPtr<UKataAction> RepeatAction;

    /** Repeat Mode가 Graph일 때 실행할 Graph. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dummy|Repeat",
        meta = (EditCondition = "RepeatMode == EKataDummyRepeatMode::Graph", EditConditionHides))
    TObjectPtr<UKataGraph> RepeatGraph;

    /** 자동 진입이 없는 Graph를 시작한 뒤 보낼 Trigger. 비우면 보내지 않으며, 그래도 진입 대기에 머물면 경고 후 Graph를 멈춘다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dummy|Repeat",
        meta = (EditCondition = "RepeatMode == EKataDummyRepeatMode::Graph", EditConditionHides, Categories = "Trigger"))
    FGameplayTag EntryTrigger;

    /** 한 번의 실행이 끝나고(복귀를 켜면 복귀한 뒤) 다음 실행까지 기다리는 시간(초). 첫 실행은 기다리지 않는다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dummy|Repeat",
        meta = (EditCondition = "RepeatMode != EKataDummyRepeatMode::None", ClampMin = "0.0", UIMin = "0.0", Units = "s"))
    float RepeatInterval = 1.f;

    /** 한 번의 실행이 끝나면 시작 Transform으로 순간이동한 뒤 다음 실행을 기다린다. 피격 등으로 중단돼도 끝난 것으로 본다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dummy|Repeat",
        meta = (EditCondition = "RepeatMode != EKataDummyRepeatMode::None"))
    bool bReturnToStart = true;

    /**
     * 머리 위에 마지막 피격의 반응 이벤트 태그·방향·계산 피해·HP를 보인다. 피해는 Ignore Damage로 Health에 반영되지 않아도 계산값을 보인다.
     * 디버그 그리기라 Shipping처럼 디버그 그리기가 빠진 빌드에서는 동작하지 않는다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dummy|Debug")
    bool bShowHitInfo = true;

    /** 마지막 피격 뒤 표시를 유지할 시간(초). 0이면 다음 피격까지 유지한다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dummy|Debug",
        meta = (EditCondition = "bShowHitInfo", ClampMin = "0.0", UIMin = "0.0", Units = "s"))
    float HitInfoDuration = 3.f;
};

/**
 * 더미 NPC 하나의 실행 상태. 스포너의 Dummy 설정이나 런타임 스포너 패널이 생성 직후 붙이며 설정은 값으로 복사해 배치가 끝난 뒤에도 유지한다.
 * BeginPlay에서 보호 태그를 붙이고 사망을 구독한다. 죽으면 재생성 설정에 따라 생성한 스포너에 시작 Transform의 재생성을 예약한다.
 * 반복 설정이 있으면 Tick에서 Action·Graph를 반복하고, 사망하면 반복을 멈춘다. 이미 실행 중인 Action·Graph는 중지하지 않는다.
 * 피격 정보 표시를 켜면 피해 Attribute 변경과 반응 이벤트를 구독해 마지막 피격을 머리 위에 그린다.
 * EndPlay에서 붙인 보호 태그와 사망·피격 구독을 정리한다. 스포너는 약한 참조로 들고 있어 스포너가 먼저 사라지면 재생성하지 않는다.
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

    /** 붙일 때 기록한 캐릭터 Transform. 사망 뒤 재생성 위치이자 반복의 복귀 위치다. */
    UFUNCTION(BlueprintPure, Category = "Kata|Dummy")
    FTransform GetStartTransform() const { return StartTransform; }

    /** 반복 설정이 유효하고 아직 멈추지 않았으면 true다. 대기 중인 상태도 포함한다. */
    UFUNCTION(BlueprintPure, Category = "Kata|Dummy")
    bool IsRepeating() const { return bRepeatActive; }

    //~ Begin UActorComponent Interface
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    //~ End UActorComponent Interface

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

    /** 반복 설정을 검사해 유효하면 Tick을 켠다. 에셋이나 필요한 컴포넌트가 없으면 경고 후 반복하지 않는다. */
    void StartRepeating();
    void StopRepeating();

    /** 한 번 실행을 시작한다. 시작했으면 true다. 거절은 false이며 다음 간격에 다시 시도한다. */
    bool PlayOnce();

    /** 이동을 멈추고 시작 위치·회전으로 순간이동한다. 실패하면 경고 후 제자리에 둔다. */
    void ReturnToStart();

    /** 반복 중이거나 피격 정보를 그려야 하면 Tick을 켜고, 둘 다 아니면 끈다. */
    void UpdateTickEnabled();

    /** 피격 정보 구독과 그리기는 디버그 그리기가 있는 빌드에서만 한다. 없는 빌드에서는 아무것도 하지 않는다. */
    void BindHitInfo();
    void UnbindHitInfo();
    void HandleDamageChanged(const FOnAttributeChangeData& ChangeData);
    void HandleReactionEvent(FGameplayTag EventTag, const FGameplayEventData* Payload);
    /** 피격 하나를 기록한다. 같은 월드 시간의 기록이 있으면 새로 만들지 않고 그 기록에 채운다. */
    void RecordHit(const FHitResult* HitResult, const AActor* Attacker, float Damage, FGameplayTag ReactionTag);
    void DrawHitInfo() const;

    /** 마지막 피격 기록. 피해는 반응만 받은 경우 음수로 둔다. */
    struct FHitInfo
    {
        double Time = -1.0;
        FGameplayTag ReactionTag;
        FString Direction;
        float Damage = -1.f;
    };
    FHitInfo LastHit;
    FDelegateHandle DamageChangedHandle;
    FDelegateHandle ReactionEventHandle;
    /** 구독할 때 쓴 반응 이벤트 태그. 해제할 때 같은 필터가 필요하다. */
    FGameplayTagContainer ReactionEventTags;
    bool bHitInfoActive = false;

    UPROPERTY(VisibleInstanceOnly, Category = "Dummy")
    FKataDummySettings Settings;

    UPROPERTY(VisibleInstanceOnly, Category = "Dummy")
    FTransform StartTransform = FTransform::Identity;

    TWeakObjectPtr<AKataCharacterSpawner> Spawner;

    /** BeginPlay에서 실제로 붙인 보호 태그. EndPlay에서 같은 태그만 뗀다. */
    FGameplayTag AppliedProtectionTag;

    /** 반복 실행 상태. 설정과 달리 캐릭터마다 다르다. */
    float RepeatIdleTime = 0.f;
    bool bRepeatActive = false;
    /** 이 컴포넌트가 시작한 실행이 아직 끝나지 않았으면 true다. 끝남을 한 번만 처리해 복귀를 반복하지 않는다. */
    bool bRepeatRunning = false;
    bool bWarnedEntryTrigger = false;
};
