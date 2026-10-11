#include "HAL/IConsoleManager.h"
#include "KataDebugPanel_Spawner.h"
#include "KataDebugTab.h"
#include "KataRuntimeSpawnerLog.h"
#include "Modules/ModuleManager.h"
#include "Styling/AppStyle.h"
#include "ToolMenus.h"

DEFINE_LOG_CATEGORY(LogKataRuntimeSpawner);

/** 샘플 프로젝트 전용 에디터 도구를 등록한다. 재사용 기능은 Kata 플러그인에 두고 여기에는 샘플 확인용 도구만 둔다. */
class FProjectKataEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        DebugTab = MakeUnique<FKataDebugTab>();

        // 예전 스포너 탭의 명령을 유지한다. Kata Debug 탭을 스포너 패널로 열고, 이미 그 패널이면 닫는다.
        RuntimeSpawnerCommand = MakeUnique<FAutoConsoleCommand>(
            TEXT("Kata.RuntimeSpawner"),
            TEXT("Opens the Kata Debug tab on the Runtime Spawner panel, or closes it if that panel is already open."),
            FConsoleCommandDelegate::CreateRaw(this, &FProjectKataEditorModule::ToggleSpawnerPanel));

        UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FProjectKataEditorModule::RegisterMenus));
    }

    virtual void ShutdownModule() override
    {
        UToolMenus::UnRegisterStartupCallback(this);
        UToolMenus::UnregisterOwner(this);
        RuntimeSpawnerCommand.Reset();
        DebugTab.Reset();
    }

private:
    void RegisterMenus()
    {
        FToolMenuOwnerScoped OwnerScoped(this);
        UToolMenu* Menu = UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.MainMenu.Tools"));
        FToolMenuSection& Section = Menu->FindOrAddSection(TEXT("KataTools"), NSLOCTEXT("ProjectKataEditor", "KataToolsSection", "Kata"));
        Section.AddMenuEntry(
            TEXT("KataDebug"),
            NSLOCTEXT("ProjectKataEditor", "KataDebugLabel", "Kata Debug"),
            NSLOCTEXT("ProjectKataEditor", "KataDebugTooltip", "Cheats and runtime spawner for the PIE world."),
            FSlateIcon(FAppStyle::GetAppStyleSetName(), "ClassIcon.Character"),
            FUIAction(FExecuteAction::CreateLambda([this]()
            {
                if (DebugTab.IsValid())
                {
                    DebugTab->EnableWidget();
                }
            })));
    }

    void ToggleSpawnerPanel()
    {
        if (DebugTab.IsValid())
        {
            DebugTab->TogglePanel(UKataDebugPanel_Spawner::StaticClass());
        }
    }

    TUniquePtr<FKataDebugTab> DebugTab;
    TUniquePtr<FAutoConsoleCommand> RuntimeSpawnerCommand;
};

IMPLEMENT_MODULE(FProjectKataEditorModule, ProjectKataEditor);
