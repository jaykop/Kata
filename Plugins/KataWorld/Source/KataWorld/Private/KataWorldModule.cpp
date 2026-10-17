#include "KataWorldModule.h"

#include "Cheats/KataWorldCheatExtension.h"
#include "KataWorldLog.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogKataWorld);

void FKataWorldModule::StartupModule()
{
    CheatManagerCreatedHandle = UKataWorldCheatExtension::RegisterForCheatManagers();
}

void FKataWorldModule::ShutdownModule()
{
    UKataWorldCheatExtension::UnregisterFromCheatManagers(CheatManagerCreatedHandle);
    CheatManagerCreatedHandle.Reset();
}

IMPLEMENT_MODULE(FKataWorldModule, KataWorld)
