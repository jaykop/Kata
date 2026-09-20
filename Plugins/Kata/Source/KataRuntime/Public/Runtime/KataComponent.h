#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "KataRuntimeTypes.h"
#include "Runtime/KataActionInstance.h"
#include "KataComponent.generated.h"

class UKataAction;
class UKataActionInstance;
class UKataExecutionWorldSubsystem;
class UKataResolvedAction;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKataComponentStartedSignature, UKataActionInstance*, Instance);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FKataComponentEndedSignature, UKataActionInstance*, Instance, EKataEndReason, EndReason);

/**
 * 캐릭터별 Kata 인스턴스 관리와 외부 요청 창구.
 *
 * 실행 자체의 상태는 UKataActionInstance가 소유한다. 이 컴포넌트는 시작 허용 판정,
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
    EKataStartResult PlayKataAction(UKataAction* Asset, const FKataContext& Context, UKataActionInstance*& OutInstance);

    UFUNCTION(BlueprintCallable, Category = "Kata", meta = (DisplayName = "Play Kata On Self"))
    EKataStartResult PlayKataActionOnSelf(UKataAction* Asset, AActor* TargetActor, UKataActionInstance*& OutInstance);

    UFUNCTION(BlueprintCallable, Category = "Kata", meta = (DisplayName = "Can Play Kata"))
    EKataStartResult CanPlayKataAction(UKataAction* Asset, const FKataContext& Context) const;

    /** 현재 실행 중인 Kata를 중단한다. */
    UFUNCTION(BlueprintCallable, Category = "Kata")
    void StopKata(EKataEndReason Reason);

    UFUNCTION(BlueprintPure, Category = "Kata")
    UKataActionInstance* GetActiveInstance() const { return ActiveInstance; }

    UFUNCTION(BlueprintPure, Category = "Kata")
    bool IsPlayingKata() const;

    /** 현재 실행 중인 Kata의 KataTags를 반환한다. 없으면 빈 컨테이너다. */
    UFUNCTION(BlueprintPure, Category = "Kata")
    FGameplayTagContainer GetActiveKataTags() const;

    UPROPERTY(BlueprintAssignable, Category = "Kata")
    FKataComponentStartedSignature OnKataStarted;

    UPROPERTY(BlueprintAssignable, Category = "Kata")
    FKataComponentEndedSignature OnKataEnded;

    /**
     * 같은 월드에서 여러 Kata가 실행될 때 이 컴포넌트의 처리 순서.
     * 값이 작은 컴포넌트가 먼저 진행되고, 같으면 실행 시작 순서를 따른다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Execution")
    int32 ExecutionPriority = 0;

private:
    EKataStartResult CanStartResolved(const UKataResolvedAction* Resolved, const FKataContext& Context) const;
    EKataStartResult StartResolved(UKataResolvedAction* Resolved, const FKataContext& Context, UKataActionInstance*& OutInstance);

    UFUNCTION()
    void HandleInstanceEnded(UKataActionInstance* Instance, EKataEndReason EndReason);

    /** 전달받은 Context의 빈 항목을 이 컴포넌트 기준으로 채운다. */
    FKataContext BuildContext(const FKataContext& InContext) const;

    /** 월드 실행 Subsystem에 인스턴스를 등록하고 성공 여부를 기록한다. */
    void RegisterWithExecutionSubsystem(UKataActionInstance* Instance);

    /** 등록했던 인스턴스를 월드 실행 Subsystem에서 제거한다. */
    void UnregisterFromExecutionSubsystem(UKataActionInstance* Instance);

    UPROPERTY(Transient)
    TObjectPtr<UKataActionInstance> ActiveInstance;

    /** true면 인스턴스 진행은 컴포넌트 Tick이 아니라 월드 Subsystem이 담당한다. */
    bool bUsesWorldExecutionSubsystem = false;
};
