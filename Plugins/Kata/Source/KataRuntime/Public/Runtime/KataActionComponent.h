#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "KataRuntimeTypes.h"
#include "Runtime/KataActionInstance.h"
#include "Runtime/KataActionTrace.h"
#include "KataActionComponent.generated.h"

class UGameplayAbility;
class UKataAction;
class UKataActionInstance;
class UKataExecutionWorldSubsystem;
class UKataResolvedAction;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKataActionComponentStartedSignature, UKataActionInstance*, Instance);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FKataActionComponentEndedSignature, UKataActionInstance*, Instance, EKataEndReason, EndReason);
DECLARE_MULTICAST_DELEGATE_OneParam(FKataActionComponentCancelAllSignature, EKataEndReason);

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

    /**
     * GAS Ability와 Kata 실행을 함께 끝내는 공용 중단 진입점.
     *
     * ASC의 CancelAbilities는 GA 클래스의 Asset Tags로만 대상을 고르므로 직접 재생한 Kata와 그래프에 닿지 않는다.
     * 사망·기절처럼 행동을 멈춰야 하는 코드는 CancelAbilities·StopKata·StopGraph를 따로 부르지 않고 이 함수를 쓴다.
     *
     * 1. ASC의 CancelAbilities를 같은 필터와 IgnoreAbility로 호출한다. GA가 재생하던 Kata는 GA와 함께 끝난다.
     * 2. 실행 중인 Kata의 KataTags가 필터를 통과하면 EndReason으로 끝낸다. IgnoreAbility가 재생한 Kata는 남긴다.
     *    그래프가 재생한 액션이면 그래프도 같은 사유로 끝난다.
     * 3. 두 필터가 모두 비어 있는 전체 중단이면 OnCancelAllRequested를 보낸다. 첫 액션을 기다리는 그래프처럼
     *    실행 중인 액션이 없는 상태를 정리하는 데 쓴다.
     *
     * 필터는 CancelAbilities와 같다. WithTags가 비어 있으면 모두 통과하고, 있으면 하나라도 맞아야 한다.
     * WithoutTags에 하나라도 맞으면 제외한다. 태그는 Ability·Kata 분류 태그 전체가 대상이므로 Categories로 좁히지 않는다.
     * 엔진 CancelAbilities를 직접 부르는 코드(엔진 GE 컴포넌트 포함)는 이 함수를 거치지 않으므로 Kata에 닿지 않는다.
     */
    UFUNCTION(BlueprintCallable, Category = "Kata", meta = (AutoCreateRefTerm = "WithTags,WithoutTags"))
    void CancelAbilitiesAndKata(const FGameplayTagContainer& WithTags, const FGameplayTagContainer& WithoutTags,
        UGameplayAbility* IgnoreAbility = nullptr, EKataEndReason EndReason = EKataEndReason::Interrupted);

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
     * CancelAbilitiesAndKata가 필터 없이 호출되면 Kata 정리가 끝난 뒤 한 번 보낸다. 인자는 종료 사유다.
     * 이 컴포넌트가 모르는 Kata 실행 상태(예: KataGraph의 그래프)를 가진 컴포넌트가 구독해 함께 정리한다.
     */
    FKataActionComponentCancelAllSignature OnCancelAllRequested;

    /**
     * 같은 월드에서 여러 Kata가 실행될 때 이 컴포넌트의 처리 순서.
     * 값이 작은 컴포넌트가 먼저 진행되고, 같으면 실행 시작 순서를 따른다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Execution")
    int32 ExecutionPriority = 0;

#if KATA_WITH_ACTION_TRACE
    /**
     * 이 캐릭터의 실행 이력에 기록 하나를 더한다.
     *
     * 이 컴포넌트가 직접 남기는 액션 기록 외에, 이 컴포넌트로 액션을 실행하는 C++ 실행기(그래프, 액션 인스턴스의 창)가 호출한다.
     * 일련번호, 월드 시각, 프레임은 이 함수가 채운다. Kata.ActionTrace.Enable이 꺼져 있으면 아무것도 하지 않는다.
     * 직전 기록과 내용이 같으면 새 항목 대신 반복 수와 시각만 갱신한다.
     */
    void AddTraceEntry(FKataActionTraceEntry Entry);

    /** 보관 중인 실행 이력을 오래된 것부터 OutEntries에 복사한다. OutEntries의 기존 내용은 지운다. */
    void GetTraceEntries(TArray<FKataActionTraceEntry>& OutEntries) const { TraceBuffer.CopyEntries(OutEntries); }

    /** 기록이 더해지거나 합쳐질 때마다 바뀌는 값. 표시 쪽이 다시 복사할지 판단하는 데 쓴다. */
    uint32 GetTraceSerial() const { return TraceBuffer.GetSerial(); }

    /** 보관 중인 실행 이력을 모두 지운다. */
    void ClearTrace() { TraceBuffer.Reset(); }
#endif

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

    /**
     * 등록했던 인스턴스를 월드 실행 Subsystem에서 제거한다.
     * 실행 방식 플래그와 대체 Tick은 현재 인스턴스가 없거나 Instance가 현재 인스턴스일 때만 되돌린다.
     */
    void UnregisterFromExecutionSubsystem(UKataActionInstance* Instance);

    UPROPERTY(Transient)
    TObjectPtr<UKataActionInstance> ActiveInstance;

    /** true면 인스턴스 진행은 컴포넌트 Tick이 아니라 월드 Subsystem이 담당한다. false인 동안 실행 중이면 컴포넌트 Tick이 켜진다. */
    bool bUsesWorldExecutionSubsystem = false;

#if KATA_WITH_ACTION_TRACE
    /**
     * 시작 요청의 결과를 실행 이력에 남긴다.
     *
     * 거절이면 사유에 맞는 상세(처음 걸린 태그, 차단 종류, 조건 결과)를 이 자리에서 다시 구한다.
     * 시작 판정 경로(CanStartResolved)에 기록 비용을 더하지 않기 위해서다. 조건 평가에는 부작용이 없으므로 다시 평가해도 된다.
     */
    void TraceStartResult(const UKataResolvedAction* Resolved, const FKataContext& Context,
        const UKataActionInstance* ExpectedInstance, EKataStartResult Result, const UKataActionInstance* StartedInstance);

    /** 캐릭터별 실행 이력. 실행 상태라 공유 에셋에 두지 않으며 UObject를 강하게 참조하지 않는다. */
    FKataActionTraceBuffer TraceBuffer;
#endif
};
