#include "KataRuntimeSpawnerLog.h"
#include "KataRuntimeSpawnerTab.h"
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
        RuntimeSpawnerTab = MakeUnique<FKataRuntimeSpawnerTab>();
        UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FProjectKataEditorModule::RegisterMenus));
    }

    virtual void ShutdownModule() override
    {
        UToolMenus::UnRegisterStartupCallback(this);
        UToolMenus::UnregisterOwner(this);
        RuntimeSpawnerTab.Reset();
    }

private:
    void RegisterMenus()
    {
        FToolMenuOwnerScoped OwnerScoped(this);
        UToolMenu* Menu = UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.MainMenu.Tools"));
        FToolMenuSection& Section = Menu->FindOrAddSection(TEXT("KataTools"), NSLOCTEXT("ProjectKataEditor", "KataToolsSection", "Kata"));
        Section.AddMenuEntry(
            TEXT("KataRuntimeSpawner"),
            NSLOCTEXT("ProjectKataEditor", "RuntimeSpawnerLabel", "Kata Runtime Spawner"),
            NSLOCTEXT("ProjectKataEditor", "RuntimeSpawnerTooltip", "Spawn NPCs in front of the player during PIE."),
            FSlateIcon(FAppStyle::GetAppStyleSetName(), "ClassIcon.Character"),
            FUIAction(FExecuteAction::CreateLambda([this]()
            {
                if (RuntimeSpawnerTab.IsValid())
                {
                    RuntimeSpawnerTab->EnableWidget();
                }
            })));
    }

    TUniquePtr<FKataRuntimeSpawnerTab> RuntimeSpawnerTab;
};

IMPLEMENT_MODULE(FProjectKataEditorModule, ProjectKataEditor);
