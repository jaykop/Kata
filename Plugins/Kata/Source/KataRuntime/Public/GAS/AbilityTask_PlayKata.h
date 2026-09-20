#pragma once

#include "Abilities/Tasks/AbilityTask.h"
#include "CoreMinimal.h"
#include "Definition/KataDefinition.h"
#include "KataRuntimeTypes.h"
#include "Templates/SubclassOf.h"
#include "AbilityTask_PlayKata.generated.h"

class UKataAsset;
class UKataDefinition;
class UKataInstance;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKataAbilityTaskEndedSignature, EKataEndReason, EndReason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKataAbilityTaskFailedSignature, EKataStartResult, FailureReason);

/**
 * Gameplay Ability에서 Kata 하나를 실행하는 Ability Task.
 *
 * Ability의 Avatar와 ASC로 Context를 만들고 소유 액터의 UKataComponent에 실행을 요청한다.
 * Kata의 종료와 Ability의 종료는 별개이며, 이 태스크는 종료 사유만 전달한다.
 */
UCLASS()
class KATARUNTIME_API UAbilityTask_PlayKata : public UAbilityTask
{
    GENERATED_BODY()

public:
    /** Kata 에셋을 실행한다. 실행 상태는 생성되는 UKataInstance가 보관한다. */
    UFUNCTION(BlueprintCallable, Category = "Ability|Tasks",
        meta = (DisplayName = "Play Kata", HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "true"))
    static UAbilityTask_PlayKata* PlayKataAsset(UGameplayAbility* OwningAbility, UKataAsset* Asset, AActor* TargetActor);

    /**
     * @param DefinitionClass 실행할 Kata 정의 클래스.
     * @param TargetActor     조건 평가와 태스크에 전달할 대상. 없어도 된다.
     */
    UFUNCTION(BlueprintCallable, Category = "Ability|Tasks",
        meta = (DisplayName = "Play Kata (Legacy Class)", HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "true", DeprecatedFunction, DeprecationMessage = "Use Play Kata with a Kata asset."))
    static UAbilityTask_PlayKata* PlayKata(UGameplayAbility* OwningAbility, TSubclassOf<UKataDefinition> DefinitionClass, AActor* TargetActor);

    virtual void Activate() override;
    virtual void ExternalCancel() override;

    /** 타임라인이 끝까지 진행된 경우. */
    UPROPERTY(BlueprintAssignable)
    FKataAbilityTaskEndedSignature OnCompleted;

    /** 중단, 취소, 소유자 파괴 등으로 끝난 경우. */
    UPROPERTY(BlueprintAssignable)
    FKataAbilityTaskEndedSignature OnInterrupted;

    /** 시작 자체가 거절된 경우. 사유를 그대로 전달한다. */
    UPROPERTY(BlueprintAssignable)
    FKataAbilityTaskFailedSignature OnFailed;

protected:
    virtual void OnDestroy(bool bInOwnerFinished) override;

private:
    UPROPERTY()
    TObjectPtr<UKataAsset> Asset;

    bool bUseAsset = false;

    UFUNCTION()
    void HandleKataEnded(UKataInstance* Instance, EKataEndReason EndReason);

    UPROPERTY()
    TSubclassOf<UKataDefinition> DefinitionClass;

    UPROPERTY()
    TWeakObjectPtr<AActor> TargetActor;

    UPROPERTY()
    TObjectPtr<UKataInstance> KataInstance;
};
