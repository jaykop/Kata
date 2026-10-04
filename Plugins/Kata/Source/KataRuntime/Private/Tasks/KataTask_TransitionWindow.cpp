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
    if (Windows.IsEmpty())
    {
        return TEXT("NoWindows");
    }

    TSet<FGameplayTag> SeenTags;
    for (const FKataTransitionWindowEntry& Entry : Windows)
    {
        if (!Entry.WindowTag.IsValid())
        {
            return TEXT("MissingWindowTag");
        }
        bool bAlreadySeen = false;
        SeenTags.Add(Entry.WindowTag, &bAlreadySeen);
        if (bAlreadySeen)
        {
            // 같은 태그의 선행 수용 폭이 둘이면 어느 값이 적용되는지 에셋만 보고 알 수 없다.
            return TEXT("DuplicateWindowTag");
        }
        if (Entry.PreAcceptSeconds < 0.0f || !FMath::IsFinite(Entry.PreAcceptSeconds))
        {
            return TEXT("InvalidPreAcceptSeconds");
        }
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
    return Super::GetConfigurationError();
}

FString UKataTask_TransitionWindow::DescribeConfigurationError(FName ErrorCode) const
{
    if (ErrorCode == TEXT("NoWindows"))
    {
        return TEXT("'Windows' must contain at least one entry");
    }
    if (ErrorCode == TEXT("MissingWindowTag"))
    {
        return TEXT("Every entry in 'Windows' must have a 'Window Tag' so an edge can require it");
    }
    if (ErrorCode == TEXT("DuplicateWindowTag"))
    {
        return TEXT("Each 'Window Tag' can appear only once in 'Windows'");
    }
    if (ErrorCode == TEXT("InvalidPreAcceptSeconds"))
    {
        return TEXT("'Pre Accept Seconds' must be zero or greater");
    }
    if (ErrorCode == TEXT("SingleFrameWindow"))
    {
        return TEXT("'Single Frame' cannot be used for a transition window; give it a duration instead");
    }
    if (ErrorCode == TEXT("ZeroLengthWindow"))
    {
        return TEXT("'Duration' must be greater than zero or the window never opens");
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

    for (const FKataTransitionWindowEntry& Entry : Definition->Windows)
    {
        if (Entry.WindowTag.IsValid())
        {
            Instance->OpenTransitionWindow(Entry.WindowTag, Entry.PreAcceptSeconds);
            OpenedWindowTags.Add(Entry.WindowTag);
        }
    }
}

void UKataTaskInstance_TransitionWindow::OnTaskEnded_Implementation(EKataTaskEndReason Reason)
{
    if (UKataActionInstance* Instance = GetActionInstance())
    {
        for (const FGameplayTag& WindowTag : OpenedWindowTags)
        {
            Instance->CloseTransitionWindow(WindowTag);
        }
    }
    OpenedWindowTags.Reset();

    Super::OnTaskEnded_Implementation(Reason);
}
