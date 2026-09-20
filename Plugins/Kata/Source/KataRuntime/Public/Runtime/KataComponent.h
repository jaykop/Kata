#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "Definition/KataDefinition.h"
#include "KataRuntimeTypes.h"
#include "Runtime/KataInstance.h"
#include "Templates/SubclassOf.h"
#include "KataComponent.generated.h"

class UKataAsset;
class UKataDefinition;
class UKataInstance;
class UKataResolvedDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKataComponentStartedSignature, UKataInstance*, Instance);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FKataComponentEndedSignature, UKataInstance*, Instance, EKataEndReason, EndReason);

/**
 * 캐릭터별 Kata 인스턴스 관리와 외부 요청 창구.
 *
 * 실행 자체의 상태는 UKataInstance가 소유한다. 이 컴포넌트는 시작 허용 판정,
 * 인스턴스 수명 관리, 구동(Tick), 외부 조회와 알림만 담당한다.
 * 초기 구성은 캐릭터당 주 액션 하나이며 다중 액션 채널은 이번 범위가 아니다.
 */
UCLASS(ClassGroup = (Kata), meta = (BlueprintSpawnableComponent, DisplayName = "Kata Component"))
class KATARUNTIME_API UKataComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UKataComponent();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    /** 에셋을 해석하고 새 런타임 인스턴스를 만든다. 매 실행마다 부모의 최신 값을 반영한다. */
    UFUNCTION(BlueprintCallable, Category = "Kata", meta = (DisplayName = "Play Kata"))
    EKataStartResult PlayKataAsset(UKataAsset* Asset, const FKataContext& Context, UKataInstance*& OutInstance);

    UFUNCTION(BlueprintCallable, Category = "Kata", meta = (DisplayName = "Play Kata On Self"))
    EKataStartResult PlayKataAssetOnSelf(UKataAsset* Asset, AActor* TargetActor, UKataInstance*& OutInstance);

    UFUNCTION(BlueprintCallable, Category = "Kata", meta = (DisplayName = "Can Play Kata"))
    EKataStartResult CanPlayKataAsset(UKataAsset* Asset, const FKataContext& Context) const;

    /**
     * Kata 실행을 요청한다. 거절되면 인스턴스를 만들지 않고 사유를 반환한다.
     *
     * @param DefinitionClass 실행할 정의 클래스.
     * @param Context         실행 Context. OwnerActor가 비어 있으면 이 컴포넌트의 소유 액터를 사용한다.
     * @param OutInstance     성공 시 생성된 인스턴스.
     */
    UFUNCTION(BlueprintCallable, Category = "Kata", meta = (DisplayName = "Play Kata (Legacy Class)", DeprecatedFunction, DeprecationMessage = "Use Play Kata with a Kata asset."))
    EKataStartResult PlayKata(TSubclassOf<UKataDefinition> DefinitionClass, const FKataContext& Context, UKataInstance*& OutInstance);

    /** 소유 액터를 주체로 삼는 간단한 실행 요청. */
    UFUNCTION(BlueprintCallable, Category = "Kata")
    EKataStartResult PlayKataOnSelf(TSubclassOf<UKataDefinition> DefinitionClass, AActor* TargetActor, UKataInstance*& OutInstance);

    /** 현재 실행 중인 Kata를 중단한다. */
    UFUNCTION(BlueprintCallable, Category = "Kata")
    void StopKata(EKataEndReason Reason);

    /** 시작 허용 여부만 확인한다. 상태를 바꾸지 않는다. */
    UFUNCTION(BlueprintCallable, Category = "Kata")
    EKataStartResult CanPlayKata(TSubclassOf<UKataDefinition> DefinitionClass, const FKataContext& Context) const;

    UFUNCTION(BlueprintPure, Category = "Kata")
    UKataInstance* GetActiveInstance() const { return ActiveInstance; }

    UFUNCTION(BlueprintPure, Category = "Kata")
    bool IsPlayingKata() const;

    /** 현재 실행 중인 Kata의 KataTags를 반환한다. 없으면 빈 컨테이너다. */
    UFUNCTION(BlueprintPure, Category = "Kata")
    FGameplayTagContainer GetActiveKataTags() const;

    /**
     * 정의 클래스를 해석해 돌려준다. 결과는 컴포넌트가 캐시한다.
     * 에디터에서 부모 정의를 바꾸면 캐시를 비워야 한다.
     */
    UKataResolvedDefinition* GetOrResolveDefinition(TSubclassOf<UKataDefinition> DefinitionClass);

    /** 해석 캐시를 비운다. */
    UFUNCTION(BlueprintCallable, Category = "Kata")
    void ClearResolvedDefinitionCache();

    UPROPERTY(BlueprintAssignable, Category = "Kata")
    FKataComponentStartedSignature OnKataStarted;

    UPROPERTY(BlueprintAssignable, Category = "Kata")
    FKataComponentEndedSignature OnKataEnded;

private:
    EKataStartResult CanStartResolved(const UKataResolvedDefinition* Resolved, const FKataContext& Context) const;
    EKataStartResult StartResolved(UKataResolvedDefinition* Resolved, const FKataContext& Context, UKataInstance*& OutInstance);

    UFUNCTION()
    void HandleInstanceEnded(UKataInstance* Instance, EKataEndReason EndReason);

    /** 전달받은 Context의 빈 항목을 이 컴포넌트 기준으로 채운다. */
    FKataContext BuildContext(const FKataContext& InContext) const;

    UPROPERTY(Transient)
    TObjectPtr<UKataInstance> ActiveInstance;

    /** 정의 클래스별 해석 결과 캐시. 실행 상태는 담지 않는다. */
    UPROPERTY(Transient)
    TMap<TSubclassOf<UKataDefinition>, TObjectPtr<UKataResolvedDefinition>> ResolvedDefinitionCache;
};
