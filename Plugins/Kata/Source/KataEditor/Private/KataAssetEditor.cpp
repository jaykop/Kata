#include "KataAssetEditor.h"

#include "AssetToolsModule.h"
#include "ClassViewerFilter.h"
#include "ClassViewerModule.h"
#include "Definition/KataAsset.h"
#include "Definition/KataPropertyOverride.h"
#include "Definition/KataResolvedDefinition.h"
#include "Definition/KataTask.h"
#include "Editor.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Commands/GenericCommands.h"
#include "Framework/Commands/UICommandList.h"
#include "Framework/Docking/TabManager.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Framework/MultiBox/MultiBoxExtender.h"
#include "IDetailsView.h"
#include "KataAssetFactory.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/PackageName.h"
#include "PropertyEditorDelegates.h"
#include "PropertyEditorModule.h"
#include "ScopedTransaction.h"
#include "SKataPreviewViewport.h"
#include "SKataTimeline.h"
#include "Styling/AppStyle.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/UnrealType.h"
#include "UObject/Package.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
    const FName PreviewTab(TEXT("Kata.Preview"));
    const FName TimelineTab(TEXT("Kata.Timeline"));
    const FName SettingsTab(TEXT("Kata.Settings"));
    const FName TaskTab(TEXT("Kata.Task"));
    const FName PreviewSettingsTab(TEXT("Kata.PreviewSettings"));
    const TCHAR* EditorSettingsSection = TEXT("KataAssetEditor");

    /** 처음 표시하는 Details 타입만 모두 펼치고 이후에는 사용자가 저장한 상태를 따른다. */
    void ExpandDetailsByDefault(const TSharedPtr<IDetailsView>& DetailsView, FName ViewIdentifier, const UClass* ObjectClass)
    {
        if (!DetailsView.IsValid() || !ObjectClass)
        {
            return;
        }

        FString ClassKey = ObjectClass->GetPathName();
        ClassKey.ReplaceInline(TEXT("/"), TEXT("_"));
        ClassKey.ReplaceInline(TEXT("."), TEXT("_"));
        const FString StateKey = FString::Printf(TEXT("DetailsExpansionInitialized.%s.%s"),
            *ViewIdentifier.ToString(), *ClassKey);
        bool bInitialized = false;
        GConfig->GetBool(EditorSettingsSection, *StateKey, bInitialized, GEditorPerProjectIni);
        if (!bInitialized)
        {
            DetailsView->SetRootExpansionStates(true, true);
            GConfig->SetBool(EditorSettingsSection, *StateKey, true, GEditorPerProjectIni);
            GConfig->Flush(false, GEditorPerProjectIni);
        }
    }

    /** 열려 있는 Kata 에디터끼리 공유하는 태스크 복사본. 에디터를 닫으면 사라진다. */
    TStrongObjectPtr<UKataTask> TaskClipboard;

    class FKataClassFilter : public IClassViewerFilter
    {
    public:
        UClass* BaseClass = nullptr;
        virtual bool IsClassAllowed(const FClassViewerInitializationOptions&, const UClass* Class,
            TSharedRef<FClassViewerFilterFuncs>) override
        {
            return Class->IsChildOf(BaseClass) && !Class->IsChildOf(UKataAsset::StaticClass())
                && !Class->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists);
        }
        virtual bool IsUnloadedClassAllowed(const FClassViewerInitializationOptions&,
            const TSharedRef<const IUnloadedBlueprintData> Data, TSharedRef<FClassViewerFilterFuncs>) override
        {
            return Data->IsChildOf(BaseClass) && !Data->IsChildOf(UKataAsset::StaticClass())
                && !Data->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists);
        }
    };

    /** 프로퍼티가 속한 최상위 멤버. 구조체 내부 값도 같은 기준으로 판단한다. */
    const FProperty& GetRootProperty(const FPropertyAndParent& Info)
    {
        return Info.ParentProperties.Num() > 0 ? *Info.ParentProperties.Last() : Info.Property;
    }

    bool IsPreviewProperty(const FPropertyAndParent& Info)
    {
        const FString Name = GetRootProperty(Info).GetFName().ToString();
        return Name.StartsWith(TEXT("Preview")) || Name.StartsWith(TEXT("bPreview"));
    }

    bool IsSettingsPropertyVisible(const FPropertyAndParent& Info)
    {
        const FName Name = Info.Property.GetFName();
        // 프리뷰 설정은 Preview 탭에서만 편집한다.
        return !IsPreviewProperty(Info)
            && Name != GET_MEMBER_NAME_CHECKED(UKataDefinition, TimelineTasks)
            && Name != GET_MEMBER_NAME_CHECKED(UKataDefinition, TaskOverrides);
    }
}

FKataAssetEditor::~FKataAssetEditor()
{
    SaveEditorSettings();
    FCoreUObjectDelegates::OnObjectPropertyChanged.Remove(PropertyChangedHandle);
    if (GEditor)
    {
        GEditor->UnregisterForUndo(this);
    }
    if (Preview)
    {
        Preview->Stop();
    }
}

void FKataAssetEditor::Init(UKataAsset* InAsset)
{
    Asset = InAsset;
    Asset->SetFlags(RF_Transactional);
    LoadEditorSettings();
    FPropertyEditorModule& Properties = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
    FDetailsViewArgs Args;
    Args.bHideSelectionTip = true;
    Args.NameAreaSettings = FDetailsViewArgs::HideNameArea;
    Args.ExpansionPersistenceMethod = FDetailsViewArgs::EExpansionPersistenceMethod::PersistentPerViewIdentifier;
    Args.ViewIdentifier = TEXT("KataEditor.SettingsDetails");
    SettingsDetails = Properties.CreateDetailView(Args);
    Args.ViewIdentifier = TEXT("KataEditor.TaskDetails");
    TaskDetails = Properties.CreateDetailView(Args);
    Args.ViewIdentifier = TEXT("KataEditor.PreviewDetails");
    PreviewDetails = Properties.CreateDetailView(Args);
    SettingsDetails->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateStatic(&IsSettingsPropertyVisible));
    PreviewDetails->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateStatic(&IsPreviewProperty));
    SettingsDetails->OnFinishedChangingProperties().AddSP(this, &FKataAssetEditor::OnSettingsEdited);
    PreviewDetails->OnFinishedChangingProperties().AddSP(this, &FKataAssetEditor::OnSettingsEdited);
    TaskDetails->OnFinishedChangingProperties().AddSP(this, &FKataAssetEditor::OnTaskEdited);
    SAssignNew(Preview, SKataPreviewViewport)
        .OnTargetMoved(FKataTargetTransformChanged::CreateSP(this, &FKataAssetEditor::ApplyPreviewTargetTransform));
    TimelineCommands = MakeShared<FUICommandList>();
    BindCommands();
    SAssignNew(Timeline, SKataTimeline)
        .OnSelect(FKataSelectTask::CreateSP(this, &FKataAssetEditor::SelectTask))
        .OnMove(FKataMoveTask::CreateSP(this, &FKataAssetEditor::MoveTask))
        .OnSeek(FKataSeekPreview::CreateLambda([this](float Time) { Preview->Seek(Asset, Time); }))
        .OnContextMenu(FKataTimelineMenu::CreateSP(this, &FKataAssetEditor::MakeTimelineContextMenu))
        .CommandList(TimelineCommands)
        .Playhead_Lambda([this]() { return Preview->GetTime(); })
        .ViewDuration_Lambda([this]() { return ViewDuration; })
        .SnapInterval_Lambda([this]() { return SnapInterval; })
        .SnapEnabled_Lambda([this]() { return bSnapEnabled; });
    ExtendToolbar();
    Refresh();
    Preview->ResetScene(Asset);
    GEditor->RegisterForUndo(this);
    PropertyChangedHandle = FCoreUObjectDelegates::OnObjectPropertyChanged.AddRaw(this, &FKataAssetEditor::OnObjectChanged);

    // Preview 월드는 자체 탭에 두고, 프리뷰 설정은 Kata Details 옆의 별도 탭으로 분리한다.
    //
    // 레이아웃 이름은 사용자가 배치한 탭 구성을 EditorLayout에 저장할 때 쓰는 키다.
    // 이름을 바꾸면 저장된 배치를 버리고 아래 기본값으로 되돌아가므로 이 이름은 고정한다.
    // 탭을 추가할 때도 이름을 바꾸지 않는다. 새 탭은 Window 메뉴에서 열 수 있다.
    const TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout("KataAssetEditor_v3")
        ->AddArea(FTabManager::NewPrimaryArea()->SetOrientation(Orient_Vertical)
            ->Split(FTabManager::NewSplitter()->SetSizeCoefficient(0.65f)->SetOrientation(Orient_Horizontal)
                ->Split(FTabManager::NewStack()->SetSizeCoefficient(0.7f)->AddTab(PreviewTab, ETabState::OpenedTab))
                ->Split(FTabManager::NewSplitter()->SetSizeCoefficient(0.3f)->SetOrientation(Orient_Vertical)
                    ->Split(FTabManager::NewStack()->SetSizeCoefficient(0.55f)
                        ->AddTab(SettingsTab, ETabState::OpenedTab)
                        ->AddTab(PreviewSettingsTab, ETabState::OpenedTab)
                        ->SetForegroundTab(SettingsTab))
                    ->Split(FTabManager::NewStack()->SetSizeCoefficient(0.45f)->AddTab(TaskTab, ETabState::OpenedTab))))
            ->Split(FTabManager::NewStack()->SetSizeCoefficient(0.35f)->AddTab(TimelineTab, ETabState::OpenedTab)));
    InitAssetEditor(EToolkitMode::Standalone, TSharedPtr<IToolkitHost>(), TEXT("KataAssetEditor"), Layout, true, true, Asset);
}

void FKataAssetEditor::RegisterTabSpawners(const TSharedRef<FTabManager>& Manager)
{
    FAssetEditorToolkit::RegisterTabSpawners(Manager);
    // Window 메뉴에 등록해 두면 닫은 탭을 레이아웃 초기화 없이 다시 열 수 있다.
    const TSharedRef<FWorkspaceItem> Category = AssetEditorTabsCategory.IsValid()
        ? AssetEditorTabsCategory.ToSharedRef() : Manager->GetLocalWorkspaceMenuRoot();
    Manager->RegisterTabSpawner(PreviewTab, FOnSpawnTab::CreateSP(this, &FKataAssetEditor::SpawnTab))
        .SetDisplayName(NSLOCTEXT("Kata", "PreviewTab", "Preview")).SetGroup(Category);
    Manager->RegisterTabSpawner(TimelineTab, FOnSpawnTab::CreateSP(this, &FKataAssetEditor::SpawnTab))
        .SetDisplayName(NSLOCTEXT("Kata", "TimelineTab", "Timeline")).SetGroup(Category);
    Manager->RegisterTabSpawner(SettingsTab, FOnSpawnTab::CreateSP(this, &FKataAssetEditor::SpawnTab))
        .SetDisplayName(NSLOCTEXT("Kata", "SettingsTab", "Kata Details")).SetGroup(Category);
    Manager->RegisterTabSpawner(TaskTab, FOnSpawnTab::CreateSP(this, &FKataAssetEditor::SpawnTab))
        .SetDisplayName(NSLOCTEXT("Kata", "TaskTab", "Task Details")).SetGroup(Category);
    Manager->RegisterTabSpawner(PreviewSettingsTab, FOnSpawnTab::CreateSP(this, &FKataAssetEditor::SpawnTab))
        .SetDisplayName(NSLOCTEXT("Kata", "PreviewSettingsTab", "Preview Details")).SetGroup(Category);
}

void FKataAssetEditor::UnregisterTabSpawners(const TSharedRef<FTabManager>& Manager)
{
    Manager->UnregisterTabSpawner(PreviewTab);
    Manager->UnregisterTabSpawner(TimelineTab);
    Manager->UnregisterTabSpawner(SettingsTab);
    Manager->UnregisterTabSpawner(TaskTab);
    Manager->UnregisterTabSpawner(PreviewSettingsTab);
    FAssetEditorToolkit::UnregisterTabSpawners(Manager);
}

TSharedRef<SDockTab> FKataAssetEditor::SpawnTab(const FSpawnTabArgs& Args)
{
    TSharedRef<SWidget> Content = SNullWidget::NullWidget;
    if (Args.GetTabId().TabType == PreviewTab) { Content = MakePreviewPanel(); }
    else if (Args.GetTabId().TabType == TimelineTab) { Content = MakeTimelinePanel(); }
    else if (Args.GetTabId().TabType == SettingsTab) { Content = MakeSettingsPanel(); }
    else if (Args.GetTabId().TabType == TaskTab) { Content = MakeTaskPanel(); }
    else if (Args.GetTabId().TabType == PreviewSettingsTab) { Content = MakePreviewSettingsPanel(); }
    return SNew(SDockTab).TabRole(ETabRole::PanelTab)[Content];
}

TSharedRef<SWidget> FKataAssetEditor::MakePreviewPanel()
{
    // 프리뷰 화면 탭에는 카메라 전환과 월드만 둔다. 설정은 Preview Details 탭에서 편집한다.
    return SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight().Padding(4)[MakeViewTypeControls()]
        + SVerticalBox::Slot().FillHeight(1)[Preview.ToSharedRef()];
}

TSharedRef<SWidget> FKataAssetEditor::MakePreviewSettingsPanel()
{
    return SNew(SVerticalBox)
        + SVerticalBox::Slot().FillHeight(1)[PreviewDetails.ToSharedRef()];
}

TSharedRef<SWidget> FKataAssetEditor::MakeViewTypeControls()
{
    TSharedRef<SHorizontalBox> Box = SNew(SHorizontalBox);
    struct FViewEntry
    {
        const TCHAR* Label;
        ELevelViewportType Type;
    };
    const FViewEntry Entries[] = {
        { TEXT("Perspective"), LVT_Perspective },
        { TEXT("Top"), LVT_OrthoTop },
        { TEXT("Right"), LVT_OrthoRight },
        { TEXT("Back"), LVT_OrthoBack } };
    for (const FViewEntry& Entry : Entries)
    {
        const ELevelViewportType Type = Entry.Type;
        Box->AddSlot().AutoWidth().Padding(0, 0, 4, 0)
        [
            SNew(SCheckBox)
            .Style(FAppStyle::Get(), "ToggleButtonCheckbox")
            .IsChecked_Lambda([this, Type]()
            {
                return Preview->IsPreviewViewportType(Type)
                    ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
            })
            .ToolTipText(FText::FromString(Type == LVT_Perspective
                ? TEXT("Perspective view") : TEXT("Orthographic view")))
            .OnCheckStateChanged_Lambda([this, Type](ECheckBoxState NewState)
            {
                if (NewState == ECheckBoxState::Checked)
                {
                    Preview->SetPreviewViewportType(Type);
                }
            })
            [
                SNew(STextBlock).Text(FText::FromString(Entry.Label))
            ]
        ];
    }
    return Box;
}

TSharedRef<SWidget> FKataAssetEditor::MakeTransportControls()
{
    auto MakeButton = [](const TCHAR* Icon, const TCHAR* Tip, FOnClicked Clicked)
    {
        return SNew(SButton)
            .ButtonStyle(FAppStyle::Get(), "SimpleButton")
            .ContentPadding(FMargin(4.0f, 2.0f))
            .ToolTipText(FText::FromString(Tip))
            .OnClicked(Clicked)
            [
                SNew(SImage).Image(FAppStyle::Get().GetBrush(Icon)).ColorAndOpacity(FSlateColor::UseForeground())
            ];
    };
    return SNew(SHorizontalBox)
        + SHorizontalBox::Slot().AutoWidth()
        [
            MakeButton(TEXT("Animation.Forward"), TEXT("Play"), FOnClicked::CreateLambda([this]()
                { Preview->Play(Asset); return FReply::Handled(); }))
        ]
        + SHorizontalBox::Slot().AutoWidth()
        [
            MakeButton(TEXT("Animation.Pause"), TEXT("Pause"), FOnClicked::CreateLambda([this]()
                { Preview->Pause(); return FReply::Handled(); }))
        ]
        + SHorizontalBox::Slot().AutoWidth()
        [
            MakeButton(TEXT("Animation.Stop"), TEXT("Stop / Reset"), FOnClicked::CreateLambda([this]()
                { Preview->ResetScene(Asset); return FReply::Handled(); }))
        ]
        + SHorizontalBox::Slot().AutoWidth().Padding(8, 4)
        [
            SNew(STextBlock).Text_Lambda([this]()
                { return FText::FromString(FString::Printf(TEXT("%s  |  %.2f s"), *Preview->GetStatus(), Preview->GetTime())); })
        ];
}

TSharedRef<SWidget> FKataAssetEditor::MakeTimelinePanel()
{
    return SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight().Padding(4)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth()[MakeTransportControls()]
            + SHorizontalBox::Slot().AutoWidth().Padding(8, 4)[SNew(STextBlock).Text(FText::FromString(TEXT("View (s)")))]
            + SHorizontalBox::Slot().AutoWidth()
            [
                SNew(SBox).WidthOverride(90)
                [
                    SNew(SSpinBox<float>).MinValue(0.1f).MaxValue(3600.0f)
                    .Value_Lambda([this]() { return ViewDuration; })
                    .OnValueChanged_Lambda([this](float Value) { ViewDuration = Value; })
                    .OnValueCommitted_Lambda([this](float Value, ETextCommit::Type)
                    {
                        ViewDuration = Value;
                        SaveEditorSettings();
                    })
                ]
            ]
            + SHorizontalBox::Slot().AutoWidth()[MakeSnapControls()]
        ]
        + SVerticalBox::Slot().FillHeight(1)[SNew(SScrollBox) + SScrollBox::Slot()[Timeline.ToSharedRef()]]
        + SVerticalBox::Slot().AutoHeight().MaxHeight(100).Padding(4)
        [
            SNew(SScrollBox) + SScrollBox::Slot()
            [SNew(STextBlock).Text_Lambda([this]() { return FText::FromString(Diagnostics); }).AutoWrapText(true)]
        ];
}

TSharedRef<SWidget> FKataAssetEditor::MakeSnapControls()
{
    return SNew(SHorizontalBox)
        + SHorizontalBox::Slot().AutoWidth().Padding(12, 4)
        [
            SNew(SCheckBox)
            .ToolTipText(FText::FromString(TEXT("Snap dragged tasks to the interval and to other task edges")))
            .IsChecked_Lambda([this]() { return bSnapEnabled ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
            .OnCheckStateChanged_Lambda([this](ECheckBoxState State) { bSnapEnabled = State == ECheckBoxState::Checked; })
            [
                SNew(STextBlock).Text(FText::FromString(TEXT("Snap")))
            ]
        ]
        + SHorizontalBox::Slot().AutoWidth().Padding(4, 4)[SNew(STextBlock).Text(FText::FromString(TEXT("Interval (s)")))]
        + SHorizontalBox::Slot().AutoWidth()
        [
            SNew(SBox).WidthOverride(90)
            [
                SNew(SSpinBox<float>).MinValue(0.001f).MaxValue(60.0f).Delta(0.01f)
                .ToolTipText(FText::FromString(TEXT("Timeline grid and snap interval")))
                .Value_Lambda([this]() { return SnapInterval; })
                .OnValueChanged_Lambda([this](float Value) { SnapInterval = FMath::Max(0.001f, Value); })
            ]
        ];
}

TSharedRef<SWidget> FKataAssetEditor::MakeSettingsPanel()
{
    return SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight()
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth()[SNew(SButton).Text(FText::FromString(TEXT("Create Child")))
                .OnClicked(this, &FKataAssetEditor::CreateChild)]
            + SHorizontalBox::Slot().AutoWidth()
            [
                SNew(SComboButton).ButtonContent()[SNew(STextBlock).Text(FText::FromString(TEXT("Reset Override")))]
                    .OnGetMenuContent_Lambda([this]() { return MakeResetMenu(false); })
            ]
            + SHorizontalBox::Slot().AutoWidth()
            [
                SNew(SComboButton).ButtonContent()[SNew(STextBlock).Text(FText::FromString(TEXT("Import Legacy (Replace)")))]
                    .OnGetMenuContent_Lambda([this]() { return MakeClassMenu(true); })
            ]
        ]
        + SVerticalBox::Slot().FillHeight(1)[SettingsDetails.ToSharedRef()];
}

TSharedRef<SWidget> FKataAssetEditor::MakeTaskPanel()
{
    return SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight()
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
            [
                SNew(STextBlock).Text_Lambda([this]()
                {
                    if (SelectedIds.IsEmpty())
                    {
                        return FText::FromString(TEXT("No task selected"));
                    }
                    if (SelectedIds.Num() == 1)
                    {
                        const UKataTask* Task = GetSelectedTask();
                        return FText::FromString(Task ? Task->GetDisplayName() : TEXT("No task selected"));
                    }
                    const TArray<UKataTask*> Tasks = GetSelectedTasks();
                    const bool bSameType = Tasks.Num() == SelectedIds.Num() && Tasks.Num() > 0
                        && Tasks.ContainsByPredicate([FirstClass = Tasks[0]->GetClass()](const UKataTask* Task)
                        {
                            return Task->GetClass() != FirstClass;
                        }) == false;
                    return FText::Format(bSameType
                        ? NSLOCTEXT("Kata", "MultiTaskSelection", "{0} tasks selected")
                        : NSLOCTEXT("Kata", "MixedTaskSelection", "{0} tasks selected (different types)"),
                        FText::AsNumber(SelectedIds.Num()));
                })
            ]
            + SHorizontalBox::Slot().AutoWidth()
            [
                SNew(SComboButton).ButtonContent()[SNew(STextBlock).Text(FText::FromString(TEXT("Reset Task Override")))]
                    .OnGetMenuContent_Lambda([this]() { return MakeResetMenu(true); })
            ]
        ]
        + SVerticalBox::Slot().FillHeight(1)[TaskDetails.ToSharedRef()];
}

TSharedRef<SWidget> FKataAssetEditor::MakeClassMenu(bool bLegacy)
{
    FClassViewerInitializationOptions Options;
    Options.Mode = EClassViewerMode::ClassPicker;
    Options.bShowNoneOption = false;
    TSharedRef<FKataClassFilter> Filter = MakeShared<FKataClassFilter>();
    Filter->BaseClass = bLegacy ? UKataDefinition::StaticClass() : UKataTask::StaticClass();
    Options.ClassFilters.Add(Filter);
    FClassViewerModule& Classes = FModuleManager::LoadModuleChecked<FClassViewerModule>("ClassViewer");
    return SNew(SBox).WidthOverride(350).HeightOverride(400)
    [
        Classes.CreateClassViewer(Options, bLegacy
            ? FOnClassPicked::CreateSP(this, &FKataAssetEditor::ImportLegacy)
            : FOnClassPicked::CreateSP(this, &FKataAssetEditor::AddTask))
    ];
}

TSharedRef<SWidget> FKataAssetEditor::MakeResetMenu(bool bTask)
{
    FMenuBuilder Menu(true, nullptr);
    if (bTask)
    {
        if (const FKataTaskOverride* Override = Asset->TaskOverrides.FindByPredicate(
            [this](const FKataTaskOverride& Item) { return Item.TargetTaskId == SelectedId; }))
        {
            Menu.AddMenuEntry(FText::FromString(TEXT("All Properties")), FText::GetEmpty(), FSlateIcon(),
                FUIAction(FExecuteAction::CreateSP(this, &FKataAssetEditor::ResetTaskProperty, FName())));
            for (FName Path : Override->OverriddenProperties)
            {
                Menu.AddMenuEntry(FText::FromName(Path), FText::GetEmpty(), FSlateIcon(),
                    FUIAction(FExecuteAction::CreateSP(this, &FKataAssetEditor::ResetTaskProperty, Path)));
            }
        }
        // 삭제한 상속 행은 타임라인에 없으므로 여기에서 복원할 수 있게 한다.
        for (const FKataTaskOverride& Override : Asset->TaskOverrides)
        {
            if (Override.Mode == EKataTimelineChangeMode::Remove)
            {
                const FKataTaskId Id = Override.TargetTaskId;
                Menu.AddMenuEntry(FText::FromString(TEXT("Restore ") + Id.ToString()), FText::GetEmpty(), FSlateIcon(),
                    FUIAction(FExecuteAction::CreateLambda([this, Id]()
                    {
                        SelectedId = Id;
                        SelectedIds.Reset();
                        SelectedIds.Add(Id);
                        ResetTaskProperty(NAME_None);
                    })));
            }
        }
    }
    else
    {
        for (FName Path : Asset->OverriddenSettings)
        {
            Menu.AddMenuEntry(FText::FromName(Path), FText::GetEmpty(), FSlateIcon(),
                FUIAction(FExecuteAction::CreateSP(this, &FKataAssetEditor::ResetSetting, Path)));
        }
    }
    return Menu.MakeWidget();
}

TSharedPtr<SWidget> FKataAssetEditor::MakeTimelineContextMenu(float Time)
{
    // 우클릭한 위치를 새 태스크와 붙여넣기의 시작 시각으로 사용한다.
    InsertTime = FMath::Max(0.0f, Time);
    const FGenericCommands& Commands = FGenericCommands::Get();
    // 메뉴 전용 목록은 단축키 표시를 유지하면서 클릭 위치에 붙여넣는다.
    const TSharedRef<FUICommandList> MenuCommands = MakeShared<FUICommandList>();
    MenuCommands->Append(TimelineCommands.ToSharedRef());
    MenuCommands->MapAction(Commands.Paste,
        FExecuteAction::CreateLambda([this, Time]()
        {
            InsertTime = FMath::Max(0.0f, Time);
            PasteTask();
        }),
        FCanExecuteAction::CreateSP(this, &FKataAssetEditor::CanPasteTask));
    FMenuBuilder Menu(true, MenuCommands);
    Menu.BeginSection(TEXT("KataTask"), NSLOCTEXT("Kata", "TaskSection", "Task"));
    Menu.AddSubMenu(NSLOCTEXT("Kata", "AddTaskLabel", "Add Task"), NSLOCTEXT("Kata", "AddTaskTip", "Add a task at this time"),
        FNewMenuDelegate::CreateLambda([this](FMenuBuilder& SubMenu)
        {
            SubMenu.AddWidget(MakeClassMenu(false), FText::GetEmpty(), true);
        }));
    Menu.AddMenuEntry(Commands.Delete, NAME_None, NSLOCTEXT("Kata", "DeleteTaskLabel", "Delete Task"));
    Menu.EndSection();
    Menu.BeginSection(TEXT("KataEdit"), NSLOCTEXT("Kata", "EditSection", "Edit"));
    Menu.AddMenuEntry(Commands.Copy);
    Menu.AddMenuEntry(Commands.Paste);
    Menu.AddSeparator();
    Menu.AddMenuEntry(Commands.Undo);
    Menu.AddMenuEntry(Commands.Redo);
    Menu.EndSection();
    return Menu.MakeWidget();
}

void FKataAssetEditor::ExtendToolbar()
{
    const TSharedRef<FExtender> Extender = MakeShared<FExtender>();
    Extender->AddToolBarExtension(TEXT("Asset"), EExtensionHook::After, GetToolkitCommands(),
        FToolBarExtensionDelegate::CreateSP(this, &FKataAssetEditor::FillToolbar));
    AddToolbarExtender(Extender);
}

void FKataAssetEditor::FillToolbar(FToolBarBuilder& Builder)
{
    Builder.BeginSection(TEXT("KataPreview"));
    Builder.AddToolBarButton(
        FUIAction(
            FExecuteAction::CreateSP(this, &FKataAssetEditor::ToggleTargetSelection),
            FCanExecuteAction(),
            FIsActionChecked::CreateSP(this, &FKataAssetEditor::IsTargetSelectionEnabled)),
        NAME_None,
        NSLOCTEXT("Kata", "SelectTarget", "Select Target"),
        NSLOCTEXT("Kata", "SelectTargetTip",
            "Move the preview target actor with the transform widget. W, E, R switch move, rotate and scale."),
        FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("EditorViewport.TranslateMode")),
        EUserInterfaceActionType::ToggleButton);
    Builder.AddToolBarButton(
        FUIAction(FExecuteAction::CreateSP(this, &FKataAssetEditor::ResizeViewToTasks)),
        NAME_None,
        NSLOCTEXT("Kata", "ResizeView", "Resize"),
        NSLOCTEXT("Kata", "ResizeViewTip", "Fit the timeline view to the task that ends last."),
        FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("Icons.Adjust")));
    Builder.EndSection();
}

void FKataAssetEditor::ToggleTargetSelection()
{
    Preview->SetTargetSelectionEnabled(!Preview->IsTargetSelectionEnabled());
}

bool FKataAssetEditor::IsTargetSelectionEnabled() const
{
    return Preview.IsValid() && Preview->IsTargetSelectionEnabled();
}

void FKataAssetEditor::ResizeViewToTasks()
{
    float End = 0.0f;
    if (EditingDefinition)
    {
        for (const UKataTask* Task : EditingDefinition->Tasks)
        {
            End = FMath::Max(End, Task->StartTime + Task->Duration);
        }
    }
    // 태스크가 없으면 기본 범위를 유지한다.
    ViewDuration = End > UE_KINDA_SMALL_NUMBER ? End : 5.0f;
    SaveEditorSettings();
}

void FKataAssetEditor::LoadEditorSettings()
{
    GConfig->GetFloat(EditorSettingsSection, TEXT("ViewDuration"), ViewDuration, GEditorPerProjectIni);
    ViewDuration = FMath::Clamp(ViewDuration, 0.1f, 3600.0f);
}

void FKataAssetEditor::SaveEditorSettings() const
{
    GConfig->SetFloat(EditorSettingsSection, TEXT("ViewDuration"), ViewDuration, GEditorPerProjectIni);
    GConfig->Flush(false, GEditorPerProjectIni);
}

void FKataAssetEditor::ApplyPreviewTargetTransform(const FTransform& Transform)
{
    if (!Asset)
    {
        return;
    }
    const FScopedTransaction Transaction(NSLOCTEXT("Kata", "MoveTarget", "Move Kata Preview Target"));
    Asset->Modify();
    Asset->PreviewTargetTransform = Transform;
    if (Settings)
    {
        Settings->PreviewTargetTransform = Transform;
    }
    Asset->MarkPackageDirty();
    // 장면을 다시 만들지 않고 값만 갱신해 위젯 조작을 이어서 할 수 있게 한다.
    PreviewDetails->ForceRefresh();
}

void FKataAssetEditor::BindCommands()
{
    const FGenericCommands& Commands = FGenericCommands::Get();
    TimelineCommands->MapAction(Commands.Delete,
        FExecuteAction::CreateSP(this, &FKataAssetEditor::DeleteSelectedTask),
        FCanExecuteAction::CreateSP(this, &FKataAssetEditor::CanDeleteTask));
    TimelineCommands->MapAction(Commands.Copy,
        FExecuteAction::CreateSP(this, &FKataAssetEditor::CopySelectedTask),
        FCanExecuteAction::CreateSP(this, &FKataAssetEditor::CanCopyTask));
    // 단축키로 붙여넣을 때는 재생 헤드 위치를 시작 시각으로 쓴다.
    TimelineCommands->MapAction(Commands.Paste,
        FExecuteAction::CreateLambda([this]()
        {
            InsertTime = Preview->GetTime();
            PasteTask();
        }),
        FCanExecuteAction::CreateSP(this, &FKataAssetEditor::CanPasteTask));
    const FExecuteAction Undo = FExecuteAction::CreateLambda([]() { GEditor->UndoTransaction(); });
    const FExecuteAction Redo = FExecuteAction::CreateLambda([]() { GEditor->RedoTransaction(); });
    TimelineCommands->MapAction(Commands.Undo, Undo);
    TimelineCommands->MapAction(Commands.Redo, Redo);
    // 실행 취소는 타임라인 밖에서도 동작하도록 툴킷 명령에도 연결한다.
    GetToolkitCommands()->MapAction(Commands.Undo, Undo);
    GetToolkitCommands()->MapAction(Commands.Redo, Redo);
}

void FKataAssetEditor::Refresh()
{
    bRefreshQueued = false;
    Settings = Asset->MakeEffectiveSettings(GetTransientPackage());
    EditingDefinition = Asset->Resolve(GetTransientPackage(), true);
    SettingsDetails->SetObject(Settings, true);
    PreviewDetails->SetObject(Settings, true);
    ExpandDetailsByDefault(SettingsDetails, TEXT("KataEditor.SettingsDetails"), Settings->GetClass());
    ExpandDetailsByDefault(PreviewDetails, TEXT("KataEditor.PreviewDetails"), Settings->GetClass());
    for (UKataTask* Task : EditingDefinition->Tasks)
    {
        Task->SetFlags(RF_Transactional);
    }
    for (auto It = SelectedIds.CreateIterator(); It; ++It)
    {
        if (!EditingDefinition->FindTask(*It))
        {
            It.RemoveCurrent();
        }
    }
    if (!SelectedIds.Contains(SelectedId))
    {
        SelectedId = SelectedIds.IsEmpty() ? FKataTaskId() : *SelectedIds.CreateConstIterator();
    }
    RefreshTaskDetails();
    RefreshRows();

    UKataResolvedDefinition* RuntimeDefinition = Asset->Resolve(GetTransientPackage());
    Diagnostics.Reset();
    for (const FKataDiagnostic& Diagnostic : RuntimeDefinition->Diagnostics)
    {
        Diagnostics += Diagnostic.ToDisplayString() + TEXT("\n");
    }
    if (Diagnostics.IsEmpty())
    {
        Diagnostics = TEXT("Drag a task to move it. Drag its right edge to resize. Click the ruler to replay to a time. "
            "Right click for Add / Delete / Copy / Paste. Delete, Ctrl+Z, Ctrl+C, Ctrl+V are supported. [P] = inherited.");
    }
}

void FKataAssetEditor::RefreshRows()
{
    TArray<FKataTimelineRow> Rows;
    for (UKataTask* Task : EditingDefinition->Tasks)
    {
        FKataTimelineRow& Row = Rows.AddDefaulted_GetRef();
        Row.Id = Task->TaskId;
        Row.Label = Task->GetDisplayName();
        Row.Start = Task->StartTime;
        Row.Duration = Task->Duration;
        Row.bEnabled = Task->bEnabled;
        Row.bInherited = !IsLocalTask(Row.Id);
    }
    Timeline->SetRows(MoveTemp(Rows), SelectedIds);
}

bool FKataAssetEditor::IsLocalTask(FKataTaskId Id) const
{
    return Asset->TimelineTasks.ContainsByPredicate([Id](const FKataTimelineEntry& Entry)
        { return Entry.Task && Entry.Task->TaskId == Id; });
}

UKataTask* FKataAssetEditor::GetSelectedTask() const
{
    return EditingDefinition ? const_cast<UKataTask*>(EditingDefinition->FindTask(SelectedId)) : nullptr;
}

TArray<UKataTask*> FKataAssetEditor::GetSelectedTasks() const
{
    TArray<UKataTask*> Result;
    if (EditingDefinition)
    {
        for (UKataTask* Task : EditingDefinition->Tasks)
        {
            if (Task && SelectedIds.Contains(Task->TaskId))
            {
                Result.Add(Task);
            }
        }
    }
    return Result;
}

void FKataAssetEditor::RefreshTaskDetails()
{
    const TArray<UKataTask*> Tasks = GetSelectedTasks();
    TArray<UObject*> Objects;
    UClass* CommonClass = nullptr;
    for (UKataTask* Task : Tasks)
    {
        if (!CommonClass)
        {
            CommonClass = Task->GetClass();
        }
        else if (Task->GetClass() != CommonClass)
        {
            Objects.Reset();
            break;
        }
        Objects.Add(Task);
    }
    TaskDetails->SetObjects(Objects, true);
    if (CommonClass && !Objects.IsEmpty())
    {
        ExpandDetailsByDefault(TaskDetails, TEXT("KataEditor.TaskDetails"), CommonClass);
    }
}

void FKataAssetEditor::SelectTask(FKataTaskId Id, bool bToggle)
{
    if (bToggle)
    {
        if (SelectedIds.Contains(Id))
        {
            SelectedIds.Remove(Id);
        }
        else
        {
            SelectedIds.Add(Id);
            SelectedId = Id;
        }
    }
    else
    {
        SelectedIds.Reset();
        SelectedIds.Add(Id);
        SelectedId = Id;
    }
    if (!SelectedIds.Contains(SelectedId))
    {
        SelectedId = SelectedIds.IsEmpty() ? FKataTaskId() : *SelectedIds.CreateConstIterator();
    }
    RefreshTaskDetails();
    RefreshRows();
}

void FKataAssetEditor::AddTask(UClass* Class)
{
    FSlateApplication::Get().DismissAllMenus();
    if (!Class || !Class->IsChildOf(UKataTask::StaticClass()) || Class->HasAnyClassFlags(CLASS_Abstract))
    {
        return;
    }
    const FScopedTransaction Transaction(NSLOCTEXT("Kata", "AddTask", "Add Kata Task"));
    Asset->Modify();
    UKataTask* Task = NewObject<UKataTask>(Asset, Class, NAME_None, RF_Transactional);
    Task->TaskId = FKataTaskId::NewId();
    Task->Duration = 1.0f;
    Task->StartTime = InsertTime;
    Asset->TimelineTasks.AddDefaulted_GetRef().Task = Task;
    SelectedId = Task->TaskId;
    SelectedIds.Reset();
    SelectedIds.Add(SelectedId);
    Changed();
}

void FKataAssetEditor::ImportLegacy(UClass* Class)
{
    FSlateApplication::Get().DismissAllMenus();
    if (!Class || !Class->IsChildOf(UKataDefinition::StaticClass()))
    {
        return;
    }
    // 기존 Blueprint는 수정하지 않는다. 새 에셋에 병합된 설정과 태스크를 복사한다.
    UKataResolvedDefinition* Legacy = UKataDefinition::ResolveDefinition(Class, GetTransientPackage(), true);
    if (!Legacy || Legacy->HasErrors())
    {
        Diagnostics = TEXT("Legacy definition could not be imported");
        return;
    }
    const FScopedTransaction Transaction(NSLOCTEXT("Kata", "ImportLegacy", "Import Legacy Kata"));
    Asset->Modify();
    Asset->ParentKata = nullptr;
    Asset->OverriddenSettings.Reset();
    Asset->TaskOverrides.Reset();
    Asset->TimelineTasks.Reset();
    const TArray<FName> Names = { TEXT("KataTags"), TEXT("ActivationRequiredTags"), TEXT("ActivationBlockedTags"),
        TEXT("ActiveGrantedTags"), TEXT("StartCondition"), TEXT("BlockingPolicy"), TEXT("CooldownPolicy"), TEXT("LoopPolicy") };
    for (FName Name : Names)
    {
        FString Error;
        KataPropertyOverride::CopyOverriddenProperty(Asset, Legacy, Name, Error);
    }
    for (UKataTask* Source : Legacy->Tasks)
    {
        UKataTask* Task = DuplicateObject<UKataTask>(Source, Asset, MakeUniqueObjectName(Asset, Source->GetClass()));
        Task->SetFlags(RF_Transactional);
        Asset->TimelineTasks.AddDefaulted_GetRef().Task = Task;
    }
    Changed();
}

void FKataAssetEditor::ApplyTaskProperty(UKataTask* Edited, FName Path)
{
    FString Error;
    for (FKataTimelineEntry& Entry : Asset->TimelineTasks)
    {
        if (Entry.Task && Entry.Task->TaskId == Edited->TaskId)
        {
            Entry.Task->SetFlags(RF_Transactional);
            Entry.Task->Modify();
            if (!KataPropertyOverride::CopyOverriddenProperty(Entry.Task, Edited, Path, Error))
            {
                Diagnostics = Error;
            }
            return;
        }
    }
    FKataTaskOverride* Override = Asset->TaskOverrides.FindByPredicate(
        [Edited](const FKataTaskOverride& Item) { return Item.TargetTaskId == Edited->TaskId; });
    if (!Override)
    {
        Override = &Asset->TaskOverrides.AddDefaulted_GetRef();
        Override->TargetTaskId = Edited->TaskId;
    }
    Override->Mode = EKataTimelineChangeMode::Modify;
    if (!Override->OverrideValues || Override->OverrideValues->GetClass() != Edited->GetClass())
    {
        Override->OverrideValues = DuplicateObject<UKataTask>(Edited, Asset, MakeUniqueObjectName(Asset, Edited->GetClass()));
        Override->OverrideValues->SetFlags(RF_Transactional);
    }
    Override->OverrideValues->Modify();
    if (KataPropertyOverride::CopyOverriddenProperty(Override->OverrideValues, Edited, Path, Error))
    {
        Override->OverriddenProperties.AddUnique(Path);
    }
    else
    {
        Diagnostics = Error;
    }
}

void FKataAssetEditor::MoveTask(FKataTaskId Id, float Start, float Duration)
{
    UKataTask* Task = EditingDefinition ? const_cast<UKataTask*>(EditingDefinition->FindTask(Id)) : nullptr;
    if (!Task || (Task->StartTime == Start && Task->Duration == Duration))
    {
        return;
    }
    const FScopedTransaction Transaction(NSLOCTEXT("Kata", "MoveTask", "Move Kata Task"));
    Asset->Modify();
    if (Task->StartTime != Start)
    {
        Task->StartTime = Start;
        ApplyTaskProperty(Task, GET_MEMBER_NAME_CHECKED(UKataTask, StartTime));
    }
    if (Task->Duration != Duration)
    {
        Task->Duration = Duration;
        ApplyTaskProperty(Task, GET_MEMBER_NAME_CHECKED(UKataTask, Duration));
    }
    Changed();
}

void FKataAssetEditor::OnTaskEdited(const FPropertyChangedEvent& Event)
{
    const TArray<UKataTask*> Tasks = GetSelectedTasks();
    if (Tasks.IsEmpty() || !Event.MemberProperty)
    {
        return;
    }
    const FScopedTransaction Transaction(NSLOCTEXT("Kata", "EditTask", "Edit Kata Task"));
    Asset->Modify();
    const FName Path = KataPropertyOverride::GetPropertyPath(Event.MemberProperty, Event.Property);
    for (UKataTask* Task : Tasks)
    {
        ApplyTaskProperty(Task, Path);
    }
    Changed();
}

void FKataAssetEditor::OnSettingsEdited(const FPropertyChangedEvent& Event)
{
    if (!Event.MemberProperty)
    {
        return;
    }
    const FName Path = KataPropertyOverride::GetPropertyPath(Event.MemberProperty, Event.Property);
    const FName RootName = Event.MemberProperty->GetFName();
    const FScopedTransaction Transaction(NSLOCTEXT("Kata", "EditSettings", "Edit Kata Settings"));
    Asset->Modify();
    if (RootName == GET_MEMBER_NAME_CHECKED(UKataAsset, ParentKata))
    {
        // 부모를 바꾸기 전에 변경 후보의 상속 체인에 자신이 포함되는지 확인한다.
        TArray<const UKataAsset*> Chain;
        const bool bValid = !Settings->ParentKata
            || (Settings->ParentKata->CollectAssetChain(Chain) && !Chain.Contains(Asset));
        if (!bValid)
        {
            Diagnostics = TEXT("Parent Kata would create an inheritance cycle");
            Settings->ParentKata = Asset->ParentKata;
            return;
        }
    }
    FString Error;
    if (KataPropertyOverride::CopyOverriddenProperty(Asset, Settings, Path, Error))
    {
        if (Asset->ParentKata && RootName != GET_MEMBER_NAME_CHECKED(UKataAsset, ParentKata)
            && RootName != GET_MEMBER_NAME_CHECKED(UKataAsset, OverriddenSettings)
            && !RootName.ToString().StartsWith(TEXT("Preview"))
            && !RootName.ToString().StartsWith(TEXT("bPreview")))
        {
            Asset->OverriddenSettings.AddUnique(Path);
        }
        Changed();
    }
    else
    {
        Diagnostics = Error;
    }
}

void FKataAssetEditor::ResetSetting(FName Path)
{
    const FScopedTransaction Transaction(NSLOCTEXT("Kata", "ResetSetting", "Reset Kata Setting"));
    Asset->Modify();
    Asset->OverriddenSettings.Remove(Path);
    Changed();
}

void FKataAssetEditor::ResetTaskProperty(FName Path)
{
    const FScopedTransaction Transaction(NSLOCTEXT("Kata", "ResetTaskProperty", "Reset Kata Task Override"));
    Asset->Modify();
    for (int32 Index = Asset->TaskOverrides.Num() - 1; Index >= 0; --Index)
    {
        FKataTaskOverride& Override = Asset->TaskOverrides[Index];
        if (!SelectedIds.Contains(Override.TargetTaskId))
        {
            continue;
        }
        Override.OverriddenProperties.Remove(Path);
        if (Path.IsNone() || Override.OverriddenProperties.IsEmpty())
        {
            Asset->TaskOverrides.RemoveAt(Index);
        }
    }
    Changed();
}

bool FKataAssetEditor::CanDeleteTask() const
{
    return !GetSelectedTasks().IsEmpty();
}

bool FKataAssetEditor::CanCopyTask() const
{
    return SelectedIds.Num() == 1 && GetSelectedTask() != nullptr;
}

bool FKataAssetEditor::CanPasteTask() const
{
    return TaskClipboard.IsValid();
}

void FKataAssetEditor::CopySelectedTask()
{
    UKataTask* Task = GetSelectedTask();
    if (!Task)
    {
        return;
    }
    // 복사본은 에셋 밖에 두어 원본 편집이 클립보드에 반영되지 않게 한다.
    UObject* Outer = GetTransientPackage();
    TaskClipboard.Reset(DuplicateObject<UKataTask>(Task, Outer, MakeUniqueObjectName(Outer, Task->GetClass())));
}

void FKataAssetEditor::PasteTask()
{
    UKataTask* Source = TaskClipboard.Get();
    if (!Source)
    {
        return;
    }
    const FScopedTransaction Transaction(NSLOCTEXT("Kata", "PasteTask", "Paste Kata Task"));
    Asset->Modify();
    UKataTask* Task = DuplicateObject<UKataTask>(Source, Asset, MakeUniqueObjectName(Asset, Source->GetClass()));
    Task->SetFlags(RF_Transactional);
    // 붙여넣은 항목은 부모·자식 오버라이드와 겹치지 않도록 새 ID를 받는다.
    Task->TaskId = FKataTaskId::NewId();
    Task->StartTime = FMath::Max(0.0f, InsertTime);
    Asset->TimelineTasks.AddDefaulted_GetRef().Task = Task;
    SelectedId = Task->TaskId;
    SelectedIds.Reset();
    SelectedIds.Add(SelectedId);
    Changed();
}

void FKataAssetEditor::DeleteSelectedTask()
{
    const TArray<UKataTask*> Tasks = GetSelectedTasks();
    if (Tasks.IsEmpty())
    {
        return;
    }
    const FScopedTransaction Transaction(NSLOCTEXT("Kata", "DeleteTask", "Delete Kata Task"));
    Asset->Modify();
    for (UKataTask* Task : Tasks)
    {
        const FKataTaskId Id = Task->TaskId;
        if (IsLocalTask(Id))
        {
            Asset->TimelineTasks.RemoveAll([Id](const FKataTimelineEntry& Entry)
                { return Entry.Task && Entry.Task->TaskId == Id; });
        }
        else
        {
            Asset->TaskOverrides.RemoveAll([Id](const FKataTaskOverride& Entry) { return Entry.TargetTaskId == Id; });
            FKataTaskOverride& Override = Asset->TaskOverrides.AddDefaulted_GetRef();
            Override.TargetTaskId = Id;
            Override.Mode = EKataTimelineChangeMode::Remove;
        }
    }
    SelectedId.Invalidate();
    SelectedIds.Reset();
    Changed();
}

FReply FKataAssetEditor::CreateChild()
{
    UKataAssetFactory* Factory = NewObject<UKataAssetFactory>();
    Factory->ParentAsset = Asset;
    FAssetToolsModule& Tools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
    UObject* Child = Tools.Get().CreateAssetWithDialog(Asset->GetName() + TEXT("_Child"),
        FPackageName::GetLongPackagePath(Asset->GetOutermost()->GetName()), UKataAsset::StaticClass(), Factory);
    if (Child)
    {
        GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Child);
    }
    return FReply::Handled();
}

void FKataAssetEditor::Changed()
{
    Asset->MarkPackageDirty();
    Preview->Stop();
    bRefreshQueued = true;
    // 부모 에셋을 열어 둔 다른 Kata 에디터에도 갱신을 알린다.
    FPropertyChangedEvent Event(nullptr);
    FCoreUObjectDelegates::OnObjectPropertyChanged.Broadcast(Asset, Event);
}

void FKataAssetEditor::OnObjectChanged(UObject* Object, FPropertyChangedEvent& Event)
{
    TArray<const UKataAsset*> Chain;
    if (!Asset->CollectAssetChain(Chain))
    {
        return;
    }
    for (const UKataAsset* Entry : Chain)
    {
        if (Object == Entry || (Object && Object->IsIn(Entry)))
        {
            bRefreshQueued = true;
            return;
        }
    }
}

void FKataAssetEditor::Tick(float DeltaTime)
{
    if (bRefreshQueued)
    {
        // Details 콜백 안에서 패널을 재구성하지 않는다.
        Preview->ResetScene(Asset);
        Refresh();
    }
    Preview->TickSimulation(DeltaTime);
}

void FKataAssetEditor::PostUndo(bool bSuccess)
{
    if (bSuccess)
    {
        bRefreshQueued = true;
    }
}

void FKataAssetEditor::AddReferencedObjects(FReferenceCollector& Collector)
{
    Collector.AddReferencedObject(Asset);
    Collector.AddReferencedObject(Settings);
    Collector.AddReferencedObject(EditingDefinition);
}
