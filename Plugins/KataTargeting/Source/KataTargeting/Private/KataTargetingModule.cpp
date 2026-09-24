#include "KataTargetingModule.h"

#include "Faction/KataFactionSettings.h"
#include "GenericTeamAgentInterface.h"
#include "KataTargetingLog.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogKataTargeting);

void FKataTargetingModule::StartupModule()
{
    // 설정 객체는 판정 시점에 조회한다. 여기서는 함수만 등록하므로 설정 로드 순서에 영향을 받지 않는다.
    FGenericTeamId::SetAttitudeSolver(&UKataFactionSettings::SolveTeamAttitude);
}

void FKataTargetingModule::ShutdownModule()
{
    FGenericTeamId::ResetAttitudeSolver();
}

IMPLEMENT_MODULE(FKataTargetingModule, KataTargeting)
