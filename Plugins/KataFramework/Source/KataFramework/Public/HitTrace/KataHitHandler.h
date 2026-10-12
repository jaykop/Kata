#pragma once

#include "CoreMinimal.h"
#include "Engine/HitResult.h"
#include "GameplayTagContainer.h"
#include "Templates/SubclassOf.h"
#include "UObject/Object.h"
#include "KataHitHandler.generated.h"

class AActor;
class UAbilitySystemComponent;
class UGameplayEffect;
class UKataTask;
struct FGameplayEffectContextHandle;

/**
 * Hit Trace가 찾은 히트 한 건을 처리하는 객체.
 *
 * UKataTask_HitTrace가 Instanced 배열로 소유하며, UKataHitSubsystem이 판정이 끝난 뒤 같은 프레임에 제출 순서대로 호출한다.
 * 공유 태스크 에셋의 일부이므로 실행 상태를 멤버에 저장하지 않는다. 피해 계산은 GE가 맡고, 처리기는 공격별 계수 같은 SetByCaller 값만 넘긴다.
 * 프로젝트 고유 처리는 Blueprint에서 HandleHit을 구현하거나 C++에서 상속해 추가한다.
 */
UCLASS(Abstract, BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced, CollapseCategories)
class KATAFRAMEWORK_API UKataHitHandler : public UObject
{
    GENERATED_BODY()

public:
    /**
     * 히트 한 건을 처리한다.
     *
     * @param InstigatorActor 공격한 액터(Kata Context의 Avatar).
     * @param TargetActor 맞은 액터. 호출 시점에 유효함이 보장된다.
     * @param HitResult 판정 결과. Component는 맞은 UKataHurtBoxComponent, BoneName은 그 HurtBox가 붙은 소켓(또는 본)이다.
     *        ShapeSweep의 시작 시점 판정처럼 겹침으로 찾은 히트는 bStartPenetrating이 true이고 위치가 판정 도형의 중심이다.
     * @param SourceAbilitySystem 공격한 쪽의 ASC. 없으면 nullptr이다.
     * @param SourceTask 히트를 만든 태스크 정의. 읽기 전용이다.
     */
    UFUNCTION(BlueprintNativeEvent, Category = "Kata|Hit")
    void HandleHit(AActor* InstigatorActor, AActor* TargetActor, const FHitResult& HitResult,
        UAbilitySystemComponent* SourceAbilitySystem, const UKataTask* SourceTask) const;
    virtual void HandleHit_Implementation(AActor* InstigatorActor, AActor* TargetActor, const FHitResult& HitResult,
        UAbilitySystemComponent* SourceAbilitySystem, const UKataTask* SourceTask) const;

    /** 설정 검사. 비어 있으면 유효하다. 소유 태스크의 설정 검사가 호출한다. */
    virtual FString GetConfigurationError() const;
};

/** Gameplay Event를 받을 쪽. */
UENUM(BlueprintType)
enum class EKataHitEventRecipient : uint8
{
    /** 맞은 대상. 피격 반응 Ability를 깨울 때 쓴다. */
    Target,
    /** 공격한 쪽. 공격 Ability가 히트를 받아 GE를 적용할 때 쓴다. */
    Instigator
};

/**
 * 히트마다 Gameplay Event를 보내는 처리기.
 * Payload의 Instigator·Target과 TargetData(HitResult)를 채우고, TargetTags에는 맞은 HurtBox의 HurtBoxTags를 넣는다.
 */
UCLASS(meta = (DisplayName = "Send Gameplay Event"))
class KATAFRAMEWORK_API UKataHitHandler_SendGameplayEvent : public UKataHitHandler
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event")
    FGameplayTag EventTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event")
    EKataHitEventRecipient Recipient = EKataHitEventRecipient::Instigator;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event")
    float EventMagnitude = 0.0f;

    virtual void HandleHit_Implementation(AActor* InstigatorActor, AActor* TargetActor, const FHitResult& HitResult,
        UAbilitySystemComponent* SourceAbilitySystem, const UKataTask* SourceTask) const override;
    virtual FString GetConfigurationError() const override;
};

/**
 * 히트마다 공격한 쪽의 ASC로 대상에게 Gameplay Effect를 적용하는 처리기.
 * Effect Context에 HitResult와 출처 태스크를 담고, Set By Caller Magnitudes를 스펙에 넣는다. 공격한 쪽이나 대상에 ASC가 없으면 경고를 남기고 건너뛴다.
 * GE가 실제로 적용됐으면 맞은 쪽·때린 쪽 연출 Cue를 실행하고, Hit Stop Duration만큼 공격자와 피격자에게 히트스톱을 건다.
 */
UCLASS(meta = (DisplayName = "Apply Gameplay Effect"))
class KATAFRAMEWORK_API UKataHitHandler_ApplyGameplayEffect : public UKataHitHandler
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
    TSubclassOf<UGameplayEffect> EffectClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
    float EffectLevel = 1.0f;

    /**
     * 스펙에 넣을 SetByCaller 값. 피해 GE라면 Kata Combat 설정의 Damage Set By Caller Tag로 공격 계수를 넣는다.
     * 하나의 GE 에셋을 공격마다 다른 값으로 재사용하기 위한 항목이다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect", meta = (Categories = "SetByCaller"))
    TMap<FGameplayTag, float> SetByCallerMagnitudes;

    /**
     * 공격의 Impact 등급. 스펙의 동적 Asset Tag로 넣어 피격 반응과 피격 연출이 등급을 읽게 한다.
     * 비어 있으면 넣지 않으며, 반응 쪽은 Kata Combat 설정의 Default Impact Tag를 쓴다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect", meta = (Categories = "Impact"))
    FGameplayTag ImpactTag;

    /**
     * 피해가 적용됐을 때 공격자와 피격자를 멈추는 시간(초). 0이면 히트스톱을 걸지 않는다.
     * 배율과 Priority는 Kata Combat 설정의 Hit Stop 항목을 쓰고, 요청은 UKataTimeDilationSubsystem에 건다.
     * 시간은 배율의 영향을 받지 않고 일시정지 중에는 멈추는 시계로 재므로 배율과 관계없이 이 시간만큼 유지된다.
     * 무적 등으로 GE 적용이 막히거나 대상에 ASC가 없으면 걸지 않는다.
     * 멈추는 공격자는 InstigatorActor(Kata Context의 Avatar)다. Kata 실행기를 소유한 액터가 Avatar와 다르면 실행기의 타임라인은 멈추지 않는다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit Stop", meta = (ClampMin = "0.0", UIMin = "0.0", Units = "s"))
    float HitStopDuration = 0.0f;

    /**
     * 피해가 적용됐을 때 맞은 쪽에서 실행할 이 공격의 피격 연출 Gameplay Cue.
     * 비어 있으면 Kata Combat 설정에서 Impact Tag에 대응하는 행의 Hit Cue Tag를 쓴다. 같은 등급 안에서 공격마다 피격 연출이 다를 때 지정한다(예: 화염 공격).
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit Cue", meta = (Categories = "GameplayCue"))
    FGameplayTag HitCueTag;

    /**
     * 피해가 적용됐을 때 때린 쪽에서 실행할 이 공격의 타격 연출 Gameplay Cue.
     * 비어 있으면 Kata Combat 설정에서 Impact Tag에 대응하는 행의 Attacker Cue Tag를 쓴다. 무기·스킬마다 타격 연출이 다를 때 지정한다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit Cue", meta = (Categories = "GameplayCue"))
    FGameplayTag AttackerCueTag;

    virtual void HandleHit_Implementation(AActor* InstigatorActor, AActor* TargetActor, const FHitResult& HitResult,
        UAbilitySystemComponent* SourceAbilitySystem, const UKataTask* SourceTask) const override;
    virtual FString GetConfigurationError() const override;

private:
    /**
     * 맞은 쪽 Cue를 피격자에서, 때린 쪽 Cue를 공격자에서 실행한다. 피해 적용이 확인된 뒤에만 부른다.
     * 싱글플레이 전용이라 복제하지 않는 경로로 실행한다.
     */
    void ExecuteHitCues(AActor* InstigatorActor, AActor* TargetActor, const FHitResult& HitResult,
        const FGameplayEffectContextHandle& EffectContext) const;

    /** 공격자와 피격자에게 히트스톱 요청을 건다. 피해 적용이 확인된 뒤에만 부른다. */
    void RequestHitStop(AActor* InstigatorActor, AActor* TargetActor) const;
};
