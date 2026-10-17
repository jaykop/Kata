#include "Time/KataLevelTimeSettings.h"

#include "Time/KataWorldTimeSubsystem.h"

void AKataLevelTimeSettings::BeginPlay()
{
    Super::BeginPlay();

    if (UKataWorldTimeSubsystem* TimeSubsystem = UKataWorldTimeSubsystem::Get(this))
    {
        TimeSubsystem->RegisterLevelSettings(this);
    }
}

void AKataLevelTimeSettings::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // 월드 정리 중에도 Subsystem은 액터 EndPlay 뒤에 해제되므로 여기서 등록을 풀 수 있다.
    if (UKataWorldTimeSubsystem* TimeSubsystem = UKataWorldTimeSubsystem::Get(this))
    {
        TimeSubsystem->UnregisterLevelSettings(this);
    }

    Super::EndPlay(EndPlayReason);
}
