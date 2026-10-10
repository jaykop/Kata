#pragma once

#include "Abilities/Tasks/AbilityTask.h"
#include "CoreMinimal.h"
#include "AbilityTask_KataDimOut.generated.h"

class UMaterialInstanceDynamic;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FKataDimOutFinishedSignature);

/**
 * Avatar의 모든 메시 머티리얼에서 스칼라 파라미터 하나를 지속 시간 동안 0에서 1로 올리는 Ability Task.
 *
 * 시작할 때 Avatar가 소유한 메시 컴포넌트(장비가 런타임에 추가한 메시 포함)를 모아, 이 파라미터를 가진 머티리얼 슬롯만
 * Dynamic Material Instance로 바꾼다. 파라미터가 없는 슬롯은 건드리지 않는다. 진행은 선형이며 이 태스크가 실행되는 동안에만 Tick한다.
 * 사망 시체의 DimOut에 쓰지만 사망 Ability에 묶이지 않으므로 다른 Ability에서도 쓸 수 있다.
 * Ability가 먼저 끝나면 값은 그 시점에서 멈추고 OnFinished는 호출되지 않는다.
 */
UCLASS()
class KATAFRAMEWORK_API UAbilityTask_KataDimOut : public UAbilityTask
{
    GENERATED_BODY()

public:
    UAbilityTask_KataDimOut(const FObjectInitializer& ObjectInitializer);

    /**
     * @param ParameterName 0에서 1로 올릴 머티리얼 스칼라 파라미터 이름.
     * @param Duration 걸리는 시간(초). 0 이하이면 바로 1로 맞추고 끝난다.
     */
    UFUNCTION(BlueprintCallable, Category = "Ability|Tasks",
        meta = (DisplayName = "Kata Dim Out", HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "true"))
    static UAbilityTask_KataDimOut* KataDimOut(UGameplayAbility* OwningAbility, FName ParameterName, float Duration);

    virtual void Activate() override;
    virtual void TickTask(float DeltaTime) override;

    /** 값이 1에 도달했을 때 한 번 호출된다. */
    UPROPERTY(BlueprintAssignable)
    FKataDimOutFinishedSignature OnFinished;

protected:
    virtual void OnDestroy(bool bInOwnerFinished) override;

private:
    void ApplyAlpha(float Alpha);

    FName ParameterName;
    float Duration = 0.0f;
    float Elapsed = 0.0f;

    UPROPERTY()
    TArray<TObjectPtr<UMaterialInstanceDynamic>> Materials;
};
