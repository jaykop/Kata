#include "Tasks/KataTask_TransitionWindow.h"

#include "Runtime/KataActionInstance.h"

UKataTask_TransitionWindow::UKataTask_TransitionWindow()
{
    // 창은 구간을 차지해야 의미가 있다. 기본값으로 짧은 구간을 준다.
    Duration = 0.2f;
}

TSubclassOf<UKataTaskInstance> UKataTask_TransitionWindow::GetTaskInstanceClass_Implementation() const
{
    return UKataTaskInstance_TransitionWindow::StaticClass();
}

FName UKataTask_TransitionWindow::GetConfigurationError() const
{
    if (!WindowTag.IsValid())
    {
        return TEXT("MissingWindowTag");
    }
    if (bSingleFrame)
    {
        // 한 프레임 창은 열자마자 닫혀 어떤 전이도 성립시키지 못한다.
        return TEXT("SingleFrameWindow");
    }
    if (!(Duration > 0.0f))
    {
        return TEXT("ZeroLengthWindow");
    }
    if (PreAcceptSeconds < 0.0f || !FMath::IsFinite(PreAcceptSeconds))
    {
        return TEXT("InvalidPreAcceptSeconds");
    }
    return Super::GetConfigurationError();
}

FString UKataTask_TransitionWindow::DescribeConfigurationError(FName ErrorCode) const
{
    if (ErrorCode == TEXT("MissingWindowTag"))
    {
        return TEXT("'Window Tag' must be set so an edge can require this window");
    }
    if (ErrorCode == TEXT("SingleFrameWindow"))
    {
        return TEXT("'Single Frame' cannot be used for a transition window; give it a duration instead");
    }
    if (ErrorCode == TEXT("ZeroLengthWindow"))
    {
        return TEXT("'Duration' must be greater than zero or the window never opens");
    }
    if (ErrorCode == TEXT("InvalidPreAcceptSeconds"))
    {
        return TEXT("'Pre Accept Seconds' must be zero or greater");
    }
    return Super::DescribeConfigurationError(ErrorCode);
}

void UKataTaskInstance_TransitionWindow::OnTaskStarted_Implementation()
{
    Super::OnTaskStarted_Implementation();

    const UKataTask_TransitionWindow* Definition = Cast<UKataTask_TransitionWindow>(GetTaskDefinition());
    UKataActionInstance* Instance = GetActionInstance();
    if (Definition == nullptr || Instance == nullptr)
    {
        return;
    }

    OpenedWindowTag = Definition->WindowTag;
    Instance->OpenTransitionWindow(OpenedWindowTag, Definition->PreAcceptSeconds);
}

void UKataTaskInstance_TransitionWindow::OnTaskEnded_Implementation(EKataTaskEndReason Reason)
{
    if (OpenedWindowTag.IsValid())
    {
        if (UKataActionInstance* Instance = GetActionInstance())
        {
            Instance->CloseTransitionWindow(OpenedWindowTag);
        }
        OpenedWindowTag = FGameplayTag();
    }

    Super::OnTaskEnded_Implementation(Reason);
}
