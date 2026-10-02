#include "KataTargetingModule.h"

#include "Faction/KataFactionSettings.h"
#include "GenericTeamAgentInterface.h"
#include "KataTargetingLog.h"
#include "Modules/ModuleManager.h"

#if WITH_GAMEPLAY_DEBUGGER
#include "Debug/GameplayDebuggerCategory_KataTargeting.h"
#include "GameplayDebugger.h"
#endif

DEFINE_LOG_CATEGORY(LogKataTargeting);

#if WITH_GAMEPLAY_DEBUGGER
namespace
{
    const FName KataTargetingDebuggerCategoryName(TEXT("KataTargeting"));
}
#endif

void FKataTargetingModule::StartupModule()
{
    // 설정 객체는 판정 시점에 조회한다. 여기서는 함수만 등록하므로 설정 로드 순서에 영향을 받지 않는다.
    FGenericTeamId::SetAttitudeSolver(&UKataFactionSettings::SolveTeamAttitude);

#if WITH_GAMEPLAY_DEBUGGER
    IGameplayDebugger& GameplayDebugger = IGameplayDebugger::Get();
    GameplayDebugger.RegisterCategory(
        KataTargetingDebuggerCategoryName,
        IGameplayDebugger::FOnGetCategory::CreateStatic(&FGameplayDebuggerCategory_KataTargeting::MakeInstance),
        EGameplayDebuggerCategoryState::EnabledInGameAndSimulate);
    GameplayDebugger.NotifyCategoriesChanged();
#endif
}

void FKataTargetingModule::ShutdownModule()
{
#if WITH_GAMEPLAY_DEBUGGER
    // 엔진 종료 순서에 따라 GameplayDebugger가 먼저 내려갈 수 있으므로 로드되어 있을 때만 해제한다.
    if (IGameplayDebugger::IsAvailable())
    {
        IGameplayDebugger& GameplayDebugger = IGameplayDebugger::Get();
        GameplayDebugger.UnregisterCategory(KataTargetingDebuggerCategoryName);
        GameplayDebugger.NotifyCategoriesChanged();
    }
#endif

    FGenericTeamId::ResetAttitudeSolver();
}

IMPLEMENT_MODULE(FKataTargetingModule, KataTargeting)
