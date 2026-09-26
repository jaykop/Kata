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

/**
 * Hit Trace가 찾은 히트 한 건을 처리하는 객체.
 *
 * UKataTask_HitTrace가 Instanced 배열로 소유하며, UKataHitSubsystem이 판정이 끝난 뒤 같은 프레임에 제출 순서대로 호출한다.
 * 공유 태스크 에셋의 일부이므로 실행 상태를 멤버에 저장하지 않는다. 데미지 같은 수치는 GE와 Ability가 가진다.
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
 * Effect Context에 HitResult와 출처 태스크를 담는다. 공격한 쪽이나 대상에 ASC가 없으면 경고를 남기고 건너뛴다.
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

    virtual void HandleHit_Implementation(AActor* InstigatorActor, AActor* TargetActor, const FHitResult& HitResult,
        UAbilitySystemComponent* SourceAbilitySystem, const UKataTask* SourceTask) const override;
    virtual FString GetConfigurationError() const override;
};
