#include "Modules/ModuleManager.h"

#include "Framework/Docking/TabManager.h"
#include "HAL/IConsoleManager.h"
#include "SKataGASInspector.h"
#include "Styling/AppStyle.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/SNullWidget.h"

class FKataGASInspectorEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        if (IsRunningCommandlet())
        {
            return;
        }
        FGlobalTabmanager::Get()->RegisterNomadTabSpawner(TabName,
            FOnSpawnTab::CreateRaw(this, &FKataGASInspectorEditorModule::SpawnTab))
            .SetDisplayName(FText::FromString(TEXT("GAS Inspector")))
            .SetTooltipText(FText::FromString(TEXT("Inspect Gameplay Ability System state and ability trigger settings.")))
            .SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("Icons.Search")))
            .SetMenuType(ETabSpawnerMenuType::Hidden);
        UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this,
            &FKataGASInspectorEditorModule::RegisterMenu));
        OpenCommand = MakeUnique<FAutoConsoleCommand>(TEXT("Kata.GASInspector.Open"),
            TEXT("Open the read-only GAS Inspector editor tab."),
            FConsoleCommandDelegate::CreateRaw(this, &FKataGASInspectorEditorModule::Open));
        bRegistered = true;
    }

    virtual void ShutdownModule() override
    {
        if (!bRegistered)
        {
            return;
        }
        OpenCommand.Reset();
        UToolMenus::UnRegisterStartupCallback(this);
        UToolMenus::UnregisterOwner(this);
        if (const TSharedPtr<SKataGASInspector> Widget = Inspector.Pin())
        {
            Widget->Stop();
        }
        if (const TSharedPtr<SDockTab> Tab = LiveTab.Pin())
        {
            // 모듈 코드가 해제된 뒤 Slate 콜백이 남지 않도록 콘텐츠를 먼저 분리한다.
            Tab->SetContent(SNullWidget::NullWidget);
            Tab->RequestCloseTab();
        }
        FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(TabName);
        Inspector.Reset();
        LiveTab.Reset();
        bRegistered = false;
    }

private:
    void Open()
    {
        FGlobalTabmanager::Get()->TryInvokeTab(TabName);
    }

    void RegisterMenu()
    {
        FToolMenuOwnerScoped Owner(this);
        UToolMenu* Menu = UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.MainMenu.Window"));
        Menu->FindOrAddSection(TEXT("WindowLayout")).AddMenuEntry(TEXT("KataGASInspector"),
            FText::FromString(TEXT("GAS Inspector")),
            FText::FromString(TEXT("Inspect Gameplay Ability System state and ability trigger settings.")),
            FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("Icons.Search")),
            FUIAction(FExecuteAction::CreateRaw(this, &FKataGASInspectorEditorModule::Open)));
    }

    TSharedRef<SDockTab> SpawnTab(const FSpawnTabArgs& Args)
    {
        TSharedRef<SDockTab> Tab = SNew(SDockTab).TabRole(ETabRole::NomadTab);
        const TWeakPtr<SDockTab> WeakTab = Tab;
        TSharedRef<SKataGASInspector> Widget = SNew(SKataGASInspector)
            .CanCollect_Lambda([WeakTab]
            {
                const TSharedPtr<SDockTab> Pinned = WeakTab.Pin();
                return Pinned.IsValid() && Pinned->IsForeground();
            });
        Tab->SetContent(Widget);
        Tab->SetOnTabClosed(SDockTab::FOnTabClosedCallback::CreateLambda(
            [WeakWidget = TWeakPtr<SKataGASInspector>(Widget)](TSharedRef<SDockTab> ClosedTab)
            {
                if (const TSharedPtr<SKataGASInspector> Pinned = WeakWidget.Pin())
                {
                    Pinned->Stop();
                }
            }));
        Inspector = Widget;
        LiveTab = Tab;
        return Tab;
    }

    const FName TabName = TEXT("KataGASInspector");
    TUniquePtr<FAutoConsoleCommand> OpenCommand;
    TWeakPtr<SDockTab> LiveTab;
    TWeakPtr<SKataGASInspector> Inspector;
    bool bRegistered = false;
};

IMPLEMENT_MODULE(FKataGASInspectorEditorModule, KataGASInspectorEditor)
