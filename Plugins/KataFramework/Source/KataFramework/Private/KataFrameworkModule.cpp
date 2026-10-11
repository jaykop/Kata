#include "KataFrameworkModule.h"

#include "Cheats/KataFrameworkCheatExtension.h"
#include "KataFrameworkLog.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogKataFramework);

void FKataFrameworkModule::StartupModule()
{
    CheatManagerCreatedHandle = UKataFrameworkCheatExtension::RegisterForCheatManagers();
}

void FKataFrameworkModule::ShutdownModule()
{
    UKataFrameworkCheatExtension::UnregisterFromCheatManagers(CheatManagerCreatedHandle);
    CheatManagerCreatedHandle.Reset();
}

IMPLEMENT_MODULE(FKataFrameworkModule, KataFramework)
