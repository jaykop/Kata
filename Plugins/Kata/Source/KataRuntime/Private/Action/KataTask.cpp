#include "Action/KataTask.h"

#include "Runtime/KataTaskInstance.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

TSubclassOf<UKataTaskInstance> UKataTask::GetTaskInstanceClass_Implementation() const
{
    return UKataTaskInstance::StaticClass();
}

bool UKataTask::IsInstant() const
{
    // 한 프레임 태스크는 Tick을 한 번 받아야 하므로 순간 태스크로 보지 않는다.
    return !bSingleFrame && Duration <= UE_KINDA_SMALL_NUMBER;
}

float UKataTask::GetEndTime() const
{
    // 한 프레임 태스크는 타임라인에서 길이를 차지하지 않는다. 종료는 Tick 횟수로 정한다.
    return bSingleFrame ? StartTime : StartTime + FMath::Max(0.0f, Duration);
}

FString UKataTask::GetDisplayName() const
{
    return TaskName.IsNone() ? GetClass()->GetName() : TaskName.ToString();
}

FName UKataTask::GetConfigurationError() const
{
    if (StartTime < 0.0f || !FMath::IsFinite(StartTime))
    {
        return TEXT("InvalidStartTime");
    }
    if (!bSingleFrame && (Duration < 0.0f || !FMath::IsFinite(Duration)))
    {
        return TEXT("InvalidDuration");
    }
    if (UpdateHook == EKataTaskUpdateHook::AfterMeshPose)
    {
        // 라벨만으로 포즈 완료 시점을 보장할 수 없으므로 지원 전까지 오류로 다룬다.
        return TEXT("UnsupportedUpdateHook");
    }

    for (const FKataTaskDependency& Dependency : Dependencies)
    {
        if (!Dependency.TaskId.IsValid())
        {
            return TEXT("InvalidDependencyId");
        }
        if (TaskId.IsValid() && Dependency.TaskId == TaskId)
        {
            return TEXT("SelfDependency");
        }
    }
    return NAME_None;
}

FString UKataTask::DescribeConfigurationError(FName ErrorCode) const
{
    if (ErrorCode == TEXT("InvalidStartTime"))
    {
        return TEXT("'Start Time' must be zero or greater");
    }
    if (ErrorCode == TEXT("InvalidDuration"))
    {
        return TEXT("'Duration' must be zero or greater");
    }
    if (ErrorCode == TEXT("UnsupportedUpdateHook"))
    {
        return TEXT("'Update Hook' After Mesh Pose is not supported yet");
    }
    if (ErrorCode == TEXT("InvalidDependencyId"))
    {
        return TEXT("'Dependencies' contains an empty Task Id");
    }
    if (ErrorCode == TEXT("SelfDependency"))
    {
        return TEXT("'Dependencies' points at this task itself");
    }
    return ErrorCode.ToString();
}

void UKataTask::PostInitProperties()
{
    Super::PostInitProperties();

#if WITH_EDITOR
    // CDO는 편집 대상이 아니므로 ID를 발급하지 않는다. 배열에 배치된 템플릿만 발급한다.
    if (!HasAnyFlags(RF_ClassDefaultObject))
    {
        EnsureTaskId();
    }
#endif
}

#if WITH_EDITOR
void UKataTask::EnsureTaskId()
{
    if (!TaskId.IsValid())
    {
        TaskId = FKataTaskId::NewId();
    }
}

EDataValidationResult UKataTask::IsDataValid(FDataValidationContext& Context) const
{
    EDataValidationResult Result = Super::IsDataValid(Context);

    const FName Error = GetConfigurationError();
    if (!Error.IsNone())
    {
        // 저작 중에는 아직 채우지 않은 값이 있을 수 있으므로 저장 검사에서는 경고로 남긴다.
        Context.AddWarning(FText::Format(
            NSLOCTEXT("KataRuntime", "InvalidTaskConfiguration", "Kata task '{0}' will not run: {1} ({2})"),
            FText::FromString(GetDisplayName()),
            FText::FromString(DescribeConfigurationError(Error)),
            FText::FromName(Error)));
    }

    if (!TaskId.IsValid())
    {
        Context.AddError(FText::Format(
            NSLOCTEXT("KataRuntime", "MissingTaskId", "Kata task has no stable Task Id: {0}"),
            FText::FromString(GetDisplayName())));
        Result = EDataValidationResult::Invalid;
    }

    return Result;
}
#endif
