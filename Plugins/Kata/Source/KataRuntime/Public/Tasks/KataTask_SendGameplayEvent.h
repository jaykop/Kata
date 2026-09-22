#pragma once

#include "Action/KataTask.h"
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "KataRuntimeTypes.h"
#include "Runtime/KataTaskInstance.h"
#include "KataTask_SendGameplayEvent.generated.h"

/**
 * 타임라인의 한 시점에 Gameplay Event를 보내는 태스크.
 *
 * 프로젝트 고유 동작을 위해 태스크를 새로 만들지 않아도 되도록 두는 확장 지점이다.
 * 이벤트를 받는 쪽(Ability의 Trigger나 WaitGameplayEvent)이 무엇을 할지는 Kata가 알지 않는다.
 * 지속 시간을 주더라도 이벤트는 시작 시각에 한 번만 보내고 태스크는 곧바로 완료한다.
 */
UCLASS(meta = (DisplayName = "Kata Task: Send Gameplay Event"))
class KATARUNTIME_API UKataTask_SendGameplayEvent : public UKataTask
{
    GENERATED_BODY()

public:
    /** 보낼 이벤트 태그. 받는 쪽이 같은 태그를 기다린다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event")
    FGameplayTag EventTag;

    /** 이벤트를 받을 대상. 대상의 ASC로 보낸다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event")
    EKataTaskTargetSource EventTarget = EKataTaskTargetSource::Avatar;

    /** 페이로드의 Event Magnitude로 전달할 값. 의미는 받는 쪽이 정한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event")
    float EventMagnitude = 0.0f;

    /** 페이로드의 Optional Object로 전달할 참조. 비워 둘 수 있다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event")
    TObjectPtr<UObject> OptionalObject;

    virtual TSubclassOf<UKataTaskInstance> GetTaskInstanceClass_Implementation() const override;
    virtual FName GetConfigurationError() const override;
    virtual FString DescribeConfigurationError(FName ErrorCode) const override;
};

/**
 * 이벤트 발송의 실행별 상태.
 * 보낸 뒤에 회수할 자원이나 구독이 없으므로 시작에서 모든 일을 끝낸다.
 */
UCLASS()
class KATARUNTIME_API UKataTaskInstance_SendGameplayEvent : public UKataTaskInstance
{
    GENERATED_BODY()

protected:
    virtual void OnTaskStarted_Implementation() override;
};
