#pragma once

#include "Action/KataTask.h"
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Runtime/KataTaskInstance.h"
#include "KataTask_CancelWindow.generated.h"

/** 캔슬 창 태스크가 여는 창 하나의 설정. */
USTRUCT(BlueprintType)
struct KATARUNTIME_API FKataCancelWindowEntry
{
    GENERATED_BODY()

    /**
     * 이 구간 동안 받아들이는 캔슬 요청의 이름. UKataActionComponent::TryCancelKata에 전달된 태그와 정확히 같아야 한다.
     * 캔슬 창 태그는 Window.Cancel 루트 아래에 두는 것이 Kata의 규약이며, Categories는 에디터 선택 목록만 거른다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cancel", meta = (Categories = "Window.Cancel"))
    FGameplayTag CancelTag;

    /**
     * true면 창이 열리기 전부터 눌러 두고 있던 입력으로도 캔슬한다. 창이 열린 첫 프레임에 바로 끊긴다.
     * false면 창이 열려 있는 동안 새로 누른 입력만 받는다.
     * 이동처럼 계속 누르는 입력은 true, 점프처럼 한 번 누르는 입력은 false가 자연스럽다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cancel")
    bool bCancelWhileHeld = false;
};

/**
 * 타임라인의 한 구간 동안 입력 등으로 현재 액션을 즉시 끝낼 수 있게 하는 태스크.
 *
 * 그래프 전이와 달리 다음 Kata 액션을 정하지 않는다. 캔슬된 액션은 Cancelled로 끝나고,
 * 그 뒤의 동작(이동, 점프 등)은 캔슬을 요청한 쪽이 처리한다.
 * 구간은 UKataTask의 Start Time과 Duration을 그대로 쓴다.
 */
UCLASS(meta = (DisplayName = "Kata Task: Cancel Window"))
class KATARUNTIME_API UKataTask_CancelWindow : public UKataTask
{
    GENERATED_BODY()

public:
    UKataTask_CancelWindow();

    /** 이 구간 동안 받아들이는 캔슬 목록. 같은 태그를 두 번 넣을 수 없다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cancel", meta = (TitleProperty = "CancelTag"))
    TArray<FKataCancelWindowEntry> Windows;

    virtual TSubclassOf<UKataTaskInstance> GetTaskInstanceClass_Implementation() const override;
    virtual FName GetConfigurationError() const override;
    virtual FString DescribeConfigurationError(FName ErrorCode) const override;
};

/** 캔슬 창 태스크의 실행별 상태. 시작과 종료에서 소유 액션 인스턴스의 캔슬 창 목록을 갱신한다. */
UCLASS()
class KATARUNTIME_API UKataTaskInstance_CancelWindow : public UKataTaskInstance
{
    GENERATED_BODY()

protected:
    virtual void OnTaskStarted_Implementation() override;
    virtual void OnTaskEnded_Implementation(EKataTaskEndReason Reason) override;

private:
    /** 시작 때 연 창을 기억해 종료에서 같은 설정만 닫는다. */
    TArray<FKataCancelWindowEntry> OpenedWindows;
};
