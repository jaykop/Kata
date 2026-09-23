#pragma once

#include "CoreMinimal.h"
#include "KataRuntimeTypes.h"
#include "UObject/Object.h"
#include "KataTaskInstance.generated.h"

class UKataActionInstance;
class UKataTask;

/**
 * 실행 한 번에만 속하는 태스크 상태.
 *
 * 공유 태스크 정의(UKataTask)에는 어떤 실행 상태도 저장하지 않는다.
 * 외부 핸들과 델리게이트 구독은 이 객체가 소유하고 종료 시 한 번만 정리한다.
 */
UCLASS(BlueprintType, Blueprintable)
class KATARUNTIME_API UKataTaskInstance : public UObject
{
    GENERATED_BODY()

public:
    virtual UWorld* GetWorld() const override;

    /** 인스턴스 생성 직후 소유자와 정의를 연결한다. */
    void InitializeTaskInstance(UKataActionInstance* InActionInstance, UKataTask* InTaskDefinition);

    /** 스케줄러가 시작 경계를 처리할 때 호출한다. */
    void BeginTask(float InKataTime);

    /** 액션 실행기가 프레임당 최대 한 번 호출한다. DeltaTime은 이번 프레임에서 실행한 구간의 길이다. */
    void TickTask(float DeltaTime, float InKataTime);

    /** 종료 콜백을 한 번만 실행한다. 반복 호출은 무시한다. */
    void EndTask(EKataTaskEndReason Reason);

    /** 시작 시각을 지났지만 완료 의존성 때문에 대기 중임을 기록한다. */
    void MarkWaitingForDependency();

    /**
     * 이번 회차에서 시작하지 않았음을 기록한다.
     * 의존성이 끝내 충족되지 않았거나 Restart On Loop를 꺼 둔 항목이 두 번째 회차를 만난 경우다.
     * 실행 중인 항목은 바꾸지 않는다.
     */
    void MarkSkipped();

    /** 다음 실행을 위해 상태를 초기화한다. 실행 중이면 소유 액션이 먼저 종료해야 한다. */
    void ResetForExecution();

    UFUNCTION(BlueprintPure, Category = "Kata|Task")
    UKataActionInstance* GetActionInstance() const;

    /** 해석된 읽기 전용 정의를 반환한다. 반환된 포인터로 값을 수정하지 않는다. */
    UFUNCTION(BlueprintPure, Category = "Kata|Task")
    UKataTask* GetTaskDefinition() const;

    UFUNCTION(BlueprintPure, Category = "Kata|Task")
    EKataTaskState GetTaskState() const { return TaskState; }

    UFUNCTION(BlueprintPure, Category = "Kata|Task")
    bool IsRunning() const { return TaskState == EKataTaskState::Running; }

    /** 진단용 표시 이름. */
    FString GetDisplayName() const;

    FKataTaskId GetTaskId() const;

protected:
    /** 태스크 시작 처리. 외부 재생·구독은 여기서 시작한다. */
    UFUNCTION(BlueprintNativeEvent, Category = "Kata|Task", meta = (BlueprintProtected))
    void OnTaskStarted();
    virtual void OnTaskStarted_Implementation();

    /** 프레임당 최대 한 번 호출된다. 액션의 최초 시작, 프레임 끝의 시작, Single Frame에서는 DeltaTime이 0일 수 있다. */
    UFUNCTION(BlueprintNativeEvent, Category = "Kata|Task", meta = (BlueprintProtected))
    void OnTaskTick(float DeltaTime);
    virtual void OnTaskTick_Implementation(float DeltaTime);

    /** 획득한 자원과 구독을 정리한다. 어떤 사유로 끝나도 한 번만 호출된다. */
    UFUNCTION(BlueprintNativeEvent, Category = "Kata|Task", meta = (BlueprintProtected))
    void OnTaskEnded(EKataTaskEndReason Reason);
    virtual void OnTaskEnded_Implementation(EKataTaskEndReason Reason);

    /** 지속 시간보다 먼저 완료를 알린다. 소유 인스턴스가 후속 의존성을 확인한다. */
    UFUNCTION(BlueprintCallable, Category = "Kata|Task", meta = (BlueprintProtected))
    void FinishTask();

    /** 실행 Context의 사본을 반환한다. 태스크가 공유 상태를 변경하지 못하도록 값으로 전달한다. */
    UFUNCTION(BlueprintPure, Category = "Kata|Task", meta = (BlueprintProtected))
    FKataContext GetKataContext() const;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "Kata|Task")
    TObjectPtr<UKataActionInstance> ActionInstance;

    /** 해석된 읽기 전용 정의. 이 객체를 통해 값을 수정하지 않는다. */
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Kata|Task")
    TObjectPtr<UKataTask> TaskDefinition;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "Kata|Task")
    EKataTaskState TaskState = EKataTaskState::Pending;

    /** 실제로 시작한 Kata 시각. 의존성 대기로 지연되면 StartTime보다 늦을 수 있다. */
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Kata|Task")
    float StartedAtKataTime = 0.0f;

private:
    /** 종료 콜백 중복 실행을 막는다. */
    bool bEndHandled = false;
};
