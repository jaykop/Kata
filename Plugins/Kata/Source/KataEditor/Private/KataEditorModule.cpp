#include "KataEditorModule.h"

#include "Action/KataActionTemplate.h"
#include "ContentBrowserMenuContexts.h"
#include "KataActionFactory.h"
#include "Modules/ModuleManager.h"
#include "ToolMenus.h"

#define LOCTEXT_NAMESPACE "KataEditorModule"

IMPLEMENT_MODULE(FKataEditorModule, KataEditor)

void FKataEditorModule::StartupModule()
{
    UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FKataEditorModule::RegisterMenus));
}

void FKataEditorModule::ShutdownModule()
{
    UToolMenus::UnRegisterStartupCallback(this);
    UToolMenus::UnregisterOwner(this);
}

void FKataEditorModule::RegisterMenus()
{
    FToolMenuOwnerScoped OwnerScoped(this);
    // Content Browser는 에셋 클래스 이름으로 우클릭 메뉴를 만든다.
    const FName MenuName(*(FString(TEXT("ContentBrowser.AssetContextMenu.")) + UKataActionTemplate::StaticClass()->GetName()));
    UToolMenu* Menu = UToolMenus::Get()->ExtendMenu(MenuName);
    if (Menu == nullptr)
    {
        return;
    }

    FToolMenuSection& Section = Menu->FindOrAddSection("GetAssetActions");
    Section.AddDynamicEntry("KataCreateChildAction", FNewToolMenuSectionDelegate::CreateLambda([](FToolMenuSection& InSection)
    {
        const UContentBrowserAssetContextMenuContext* Context = InSection.FindContext<UContentBrowserAssetContextMenuContext>();
        // 자식은 Template 하나를 기준으로 만들므로 하나만 선택했을 때만 항목을 보여 준다.
        if (Context == nullptr || Context->SelectedAssets.Num() != 1
            || !Context->SelectedAssets[0].IsInstanceOf(UKataActionTemplate::StaticClass()))
        {
            return;
        }

        const FAssetData TemplateAsset = Context->SelectedAssets[0];
        InSection.AddMenuEntry(
            "KataCreateChildAction",
            LOCTEXT("CreateChildActionLabel", "Create Child Action"),
            LOCTEXT("CreateChildActionTooltip", "Creates a new Kata Action that uses this template as its parent."),
            FSlateIcon(),
            FUIAction(FExecuteAction::CreateLambda([TemplateAsset]()
            {
                UKataActionFactory::CreateChildWithDialog(Cast<UKataActionTemplate>(TemplateAsset.GetAsset()));
            })));
    }));
}

FName KataEditor::GetPreviewViewportToolbarMenuName()
{
    return TEXT("KataActionEditor.ViewportToolbar");
}

#undef LOCTEXT_NAMESPACE
