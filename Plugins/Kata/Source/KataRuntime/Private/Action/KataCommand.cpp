#include "Action/KataCommand.h"

#include "Runtime/KataActionInstance.h"

UWorld* UKataCommand::GetWorld() const
{
    // CDO가 nullptr을 돌려줘야 Blueprint 편집기가 이 클래스를 월드 컨텍스트를 가진 객체로 인식한다.
    if (HasAnyFlags(RF_ClassDefaultObject))
    {
        return nullptr;
    }
    return RunningInstance != nullptr ? RunningInstance->GetWorld() : nullptr;
}

void UKataCommand::Run(UKataActionInstance* Instance)
{
    TGuardValue<TObjectPtr<UKataActionInstance>> RunningGuard(RunningInstance, Instance);
    Execute(Instance);
}

void UKataCommand::Execute_Implementation(UKataActionInstance* Instance)
{
}
