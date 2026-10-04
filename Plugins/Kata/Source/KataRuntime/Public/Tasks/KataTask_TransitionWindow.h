#pragma once

#include "Action/KataTask.h"
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Runtime/KataTaskInstance.h"
#include "KataTask_TransitionWindow.generated.h"

/** 전이 창 태스크가 여는 창 하나의 설정. */
USTRUCT(BlueprintType)
struct KATARUNTIME_API FKataTransitionWindowEntry
{
    GENERATED_BODY()

    /**
     * 이 구간 동안 열리는 창의 이름. 엣지의 Required Action Window Tag와 정확히 같아야 한다.
     * 전이 창 태그는 Window.Transition 루트 아래에 두는 것이 Kata의 규약이며, Categories는 에디터 선택 목록만 거른다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition", meta = (Categories = "Window.Transition"))
    FGameplayTag WindowTag;

    /**
     * 창이 열리기 이만큼 전에 도착한 트리거까지 받아들인다.
     *
     * 0이면 창이 열린 뒤에 도착한 입력만 받는다. 패링처럼 타이밍 자체가
     * 메커니즘인 전이는 0으로 둔다. 모션이 길수록 큰 값이 필요하다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition", meta = (ClampMin = "0.0", Units = "s"))
    float PreAcceptSeconds = 0.0f;
};

/**
 * 타임라인의 한 구간 동안 이름 붙은 전이 창을 여는 태스크.
 *
 * 그래프의 엣지가 같은 태그를 요구하면 이 구간에서만 전이가 성립한다.
 * 액션은 어디로 가는지 모르고 그래프는 몇 초인지 모르므로 서로 의존하지 않는다.
 * 구간은 UKataTask의 Start Time과 Duration을 그대로 쓰며, 한 태스크가 설정이 다른 창 여러 개를 같은 구간에 연다.
 */
UCLASS(meta = (DisplayName = "Kata Task: Transition Window"))
class KATARUNTIME_API UKataTask_TransitionWindow : public UKataTask
{
    GENERATED_BODY()

public:
    UKataTask_TransitionWindow();

    /** 이 구간 동안 열 창 목록. 같은 태그를 두 번 넣을 수 없다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition", meta = (TitleProperty = "WindowTag"))
    TArray<FKataTransitionWindowEntry> Windows;

    virtual TSubclassOf<UKataTaskInstance> GetTaskInstanceClass_Implementation() const override;
    virtual FName GetConfigurationError() const override;
    virtual FString DescribeConfigurationError(FName ErrorCode) const override;
};

/** 창 태스크의 실행별 상태. 시작과 종료에서 소유 액션 인스턴스의 창 목록을 갱신한다. */
UCLASS()
class KATARUNTIME_API UKataTaskInstance_TransitionWindow : public UKataTaskInstance
{
    GENERATED_BODY()

protected:
    virtual void OnTaskStarted_Implementation() override;
    virtual void OnTaskEnded_Implementation(EKataTaskEndReason Reason) override;

private:
    /** 시작 때 연 태그를 기억해 종료에서 같은 태그만 닫는다. */
    TArray<FGameplayTag> OpenedWindowTags;
};
