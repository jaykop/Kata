#include "Tasks/KataTask_CancelWindow.h"

#include "Runtime/KataActionInstance.h"

UKataTask_CancelWindow::UKataTask_CancelWindow()
{
    // 창은 구간을 차지해야 의미가 있다. 기본값으로 짧은 구간을 준다.
    Duration = 0.2f;
}

TSubclassOf<UKataTaskInstance> UKataTask_CancelWindow::GetTaskInstanceClass_Implementation() const
{
    return UKataTaskInstance_CancelWindow::StaticClass();
}

FName UKataTask_CancelWindow::GetConfigurationError() const
{
    if (Windows.IsEmpty())
    {
        return TEXT("NoWindows");
    }

    TSet<FGameplayTag> SeenTags;
    for (const FKataCancelWindowEntry& Entry : Windows)
    {
        if (!Entry.CancelTag.IsValid())
        {
            return TEXT("MissingCancelTag");
        }
        bool bAlreadySeen = false;
        SeenTags.Add(Entry.CancelTag, &bAlreadySeen);
        if (bAlreadySeen)
        {
            // 같은 태그의 홀드 설정이 둘이면 어느 값이 적용되는지 에셋만 보고 알 수 없다.
            return TEXT("DuplicateCancelTag");
        }
    }

    if (bSingleFrame)
    {
        // 한 프레임 창은 열자마자 닫혀 입력을 거의 받지 못한다.
        return TEXT("SingleFrameWindow");
    }
    if (!(Duration > 0.0f))
    {
        return TEXT("ZeroLengthWindow");
    }
    return Super::GetConfigurationError();
}

FString UKataTask_CancelWindow::DescribeConfigurationError(FName ErrorCode) const
{
    if (ErrorCode == TEXT("NoWindows"))
    {
        return TEXT("'Windows' must contain at least one entry");
    }
    if (ErrorCode == TEXT("MissingCancelTag"))
    {
        return TEXT("Every entry in 'Windows' must have a 'Cancel Tag'");
    }
    if (ErrorCode == TEXT("DuplicateCancelTag"))
    {
        return TEXT("Each 'Cancel Tag' can appear only once in 'Windows'");
    }
    if (ErrorCode == TEXT("SingleFrameWindow"))
    {
        return TEXT("'Single Frame' cannot be used for a cancel window; give it a duration instead");
    }
    if (ErrorCode == TEXT("ZeroLengthWindow"))
    {
        return TEXT("'Duration' must be greater than zero or the window never opens");
    }
    return Super::DescribeConfigurationError(ErrorCode);
}

void UKataTaskInstance_CancelWindow::OnTaskStarted_Implementation()
{
    Super::OnTaskStarted_Implementation();

    const UKataTask_CancelWindow* Definition = Cast<UKataTask_CancelWindow>(GetTaskDefinition());
    UKataActionInstance* Instance = GetActionInstance();
    if (Definition == nullptr || Instance == nullptr)
    {
        return;
    }

    for (const FKataCancelWindowEntry& Entry : Definition->Windows)
    {
        if (Entry.CancelTag.IsValid())
        {
            Instance->OpenCancelWindow(Entry.CancelTag, Entry.bCancelWhileHeld);
            OpenedWindows.Add(Entry);
        }
    }
}

void UKataTaskInstance_CancelWindow::OnTaskEnded_Implementation(EKataTaskEndReason Reason)
{
    if (UKataActionInstance* Instance = GetActionInstance())
    {
        for (const FKataCancelWindowEntry& Entry : OpenedWindows)
        {
            Instance->CloseCancelWindow(Entry.CancelTag, Entry.bCancelWhileHeld);
        }
    }
    OpenedWindows.Reset();

    Super::OnTaskEnded_Implementation(Reason);
}
