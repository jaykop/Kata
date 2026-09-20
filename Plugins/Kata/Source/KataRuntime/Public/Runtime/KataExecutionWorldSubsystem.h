#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineBaseTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "KataExecutionWorldSubsystem.generated.h"

class UKataActionInstance;

/**
 * 한 월드에서 실행 중인 Kata 인스턴스의 프레임 진행 순서를 조정한다.
 *
 * 인스턴스 내부 태스크 순서는 각 FKataTaskScheduler가 담당한다. 이 Subsystem은
 * 서로 다른 액터의 인스턴스를 Priority와 시작 순번으로 정렬해 한 번씩 진행시킨다.
 */
UCLASS()
class KATARUNTIME_API UKataExecutionWorldSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    /** 활성 인스턴스를 월드 실행 목록에 넣는다. 이미 등록된 인스턴스는 Priority만 갱신한다. */
    void RegisterInstance(UKataActionInstance* Instance, int32 Priority);

    /** 종료된 인스턴스를 월드 실행 목록에서 제거한다. */
    void UnregisterInstance(UKataActionInstance* Instance);

private:
    struct FKataRegisteredInstance
    {
        TWeakObjectPtr<UKataActionInstance> Instance;
        int32 Priority = 0;
        uint64 Sequence = 0;
    };

    /** Actor와 Component Tick 전에 월드의 Kata를 정해진 순서로 진행한다. */
    void HandleWorldPreActorTick(UWorld* TickedWorld, ELevelTick TickType, float DeltaSeconds);

    TArray<FKataRegisteredInstance> RegisteredInstances;
    uint64 NextSequence = 1;
    bool bOrderDirty = false;
    FDelegateHandle PreActorTickHandle;
};
