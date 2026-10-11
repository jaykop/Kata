#include "KataRuntimeModule.h"

#include "Modules/ModuleManager.h"
#include "Runtime/KataActionTrace.h"

#if WITH_GAMEPLAY_DEBUGGER && KATA_WITH_ACTION_TRACE
#include "Debug/GameplayDebuggerCategory_KataAction.h"
#include "GameplayDebugger.h"

namespace
{
    const FName KataActionDebuggerCategoryName(TEXT("KataAction"));
}
#endif

void FKataRuntimeModule::StartupModule()
{
#if WITH_GAMEPLAY_DEBUGGER && KATA_WITH_ACTION_TRACE
    IGameplayDebugger& GameplayDebugger = IGameplayDebugger::Get();
    GameplayDebugger.RegisterCategory(
        KataActionDebuggerCategoryName,
        IGameplayDebugger::FOnGetCategory::CreateStatic(&FGameplayDebuggerCategory_KataAction::MakeInstance),
        EGameplayDebuggerCategoryState::EnabledInGameAndSimulate);
    GameplayDebugger.NotifyCategoriesChanged();
#endif
}

void FKataRuntimeModule::ShutdownModule()
{
#if WITH_GAMEPLAY_DEBUGGER && KATA_WITH_ACTION_TRACE
    // 엔진 종료 순서에 따라 GameplayDebugger가 먼저 내려갈 수 있으므로 로드되어 있을 때만 해제한다.
    if (IGameplayDebugger::IsAvailable())
    {
        IGameplayDebugger& GameplayDebugger = IGameplayDebugger::Get();
        GameplayDebugger.UnregisterCategory(KataActionDebuggerCategoryName);
        GameplayDebugger.NotifyCategoriesChanged();
    }
#endif
}

IMPLEMENT_MODULE(FKataRuntimeModule, KataRuntime)
