#include "KataCameraModule.h"

#include "KataCameraLog.h"
#include "Modules/ModuleManager.h"

#if WITH_GAMEPLAY_DEBUGGER
#include "Debug/GameplayDebuggerCategory_KataCamera.h"
#include "GameplayDebugger.h"
#endif

DEFINE_LOG_CATEGORY(LogKataCamera);

#if WITH_GAMEPLAY_DEBUGGER
namespace
{
    const FName KataCameraDebuggerCategoryName(TEXT("KataCamera"));
}
#endif

void FKataCameraModule::StartupModule()
{
#if WITH_GAMEPLAY_DEBUGGER
    IGameplayDebugger& GameplayDebugger = IGameplayDebugger::Get();
    GameplayDebugger.RegisterCategory(
        KataCameraDebuggerCategoryName,
        IGameplayDebugger::FOnGetCategory::CreateStatic(&FGameplayDebuggerCategory_KataCamera::MakeInstance),
        EGameplayDebuggerCategoryState::EnabledInGameAndSimulate);
    GameplayDebugger.NotifyCategoriesChanged();
#endif
}

void FKataCameraModule::ShutdownModule()
{
#if WITH_GAMEPLAY_DEBUGGER
    // 엔진 종료 순서에 따라 GameplayDebugger가 먼저 내려갈 수 있으므로 로드되어 있을 때만 해제한다.
    if (IGameplayDebugger::IsAvailable())
    {
        IGameplayDebugger& GameplayDebugger = IGameplayDebugger::Get();
        GameplayDebugger.UnregisterCategory(KataCameraDebuggerCategoryName);
        GameplayDebugger.NotifyCategoriesChanged();
    }
#endif
}

IMPLEMENT_MODULE(FKataCameraModule, KataCamera)
