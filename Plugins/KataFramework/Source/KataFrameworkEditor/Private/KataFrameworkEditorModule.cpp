#include "KataFrameworkEditorModule.h"

#include "CoreGlobals.h"
#include "Customizations/KataCharacterSpawnerDetails.h"
#include "Customizations/KataRowIdCustomization.h"
#include "Data/KataDataCollection.h"
#include "Data/KataDataSettings.h"
#include "Data/KataRowId.h"
#include "Spawning/KataCharacterSpawner.h"
#include "EditorViewportClient.h"
#include "Engine/World.h"
#include "HitTrace/KataHitSubsystem.h"
#include "KataEditorModule.h"
#include "Misc/ConfigCacheIni.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "SEditorViewport.h"
#include "ToolMenus.h"
#include "ViewportToolbar/UnrealEdViewportToolbarContext.h"

#define LOCTEXT_NAMESPACE "KataFrameworkEditor"

IMPLEMENT_MODULE(FKataFrameworkEditorModule, KataFrameworkEditor)

namespace
{
    /** 프리뷰 Hit Trace 디버그 선택을 저장하는 에디터 사용자 설정 위치. */
    const TCHAR* EditorSettingsSection = TEXT("KataFrameworkEditor");
    const TCHAR* PreviewDebugModeKey = TEXT("PreviewHitTraceDebugMode");

    /** 메뉴 컨텍스트의 프리뷰 뷰포트가 보여 주는 월드의 Hit Subsystem. 뷰포트가 닫혔으면 nullptr이다. */
    UKataHitSubsystem* FindPreviewHitSubsystem(const TWeakPtr<SEditorViewport>& WeakViewport)
    {
        const TSharedPtr<SEditorViewport> Viewport = WeakViewport.Pin();
        const TSharedPtr<FEditorViewportClient> Client = Viewport.IsValid() ? Viewport->GetViewportClient() : nullptr;
        UWorld* World = Client.IsValid() ? Client->GetWorld() : nullptr;
        return World != nullptr ? World->GetSubsystem<UKataHitSubsystem>() : nullptr;
    }

    void AddDebugModeEntry(FToolMenuSection& Section, const TWeakPtr<SEditorViewport>& WeakViewport,
        EKataHitTraceDebugMode Mode, const FName EntryName, const FText& Label, const FText& Tooltip)
    {
        Section.AddMenuEntry(
            EntryName,
            Label,
            Tooltip,
            FSlateIcon(),
            FUIAction(
                FExecuteAction::CreateLambda([WeakViewport, Mode]()
                {
                    if (UKataHitSubsystem* Subsystem = FindPreviewHitSubsystem(WeakViewport))
                    {
                        Subsystem->SetDebugDrawMode(Mode);
                    }
                    // 에디터를 다시 켜거나 다른 액션 에디터를 열어도 같은 단계로 시작하도록 저장한다.
                    UKataHitSubsystem::SetPreviewDefaultDebugDrawMode(Mode);
                    GConfig->SetInt(EditorSettingsSection, PreviewDebugModeKey, static_cast<int32>(Mode), GEditorPerProjectIni);
                }),
                FCanExecuteAction::CreateLambda([WeakViewport]()
                {
                    return FindPreviewHitSubsystem(WeakViewport) != nullptr;
                }),
                FIsActionChecked::CreateLambda([WeakViewport, Mode]()
                {
                    const UKataHitSubsystem* Subsystem = FindPreviewHitSubsystem(WeakViewport);
                    return Subsystem != nullptr && Subsystem->GetDebugDrawMode() == Mode;
                })),
            EUserInterfaceActionType::RadioButton);
    }
}

void FKataFrameworkEditorModule::StartupModule()
{
    int32 SavedMode = static_cast<int32>(EKataHitTraceDebugMode::Off);
    GConfig->GetInt(EditorSettingsSection, PreviewDebugModeKey, SavedMode, GEditorPerProjectIni);
    UKataHitSubsystem::SetPreviewDefaultDebugDrawMode(static_cast<EKataHitTraceDebugMode>(FMath::Clamp(SavedMode,
        static_cast<int32>(EKataHitTraceDebugMode::Off), static_cast<int32>(EKataHitTraceDebugMode::Detailed))));

    UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FKataFrameworkEditorModule::RegisterMenus));
    RegisterPropertyCustomizations();
}

void FKataFrameworkEditorModule::ShutdownModule()
{
    UToolMenus::UnRegisterStartupCallback(this);
    UToolMenus::UnregisterOwner(this);
    UnregisterPropertyCustomizations();
}

void FKataFrameworkEditorModule::RegisterPropertyCustomizations()
{
    FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
    PropertyModule.RegisterCustomPropertyTypeLayout(FKataCharacterId::StaticStruct()->GetFName(),
        FOnGetPropertyTypeCustomizationInstance::CreateLambda([]()
        {
            return FKataRowIdCustomization::MakeInstance([](TArray<const UDataTable*>& OutTables)
            {
                if (const UKataDataCollection* Collection = UKataDataSettings::Get()->GetDataCollection())
                {
                    Collection->GetCharacterTables(OutTables);
                }
            });
        }));
    PropertyModule.RegisterCustomClassLayout(AKataCharacterSpawner::StaticClass()->GetFName(),
        FOnGetDetailCustomizationInstance::CreateStatic(&FKataCharacterSpawnerDetails::MakeInstance));
    PropertyModule.NotifyCustomizationModuleChanged();
}

void FKataFrameworkEditorModule::UnregisterPropertyCustomizations()
{
    // 에디터 종료 순서에 따라 PropertyEditor가 먼저 내려갔을 수 있다.
    if (FPropertyEditorModule* PropertyModule = FModuleManager::GetModulePtr<FPropertyEditorModule>("PropertyEditor"))
    {
        PropertyModule->UnregisterCustomPropertyTypeLayout(FKataCharacterId::StaticStruct()->GetFName());
        PropertyModule->UnregisterCustomClassLayout(AKataCharacterSpawner::StaticClass()->GetFName());
        PropertyModule->NotifyCustomizationModuleChanged();
    }
}

void FKataFrameworkEditorModule::RegisterMenus()
{
    // 이 모듈이 더한 항목을 종료 시 한꺼번에 지울 수 있도록 소유자를 지정한다.
    FToolMenuOwnerScoped OwnerScoped(this);

    UToolMenu* Toolbar = UToolMenus::Get()->ExtendMenu(KataEditor::GetPreviewViewportToolbarMenuName());
    FToolMenuSection& Section = Toolbar->FindOrAddSection("Right");
    Section.AddEntry(FToolMenuEntry::InitSubMenu(
        "KataHitTraceDebug",
        LOCTEXT("HitTraceDebugMenu", "Hit Trace"),
        LOCTEXT("HitTraceDebugMenuTooltip", "Show Kata Hit Trace areas in this preview"),
        FNewToolMenuDelegate::CreateLambda([](UToolMenu* Submenu)
        {
            const UUnrealEdViewportToolbarContext* Context = Submenu->FindContext<UUnrealEdViewportToolbarContext>();
            if (Context == nullptr)
            {
                return;
            }
            const TWeakPtr<SEditorViewport> WeakViewport = Context->Viewport;
            FToolMenuSection& DebugSection = Submenu->FindOrAddSection("KataHitTraceDebug", LOCTEXT("HitTraceDebugSection", "Hit Trace Debug"));
            AddDebugModeEntry(DebugSection, WeakViewport, EKataHitTraceDebugMode::Off, "KataHitTraceDebugOff",
                LOCTEXT("HitTraceDebugOff", "Off"),
                LOCTEXT("HitTraceDebugOffTooltip", "Do not draw hit areas"));
            AddDebugModeEntry(DebugSection, WeakViewport, EKataHitTraceDebugMode::Area, "KataHitTraceDebugArea",
                LOCTEXT("HitTraceDebugArea", "Hit Area"),
                LOCTEXT("HitTraceDebugAreaTooltip", "Draw the current hit area of each active Hit Trace task"));
            AddDebugModeEntry(DebugSection, WeakViewport, EKataHitTraceDebugMode::Detailed, "KataHitTraceDebugDetailed",
                LOCTEXT("HitTraceDebugDetailed", "Detailed"),
                LOCTEXT("HitTraceDebugDetailedTooltip", "Also draw substep trails, start and end checks, and hit points"));
        })));
}

#undef LOCTEXT_NAMESPACE
