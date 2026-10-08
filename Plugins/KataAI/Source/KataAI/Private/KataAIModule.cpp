#include "KataAIModule.h"

#include "KataAILog.h"
#include "Modules/ModuleManager.h"

#if WITH_GAMEPLAY_DEBUGGER
#include "Debug/GameplayDebuggerCategory_KataAI.h"
#include "GameplayDebugger.h"
#endif

DEFINE_LOG_CATEGORY(LogKataAI);

#if WITH_GAMEPLAY_DEBUGGER
namespace
{
    const FName KataAIDebuggerCategoryName(TEXT("KataAI"));
}
#endif

void FKataAIModule::StartupModule()
{
#if WITH_GAMEPLAY_DEBUGGER
    IGameplayDebugger& GameplayDebugger = IGameplayDebugger::Get();
    GameplayDebugger.RegisterCategory(
        KataAIDebuggerCategoryName,
        IGameplayDebugger::FOnGetCategory::CreateStatic(&FGameplayDebuggerCategory_KataAI::MakeInstance),
        EGameplayDebuggerCategoryState::EnabledInGameAndSimulate);
    GameplayDebugger.NotifyCategoriesChanged();
#endif
}

void FKataAIModule::ShutdownModule()
{
#if WITH_GAMEPLAY_DEBUGGER
    // 엔진 종료 순서에 따라 GameplayDebugger가 먼저 내려갈 수 있으므로 로드되어 있을 때만 해제한다.
    if (IGameplayDebugger::IsAvailable())
    {
        IGameplayDebugger& GameplayDebugger = IGameplayDebugger::Get();
        GameplayDebugger.UnregisterCategory(KataAIDebuggerCategoryName);
        GameplayDebugger.NotifyCategoriesChanged();
    }
#endif
}

IMPLEMENT_MODULE(FKataAIModule, KataAI)
