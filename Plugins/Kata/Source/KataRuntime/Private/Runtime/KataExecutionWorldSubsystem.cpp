#include "Runtime/KataExecutionWorldSubsystem.h"

#include "Engine/World.h"
#include "Runtime/KataActionInstance.h"

void UKataExecutionWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    PreActorTickHandle = FWorldDelegates::OnWorldPreActorTick.AddUObject(
        this, &UKataExecutionWorldSubsystem::HandleWorldPreActorTick);
}

void UKataExecutionWorldSubsystem::Deinitialize()
{
    if (PreActorTickHandle.IsValid())
    {
        FWorldDelegates::OnWorldPreActorTick.Remove(PreActorTickHandle);
        PreActorTickHandle.Reset();
    }
    RegisteredInstances.Reset();
    Super::Deinitialize();
}

void UKataExecutionWorldSubsystem::RegisterInstance(UKataActionInstance* Instance, int32 Priority)
{
    if (!IsValid(Instance))
    {
        return;
    }

    for (FKataRegisteredInstance& Registered : RegisteredInstances)
    {
        if (Registered.Instance.Get() == Instance)
        {
            if (Registered.Priority != Priority)
            {
                Registered.Priority = Priority;
                bOrderDirty = true;
            }
            return;
        }
    }

    FKataRegisteredInstance& Registered = RegisteredInstances.AddDefaulted_GetRef();
    Registered.Instance = Instance;
    Registered.Priority = Priority;
    Registered.Sequence = NextSequence++;
    bOrderDirty = true;
}

void UKataExecutionWorldSubsystem::UnregisterInstance(UKataActionInstance* Instance)
{
    RegisteredInstances.RemoveAll([Instance](const FKataRegisteredInstance& Registered)
    {
        return !Registered.Instance.IsValid() || Registered.Instance.Get() == Instance;
    });
}

void UKataExecutionWorldSubsystem::HandleWorldPreActorTick(
    UWorld* TickedWorld, ELevelTick TickType, float DeltaSeconds)
{
    if (TickedWorld != GetWorld() || !(DeltaSeconds > 0.0f))
    {
        return;
    }

    RegisteredInstances.RemoveAll([](const FKataRegisteredInstance& Registered)
    {
        return !Registered.Instance.IsValid() || !Registered.Instance->IsRunning();
    });

    if (bOrderDirty)
    {
        RegisteredInstances.Sort([](const FKataRegisteredInstance& Lhs, const FKataRegisteredInstance& Rhs)
        {
            if (Lhs.Priority != Rhs.Priority)
            {
                return Lhs.Priority < Rhs.Priority;
            }
            return Lhs.Sequence < Rhs.Sequence;
        });
        bOrderDirty = false;
    }

    // 콜백에서 등록 목록이 바뀌어도 이번 프레임의 순회 순서는 유지한다.
    const TArray<FKataRegisteredInstance> Snapshot = RegisteredInstances;
    for (const FKataRegisteredInstance& Registered : Snapshot)
    {
        if (UKataActionInstance* Instance = Registered.Instance.Get(); IsValid(Instance) && Instance->IsRunning())
        {
            // 이 콜백의 DeltaSeconds에는 월드 배율만 들어 있다. 몽타주처럼 컴포넌트 Tick으로 진행하는 쪽과
            // 같은 시간으로 진행하도록 액터의 CustomTimeDilation을 곱한다. 0이면 TickInstance가 이번 프레임을 건너뛴다.
            Instance->TickInstance(DeltaSeconds * Instance->GetCustomTimeDilation());
        }
    }
}
