#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "KataRuntimeTypes.h"
#include "Runtime/KataActionInstance.h"
#include "KataActionComponent.generated.h"

class UKataAction;
class UKataActionInstance;
class UKataExecutionWorldSubsystem;
class UKataResolvedAction;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKataActionComponentStartedSignature, UKataActionInstance*, Instance);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FKataActionComponentEndedSignature, UKataActionInstance*, Instance, EKataEndReason, EndReason);

/**
 * 캐릭터별 Kata 인스턴스 관리와 외부 요청 창구.
 *
 * 실행 자체의 상태는 UKataActionInstance가 소유한다. 이 컴포넌트는 시작 허용 판정,
 * 인스턴스 수명 관리, 구동(Tick), 외부 조회와 알림만 담당한다.
 * 초기 구성은 캐릭터당 주 액션 하나이며 다중 액션 채널은 이번 범위가 아니다.
 */
UCLASS(ClassGroup = (Kata), PrioritizeCategories = "Kata", meta = (BlueprintSpawnableComponent, DisplayName = "Kata Action Component"))
class KATARUNTIME_API UKataActionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UKataActionComponent();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    /**
     * 에셋을 해석하고 새 런타임 인스턴스를 만든다. 매 실행마다 부모의 최신 값을 반영한다.
     * 성공 시 GAS 활성 상태를 적용하고 시각 0 태스크를 즉시 실행한다. 첫 Tick의 DeltaTime은 0이다.
     * 길이 0 액션이나 시작 중 종료된 액션은 반환 전에 완료·중단 알림이 발생할 수 있다.
     */
    UFUNCTION(BlueprintCallable, Category = "Kata", meta = (DisplayName = "Play Kata"))
    EKataStartResult PlayKataAction(UKataAction* Asset, const FKataContext& Context, UKataActionInstance*& OutInstance);

    /**
     * 지정한 활성 인스턴스에서 다음 액션으로 전이하는 C++ 실행기 전용 경로다.
     * ExpectedInstance가 실행 중인 인스턴스와 일치할 때만 액션 차단 정책을 우회한다.
     * 판정·초기화 성공 후 기존 액션을 Branched로 종료하며, 거절되면 기존 액션을 유지한다.
     * null이면 일반 시작 정책을 적용한다. 시작·종료 알림은 반환 전에 발생할 수 있다.
     */
    EKataStartResult PlayKataActionTransition(UKataAction* Asset, const FKataContext& Context,
        UKataActionInstance* ExpectedInstance, UKataActionInstance*& OutInstance);

    UFUNCTION(BlueprintCallable, Category = "Kata", meta = (DisplayName = "Play Kata On Self"))
    EKataStartResult PlayKataActionOnSelf(UKataAction* Asset, AActor* TargetActor, UKataActionInstance*& OutInstance);

    UFUNCTION(BlueprintCallable, Category = "Kata", meta = (DisplayName = "Can Play Kata"))
    EKataStartResult CanPlayKataAction(UKataAction* Asset, const FKataContext& Context) const;

    /** 현재 실행 중인 Kata를 중단한다. */
    UFUNCTION(BlueprintCallable, Category = "Kata")
    void StopKata(EKataEndReason Reason);

    /**
     * 현재 Kata의 캔슬 창이 이 태그를 받아들이면 Kata를 Cancelled로 끝낸다.
     *
     * 캔슬 창은 Cancel Window 태스크가 연다. 태그는 정확히 같아야 하며, bNewPress가 false인 요청(누르고 있는 입력)은
     * Cancel While Held를 켠 창만 받는다. 그래프가 실행한 액션이면 그래프도 함께 끝난다.
     * 캔슬 뒤의 동작(이동, 점프 등)은 호출자가 처리한다.
     *
     * @return Kata를 캔슬했으면 true. 실행 중인 Kata가 없거나 창이 받아들이지 않으면 false.
     */
    UFUNCTION(BlueprintCallable, Category = "Kata")
    bool TryCancelKata(UPARAM(meta = (Categories = "Window.Cancel")) FGameplayTag CancelTag, bool bNewPress = true);

    UFUNCTION(BlueprintPure, Category = "Kata")
    UKataActionInstance* GetActiveInstance() const { return ActiveInstance; }

    UFUNCTION(BlueprintPure, Category = "Kata")
    bool IsPlayingKata() const;

    /** 현재 실행 중인 Kata의 KataTags를 반환한다. 없으면 빈 컨테이너다. */
    UFUNCTION(BlueprintPure, Category = "Kata")
    FGameplayTagContainer GetActiveKataTags() const;

    UPROPERTY(BlueprintAssignable, Category = "Kata")
    FKataActionComponentStartedSignature OnKataStarted;

    UPROPERTY(BlueprintAssignable, Category = "Kata")
    FKataActionComponentEndedSignature OnKataEnded;

    /**
     * 같은 월드에서 여러 Kata가 실행될 때 이 컴포넌트의 처리 순서.
     * 값이 작은 컴포넌트가 먼저 진행되고, 같으면 실행 시작 순서를 따른다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Execution")
    int32 ExecutionPriority = 0;

private:
    EKataStartResult CanStartResolved(const UKataResolvedAction* Resolved, const FKataContext& Context,
        const UKataActionInstance* ExpectedInstance = nullptr) const;
    EKataStartResult StartResolved(UKataResolvedAction* Resolved, const FKataContext& Context,
        UKataActionInstance*& OutInstance, UKataActionInstance* ExpectedInstance = nullptr);

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
