#include "Tasks/KataTask_HitTrace.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "HitTrace/KataHitBoxComponent.h"
#include "HitTrace/KataHitBoxPreset.h"
#include "HitTrace/KataHitHandler.h"
#include "HitTrace/KataHitSubsystem.h"
#include "KataFrameworkLog.h"
#include "Runtime/KataActionInstance.h"
#include "TargetingSystem/TargetingPreset.h"
#include "Tasks/TargetingFilterTask_BasicFilterTemplate.h"
#include "Tasks/TargetingTask.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

TSubclassOf<UKataTaskInstance> UKataTask_HitTrace::GetTaskInstanceClass_Implementation() const
{
    return UKataTaskInstance_HitTrace::StaticClass();
}

FName UKataTask_HitTrace::GetConfigurationError() const
{
    const FName SuperError = Super::GetConfigurationError();
    if (!SuperError.IsNone())
    {
        return SuperError;
    }
    if (HitBoxPreset == nullptr)
    {
        return TEXT("MissingHitBoxPreset");
    }
    if (!HitBoxPreset->GetConfigurationError().IsEmpty())
    {
        return TEXT("InvalidHitBoxPreset");
    }
    if (HitHandlers.IsEmpty() || HitHandlers.Contains(nullptr))
    {
        return TEXT("MissingHitHandler");
    }
    for (const UKataHitHandler* Handler : HitHandlers)
    {
        if (!Handler->GetConfigurationError().IsEmpty())
        {
            return TEXT("InvalidHitHandler");
        }
    }
    return NAME_None;
}

FString UKataTask_HitTrace::DescribeConfigurationError(FName ErrorCode) const
{
    if (ErrorCode == TEXT("MissingHitBoxPreset"))
    {
        return TEXT("'Hit Box Preset' is not set");
    }
    if (ErrorCode == TEXT("InvalidHitBoxPreset"))
    {
        return FString::Printf(TEXT("Hit box preset '%s': %s"), *GetNameSafe(HitBoxPreset), *HitBoxPreset->GetConfigurationError());
    }
    if (ErrorCode == TEXT("MissingHitHandler"))
    {
        return TEXT("'Hit Handlers' needs at least one handler and no empty entries, otherwise hits have no effect");
    }
    if (ErrorCode == TEXT("InvalidHitHandler"))
    {
        for (const UKataHitHandler* Handler : HitHandlers)
        {
            const FString HandlerError = Handler != nullptr ? Handler->GetConfigurationError() : FString();
            if (!HandlerError.IsEmpty())
            {
                return HandlerError;
            }
        }
    }
    return Super::DescribeConfigurationError(ErrorCode);
}

#if WITH_EDITOR
EDataValidationResult UKataTask_HitTrace::IsDataValid(FDataValidationContext& Context) const
{
    const EDataValidationResult Result = Super::IsDataValid(Context);
    if (const FTargetingTaskSet* TaskSet = FilterPreset != nullptr ? FilterPreset->GetTargetingTaskSet() : nullptr)
    {
        for (const UTargetingTask* FilterTask : TaskSet->Tasks)
        {
            // 판정 후보를 걸러내기만 해야 한다. Selection은 판정과 무관한 대상을 더하고 Sort는 처리 순서를 바꾼다.
            if (FilterTask != nullptr && !FilterTask->IsA<UTargetingFilterTask_BasicFilterTemplate>())
            {
                Context.AddWarning(FText::FromString(FString::Printf(
                    TEXT("Filter preset '%s' contains '%s', which is not a filter task. Hit Trace expects filter tasks only."),
                    *GetNameSafe(FilterPreset), *FilterTask->GetClass()->GetName())));
            }
        }
    }
    return Result;
}
#endif

void UKataTaskInstance_HitTrace::OnTaskStarted_Implementation()
{
    UKataTask_HitTrace* Definition = Cast<UKataTask_HitTrace>(GetTaskDefinition());
    UWorld* World = GetWorld();
    UKataHitSubsystem* Subsystem = World != nullptr ? World->GetSubsystem<UKataHitSubsystem>() : nullptr;
    if (Definition == nullptr || Definition->HitBoxPreset == nullptr || Subsystem == nullptr)
    {
        FinishTask();
        return;
    }

    const FKataContext Context = GetKataContext();
    AActor* Avatar = Context.GetAvatarActor();
    UKataHitBoxComponent* HitBoxComponent = Avatar != nullptr ? Avatar->FindComponentByClass<UKataHitBoxComponent>() : nullptr;

    UMeshComponent* Mesh = nullptr;
    if (HitBoxComponent != nullptr)
    {
        Mesh = HitBoxComponent->GetHitBoxMesh(Definition->MeshSource);
    }
    else if (Definition->MeshSource == EKataHitBoxMeshSource::Character)
    {
        // 컴포넌트 없이도 캐릭터 본체 판정은 쓸 수 있게 한다. 직전 포즈 기록이 없어 첫 프레임 구간은 시작 시점 판정이 대신한다.
        const ACharacter* Character = Cast<ACharacter>(Avatar);
        Mesh = Character != nullptr ? Character->GetMesh() : nullptr;
    }

    if (Mesh == nullptr)
    {
        // 판정 실패를 성공으로 감추지 않는다. 태스크만 완료하고 타임라인은 계속 진행한다.
        UE_LOG(LogKataFramework, Warning, TEXT("Kata hit trace task '%s' found no %s mesh on '%s'. Add a Kata Hit Box component%s."),
            *GetDisplayName(),
            Definition->MeshSource == EKataHitBoxMeshSource::Weapon ? TEXT("weapon") : TEXT("character"),
            *GetNameSafe(Avatar),
            Definition->MeshSource == EKataHitBoxMeshSource::Weapon ? TEXT(" and register the weapon with Set Weapon Mesh") : TEXT(""));
        FinishTask();
        return;
    }

    FKataHitBoxRegistration Registration;
    Registration.Task = Definition;
    Registration.TaskInstance = this;
    Registration.ActionInstance = GetActionInstance();
    Registration.Mesh = Mesh;
    Registration.HitBoxComponent = HitBoxComponent;
    Registration.InstigatorActor = Avatar;
    Registration.SourceAbilitySystem = Context.ResolveAbilitySystem();
    Registration.StartTime = StartedAtKataTime;
    // 한 프레임 태스크와 순간 태스크는 시작 시점 판정만 한다.
    Registration.EndTime = Definition->bSingleFrame ? StartedAtKataTime : StartedAtKataTime + Definition->Duration;

    HitBoxHandle = Subsystem->RegisterHitBox(Registration);
    if (HitBoxHandle == INDEX_NONE)
    {
        FinishTask();
    }
}

void UKataTaskInstance_HitTrace::OnTaskEnded_Implementation(EKataTaskEndReason Reason)
{
    if (HitBoxHandle != INDEX_NONE)
    {
        if (UWorld* World = GetWorld())
        {
            if (UKataHitSubsystem* Subsystem = World->GetSubsystem<UKataHitSubsystem>())
            {
                // 정상 완료만 종료 시각까지 마지막 판정을 한다. 취소·중단·액션 종료는 판정 없이 정리한다.
                Subsystem->CloseHitBox(HitBoxHandle, Reason == EKataTaskEndReason::Completed);
            }
        }
        HitBoxHandle = INDEX_NONE;
    }

    Super::OnTaskEnded_Implementation(Reason);
}
