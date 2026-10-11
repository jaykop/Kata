#pragma once

#include "Action/KataTask.h"
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Runtime/KataTaskInstance.h"
#include "KataTask_AISendTrigger.generated.h"

class UKataCondition;

/**
 * AI가 실행 중인 그래프에 트리거를 보내는 태스크.
 *
 * 플레이어 입력 대신 AI가 전이를 요청할 때 사용한다. 엣지가 Required Action Window Tag를 지정했다면 해당 전이 창이 열려 있어야 한다.
 * 소유 Pawn이 플레이어 조종이면 아무것도 하지 않으므로 PC와 AI가 같은 액션을 공유해도 PC 콤보가 저절로 이어지지 않는다.
 * 기본값은 Single Frame이며 시작한 프레임에 Chance에 따라 전이 시도 여부를 정하고 Condition을 평가한다. 같은 시각·단계의 기본 창 태스크 뒤에 실행되도록 Order Hint는 1이다.
 * Single Frame을 끄고 지속 시간을 지정해도 확률 판정은 시작 때 한 번만 한다. 확률 판정을 통과하면 구간 동안 Condition이 Pass인 시점에 전이를 시도한다.
 * 그래프가 트리거를 받으면 이 태스크는 더 보내지 않는다. 창이 아직 닫혀 있거나 엣지 조건이 맞지 않으면 구간 동안 다시 보낸다.
 * 대상 액션이 쿨다운·태그·조건·차단으로 시작을 거절하면 그 구간에서는 더 보내지 않는다.
 * 갈래가 여럿이면 갈래마다 태스크를 하나씩 둔다. 먼저 받아들여진 트리거로 전이한다.
 */
UCLASS(meta = (DisplayName = "Kata Task: AI Send Trigger", KataTaskCategory = "AI"))
class KATAAI_API UKataTask_AISendTrigger : public UKataTask
{
    GENERATED_BODY()

public:
    UKataTask_AISendTrigger();

    /** 보낼 트리거. 그래프 엣지의 Trigger Event Tag와 같은 태그(또는 그 하위 태그)를 지정한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI Trigger", meta = (Categories = "Trigger"))
    FGameplayTag TriggerTag;

    /**
     * 트리거를 보낼 조건. 비우면 항상 통과한다. 평가에 부작용이 없어야 한다.
     * 조건의 대상은 AI 타게팅의 현재 대상이며, 타게팅 컴포넌트가 없으면 액션 Context의 대상이다.
     * 대상이 사라졌거나 멀어졌을 때 콤보를 끊으려면 여기서 거리 등을 확인한다.
     */
    UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "AI Trigger")
    TObjectPtr<UKataCondition> Condition;

    /** 이 구간에서 전이를 시도할 확률. 시작 때 한 번만 판정하며, 0이면 시도하지 않는다. 1이면 확률 판정을 항상 통과하지만 Condition도 충족해야 한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI Trigger", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
    float Chance = 1.0f;

    virtual TSubclassOf<UKataTaskInstance> GetTaskInstanceClass_Implementation() const override;
    virtual FName GetConfigurationError() const override;
    virtual FString DescribeConfigurationError(FName ErrorCode) const override;
};

/** AI 트리거 태스크의 실행별 상태. 확률 결과와 발신 여부는 공유 정의가 아니라 여기에 둔다. */
UCLASS()
class KATAAI_API UKataTaskInstance_AISendTrigger : public UKataTaskInstance
{
    GENERATED_BODY()

protected:
    virtual void OnTaskStarted_Implementation() override;
    virtual void OnTaskTick_Implementation(float DeltaTime) override;
    virtual void OnTaskEnded_Implementation(EKataTaskEndReason Reason) override;

private:
    /** 조건을 확인하고 트리거를 한 번 시도한다. 그래프가 받아들이거나 대상 액션이 거절하면 bDone을 켠다. */
    void TrySend();

    /** 이 구간에서 더 시도하지 않는다. 플레이어 조종, 확률 실패, 발신 완료, 대상 액션의 시작 거절 시 켠다. */
    bool bDone = false;
};
