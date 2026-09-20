#pragma once

#include "CoreMinimal.h"
#include "Action/KataResolvedAction.h"
#include "GameplayEffectTypes.h"
#include "KataRuntimeTypes.h"
#include "Runtime/KataTaskScheduler.h"
#include "UObject/Object.h"
#include "KataActionInstance.generated.h"

class UKataAction;
class UKataResolvedAction;
class UKataTaskInstance;

/** 열려 있는 전이 창 하나의 상태. 실행 중에만 존재한다. */
struct FKataOpenTransitionWindow
{
    /** 창이 열린 월드 시각(초). 트리거 도착 시각과 같은 시계를 쓴다. */
    float OpenedAtWorldSeconds = 0.0f;

    /** 창이 열리기 이만큼 전에 도착한 트리거까지 받아들인다. */
    float PreAcceptSeconds = 0.0f;

    /** 같은 태그를 여는 창 태스크가 겹칠 수 있어 개수를 센다. */
    int32 OpenCount = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FKataInstanceEndedSignature, UKataActionInstance*, Instance, EKataEndReason, EndReason);

/**
 * Kata 실행 한 번을 나타내는 런타임 객체.
 *
 * 현재 시간, 루프 횟수, Context, 활성 태스크, 종료 처리를 이 객체가 소유한다.
 * 원본 액션 에셋과 공유 태스크 설정에는 실행 상태를 저장하지 않는다.
 * 인스턴스 내부 순서는 소유 스케줄러가 담당하고, 여러 인스턴스의 진행 순서는
 * 월드 실행 Subsystem이 조정한다.
 */
UCLASS(BlueprintType)
class KATARUNTIME_API UKataActionInstance : public UObject
{
    GENERATED_BODY()

public:
    virtual UWorld* GetWorld() const override;

    /**
     * 해석된 정의와 Context로 실행을 준비한다. 태스크 인스턴스도 이때 생성한다.
     * 태그·조건·쿨다운 같은 시작 허용 판정은 UKataComponent가 먼저 수행한다.
     */
    EKataStartResult InitializeInstance(UKataResolvedAction* InResolvedDefinition, const FKataContext& InContext);

    /** 시각 0의 경계를 처리하고 실행을 시작한다. GAS 활성 태그와 차단도 여기서 적용한다. */
    void StartInstance();

    /** 월드 실행 Subsystem이 매 프레임 호출한다. Subsystem이 없는 월드에서는 소유 컴포넌트가 호출한다. */
    void TickInstance(float DeltaTime);

    /** 외부에서 종료를 요청한다. 이미 끝났으면 무시한다. */
    UFUNCTION(BlueprintCallable, Category = "Kata|Instance")
    void RequestEnd(EKataEndReason Reason);

    /** 태스크가 스스로 완료를 알렸을 때의 단일 처리 경로. */
    void HandleTaskFinished(UKataTaskInstance* TaskInstance, EKataTaskEndReason Reason);

    UFUNCTION(BlueprintPure, Category = "Kata|Instance")
    float GetCurrentTime() const { return CurrentTime; }

    /** 0부터 세는 현재 반복 회차. */
    UFUNCTION(BlueprintPure, Category = "Kata|Instance")
    int32 GetLoopIteration() const { return LoopIteration; }

    UFUNCTION(BlueprintPure, Category = "Kata|Instance")
    EKataInstanceState GetInstanceState() const { return InstanceState; }

    UFUNCTION(BlueprintPure, Category = "Kata|Instance")
    bool IsRunning() const { return InstanceState == EKataInstanceState::Running; }

    UFUNCTION(BlueprintPure, Category = "Kata|Instance")
    UKataResolvedAction* GetResolvedDefinition() const { return ResolvedDefinition; }

    /** 이 실행의 원본 에셋. */
    UFUNCTION(BlueprintPure, Category = "Kata|Instance")
    UKataAction* GetKataAction() const;

    UFUNCTION(BlueprintPure, Category = "Kata|Instance")
    FKataContext GetContext() const { return Context; }

    UFUNCTION(BlueprintPure, Category = "Kata|Instance")
    float GetTimelineDuration() const { return Scheduler.GetTimelineDuration(); }

    const FKataContext& GetContextRef() const { return Context; }

    /** 실행 중인 태스크 인스턴스 목록의 사본. */
    UFUNCTION(BlueprintPure, Category = "Kata|Instance")
    TArray<UKataTaskInstance*> GetActiveTaskInstances() const;

    /** 전이 창을 연다. 창 태스크가 시작할 때 호출한다. */
    void OpenTransitionWindow(const FGameplayTag& WindowTag, float PreAcceptSeconds);

    /** 전이 창을 닫는다. 같은 태그를 연 태스크가 남아 있으면 열린 상태를 유지한다. */
    void CloseTransitionWindow(const FGameplayTag& WindowTag);

    /** 지금 이 태그의 창이 열려 있는지. */
    UFUNCTION(BlueprintPure, Category = "Kata|Instance")
    bool IsTransitionWindowOpen(const FGameplayTag& WindowTag) const;

    /**
     * 지정한 시각에 도착한 트리거를 이 창이 받아들이는지.
     *
     * 창이 열린 시각보다 PreAcceptSeconds만큼 앞선 입력까지 허용한다.
     * WindowTag가 비어 있으면 액션 실행 중에는 항상 받아들인다.
     */
    bool AcceptsTriggerAt(const FGameplayTag& WindowTag, float TriggerWorldSeconds) const;

    UPROPERTY(BlueprintAssignable, Category = "Kata|Instance")
    FKataInstanceEndedSignature OnKataEnded;

private:
    /**
     * From에서 To까지 시간 경계와 실제 태스크 종료 시각을 순서대로 처리한다.
     * 각 구간에서 활성 태스크를 먼저 Tick하고 같은 시각의 종료 뒤 시작을 처리한다.
     */
    void AdvanceTo(float FromTime, float ToTime, bool bIncludeFromTime);

    /** 시작 경계를 만난 태스크를 시작하거나 완료 의존성 대기로 넘긴다. */
    void TryStartTask(int32 TaskIndex);

    /** 의존성 확인을 마친 태스크를 실제로 시작한다. 순간 태스크는 같은 시각에 종료까지 처리한다. */
    void StartTaskNow(int32 TaskIndex);

    /** 완료 의존성이 모두 충족됐는지 확인한다. */
    bool AreCompletionPrerequisitesMet(int32 TaskIndex) const;

    /**
     * 실제 종료 시각이 지난 실행 중 태스크를 끝낸다.
     * 의존성으로 늦게 시작한 태스크도 자신의 지속 시간만큼 실행하도록 예정 시각이 아닌 실제 시각을 쓴다.
     */
    void FinishElapsedTasks();

    /** 대기 중인 태스크 중 조건이 충족된 것을 시작한다. 재진입 시 중복 실행을 막는다. */
    void TryStartDeferredTasks();

    void TickActiveTasks(float DeltaTime);

    /** 반복 경계에서 활성·대기 태스크를 정리하고 시각 0으로 재진입한다. */
    void BeginNextLoop();

    bool ShouldLoopAgain() const;

    /** 정상 종료, 중단, 취소, 소유자 파괴를 하나의 종료 경로로 처리한다. */
    void EndInstance(EKataEndReason Reason);

    /** 활성 태스크를 모두 끝낸다. 콜백 중 목록이 바뀌어도 안전하도록 사본을 순회한다. */
    void EndActiveTasks(EKataTaskEndReason Reason);

    /** 끝내 시작하지 못한 대기 태스크를 기록하고 비운다. */
    void FlushDeferredTasks();

    void ApplyGasActivationState();
    void RemoveGasActivationState();

    UPROPERTY(Transient)
    TObjectPtr<UKataResolvedAction> ResolvedDefinition;

    /** 스케줄러 실행 인덱스와 1:1로 대응하는 태스크 인스턴스. */
    UPROPERTY(Transient)
    TArray<TObjectPtr<UKataTaskInstance>> TaskInstances;

    UPROPERTY(Transient)
    FKataContext Context;

    FKataTaskScheduler Scheduler;

    /** 실행 중인 태스크의 실행 인덱스. */
    TArray<int32> ActiveTaskIndices;

    /** 완료 의존성 때문에 시작을 미룬 태스크의 실행 인덱스. */
    TArray<int32> DeferredTaskIndices;

    /** 이번 반복에서 정상 완료한 태스크의 실행 인덱스. */
    TSet<int32> CompletedTaskIndices;

    /**
     * 실행 인덱스별 실제 종료 시각. 시작할 때 `실제 시작 시각 + Duration`으로 채운다.
     * 예정 시각과 다를 수 있으므로 경계 목록 대신 이 값으로 종료를 판정한다.
     */
    TArray<float> TaskEndTimes;

    float CurrentTime = 0.0f;

    int32 LoopIteration = 0;

    EKataInstanceState InstanceState = EKataInstanceState::Created;

    /** 콜백 안에서 들어온 종료 요청을 경계 처리 뒤로 미루기 위한 표시. */
    bool bEndRequested = false;

    EKataEndReason PendingEndReason = EKataEndReason::Cancelled;

    /** 대기 태스크 확인의 재진입 표시. */
    bool bResolvingDeferredTasks = false;

    bool bDeferredTasksDirty = false;

    /** GAS 활성 상태를 한 번만 적용·회수하기 위한 표시. */
    bool bGasActivationApplied = false;

    /** 태그별로 열려 있는 전이 창. UObject를 담지 않아 리플렉션이 필요 없다. */
    TMap<FGameplayTag, FKataOpenTransitionWindow> OpenTransitionWindows;

    FActiveGameplayEffectHandle CooldownHandle;
};
