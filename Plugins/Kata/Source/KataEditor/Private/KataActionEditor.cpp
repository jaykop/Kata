#include "KataActionEditor.h"

#include "AssetToolsModule.h"
#include "ClassViewerFilter.h"
#include "ClassViewerModule.h"
#include "Action/KataAction.h"
#include "Action/KataPropertyOverride.h"
#include "Action/KataResolvedAction.h"
#include "Action/KataTask.h"
#include "Editor.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Commands/GenericCommands.h"
#include "Framework/Commands/UICommandList.h"
#include "Framework/Docking/TabManager.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Framework/MultiBox/MultiBoxExtender.h"
#include "IDetailsView.h"
#include "KataActionFactory.h"
#include "KataTimelineGroupDetails.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/PackageName.h"
#include "PropertyEditorDelegates.h"
#include "PropertyEditorModule.h"
#include "ScopedTransaction.h"
#include "SKataPreviewViewport.h"
#include "SKataTimeline.h"
#include "Styling/AppStyle.h"
#include "Styling/StyleColors.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Widgets/Layout/SBorder.h"
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
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SWindow.h"

namespace
{
    /** GUID가 같으면 항상 같은 색을 반환해 자동 색상이 세션마다 바뀌지 않게 한다. */
    FLinearColor MakeStableTimelineColor(const FGuid& Id)
    {
        const uint32 Hash = GetTypeHash(Id);
        return FLinearColor::MakeFromHSV8(static_cast<uint8>(Hash & 0xff), 170, 220);
    }

    const FName PreviewTab(TEXT("Kata.Preview"));
    const FName TimelineTab(TEXT("Kata.Timeline"));
    const FName SettingsTab(TEXT("Kata.Settings"));
    const FName TaskTab(TEXT("Kata.Task"));
    const FName PreviewSettingsTab(TEXT("Kata.PreviewSettings"));
    const TCHAR* EditorSettingsSection = TEXT("KataAssetEditor");

    /** 주석 표시 방식의 이름. 툴바 버튼과 팝업 메뉴가 함께 쓴다. */
    FText DescribeCommentDisplay(EKataTimelineCommentDisplay Display)
    {
        switch (Display)
        {
        case EKataTimelineCommentDisplay::Hidden:
            return FText::FromString(TEXT("Hidden"));
        case EKataTimelineCommentDisplay::Inline:
            return FText::FromString(TEXT("Inline"));
        case EKataTimelineCommentDisplay::Tooltip:
        default:
            return FText::FromString(TEXT("Tooltip"));
        }
    }

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
            return Class->IsChildOf(BaseClass) && !Class->IsChildOf(UKataAction::StaticClass())
                && !Class->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists);
        }
        virtual bool IsUnloadedClassAllowed(const FClassViewerInitializationOptions&,
            const TSharedRef<const IUnloadedBlueprintData> Data, TSharedRef<FClassViewerFilterFuncs>) override
        {
            return Data->IsChildOf(BaseClass) && !Data->IsChildOf(UKataAction::StaticClass())
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
            && Name != GET_MEMBER_NAME_CHECKED(UKataAction, TimelineTasks)
            && Name != GET_MEMBER_NAME_CHECKED(UKataAction, TaskOverrides)
            && Name != GET_MEMBER_NAME_CHECKED(UKataAction, TimelineGroups);
    }
}

FKataActionEditor::~FKataActionEditor()
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

void FKataActionEditor::Init(UKataAction* InAsset)
{
    Asset = InAsset;
    Asset->SetFlags(RF_Transactional);
    GroupDetails = NewObject<UKataTimelineGroupDetails>(GetTransientPackage());
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
    SettingsDetails->OnFinishedChangingProperties().AddSP(this, &FKataActionEditor::OnSettingsEdited);
    PreviewDetails->OnFinishedChangingProperties().AddSP(this, &FKataActionEditor::OnSettingsEdited);
    TaskDetails->OnFinishedChangingProperties().AddSP(this, &FKataActionEditor::OnTaskEdited);
    TaskDetails->OnFinishedChangingProperties().AddSP(this, &FKataActionEditor::OnGroupDetailsEdited);
    SAssignNew(Preview, SKataPreviewViewport)
        .OnActorMoved(FKataPreviewTransformChanged::CreateSP(this, &FKataActionEditor::ApplyPreviewActorTransform));
    TimelineCommands = MakeShared<FUICommandList>();
    BindCommands();
    SAssignNew(Timeline, SKataTimeline)
        .OnSelect(FKataSelectTask::CreateSP(this, &FKataActionEditor::SelectTask))
        .OnMove(FKataMoveTask::CreateSP(this, &FKataActionEditor::MoveTask))
        .OnSeek(FKataSeekPreview::CreateLambda([this](float Time) { Preview->Seek(Asset, EditingAction, Time); }))
        .OnToggleGroup(FKataToggleTimelineGroup::CreateSP(this, &FKataActionEditor::ToggleTimelineGroup))
        .OnSelectGroup(FKataSelectTimelineGroup::CreateSP(this, &FKataActionEditor::SelectTimelineGroup))
        .OnContextMenu(FKataTimelineMenu::CreateSP(this, &FKataActionEditor::MakeTimelineContextMenu))
        .CommandList(TimelineCommands)
        .Playhead_Lambda([this]() { return Preview->GetTime(); })
        .ViewDuration_Lambda([this]() { return TimelineLength; })
        .SnapInterval_Lambda([this]() { return SnapInterval; })
        .SnapEnabled_Lambda([this]() { return bSnapEnabled; })
        .CommentDisplay_Lambda([this]() { return CommentDisplay; });
    ExtendToolbar();
    Refresh();
    Preview->ResetScene(Asset);
    GEditor->RegisterForUndo(this);
    PropertyChangedHandle = FCoreUObjectDelegates::OnObjectPropertyChanged.AddRaw(this, &FKataActionEditor::OnObjectChanged);

    // Preview 월드는 자체 탭에 두고, 프리뷰 설정은 Kata Action Details 옆의 별도 탭으로 분리한다.
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

void FKataActionEditor::RegisterTabSpawners(const TSharedRef<FTabManager>& Manager)
{
    FAssetEditorToolkit::RegisterTabSpawners(Manager);
    // Window 메뉴에 등록해 두면 닫은 탭을 레이아웃 초기화 없이 다시 열 수 있다.
    const TSharedRef<FWorkspaceItem> Category = AssetEditorTabsCategory.IsValid()
        ? AssetEditorTabsCategory.ToSharedRef() : Manager->GetLocalWorkspaceMenuRoot();
    Manager->RegisterTabSpawner(PreviewTab, FOnSpawnTab::CreateSP(this, &FKataActionEditor::SpawnTab))
        .SetDisplayName(NSLOCTEXT("Kata", "PreviewTab", "Preview")).SetGroup(Category);
    Manager->RegisterTabSpawner(TimelineTab, FOnSpawnTab::CreateSP(this, &FKataActionEditor::SpawnTab))
        .SetDisplayName(NSLOCTEXT("Kata", "TimelineTab", "Timeline")).SetGroup(Category);
    Manager->RegisterTabSpawner(SettingsTab, FOnSpawnTab::CreateSP(this, &FKataActionEditor::SpawnTab))
        .SetDisplayName(NSLOCTEXT("Kata", "SettingsTab", "Kata Action Details")).SetGroup(Category);
    Manager->RegisterTabSpawner(TaskTab, FOnSpawnTab::CreateSP(this, &FKataActionEditor::SpawnTab))
        .SetDisplayName(NSLOCTEXT("Kata", "TaskTab", "Timeline Details")).SetGroup(Category);
    Manager->RegisterTabSpawner(PreviewSettingsTab, FOnSpawnTab::CreateSP(this, &FKataActionEditor::SpawnTab))
        .SetDisplayName(NSLOCTEXT("Kata", "PreviewSettingsTab", "Preview Details")).SetGroup(Category);
}

void FKataActionEditor::UnregisterTabSpawners(const TSharedRef<FTabManager>& Manager)
{
    Manager->UnregisterTabSpawner(PreviewTab);
    Manager->UnregisterTabSpawner(TimelineTab);
    Manager->UnregisterTabSpawner(SettingsTab);
    Manager->UnregisterTabSpawner(TaskTab);
    Manager->UnregisterTabSpawner(PreviewSettingsTab);
    FAssetEditorToolkit::UnregisterTabSpawners(Manager);
}

TSharedRef<SDockTab> FKataActionEditor::SpawnTab(const FSpawnTabArgs& Args)
{
    TSharedRef<SWidget> Content = SNullWidget::NullWidget;
    if (Args.GetTabId().TabType == PreviewTab) { Content = MakePreviewPanel(); }
    else if (Args.GetTabId().TabType == TimelineTab) { Content = MakeTimelinePanel(); }
    else if (Args.GetTabId().TabType == SettingsTab) { Content = MakeSettingsPanel(); }
    else if (Args.GetTabId().TabType == TaskTab) { Content = MakeTaskPanel(); }
    else if (Args.GetTabId().TabType == PreviewSettingsTab) { Content = MakePreviewSettingsPanel(); }
    return SNew(SDockTab).TabRole(ETabRole::PanelTab)[Content];
}

TSharedRef<SWidget> FKataActionEditor::MakePreviewPanel()
{
    // 카메라·뷰 모드 조작은 뷰포트 자체의 툴바가 맡는다. 설정은 Preview Details 탭에서 편집한다.
    return Preview.ToSharedRef();
}

TSharedRef<SWidget> FKataActionEditor::MakePreviewSettingsPanel()
{
    return SNew(SVerticalBox)
        + SVerticalBox::Slot().FillHeight(1)[PreviewDetails.ToSharedRef()];
}

TSharedRef<SWidget> FKataActionEditor::MakeTransportControls()
{
    // 타임라인 툴바 행은 숫자 입력칸 높이에 맞춰 늘어나므로, 아이콘을 행 높이만큼 늘이지 않도록
    // 슬롯을 세로 가운데 정렬하고 아이콘 크기를 레벨 에디터 재생 아이콘과 같은 20x20으로 고정한다.
    const FVector2D IconSize(20.0f, 20.0f);
    // 레벨 에디터 재생 툴바처럼 버튼 묶음 뒤에 둥근 배경판을 깐다. 브러시는 위젯이 참조하므로 정적 수명으로 둔다.
    static const FSlateRoundedBoxBrush BackplateBrush(FStyleColors::Dropdown, 4.0f);
    const TSharedRef<SHorizontalBox> Buttons = SNew(SHorizontalBox)
        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
        [
            // 재생과 일시 정지는 한 버튼이 상태에 따라 번갈아 맡는다.
            SNew(SButton)
            .ButtonStyle(FAppStyle::Get(), "SimpleButton")
            .ContentPadding(FMargin(4.0f, 2.0f))
            .ToolTipText_Lambda([this]()
            {
                return FText::FromString(Preview->IsPlaying() ? TEXT("Pause") : TEXT("Play"));
            })
            .OnClicked_Lambda([this]()
            {
                if (Preview->IsPlaying())
                {
                    Preview->Pause();
                }
                else
                {
                    Preview->Play(Asset);
                }
                return FReply::Handled();
            })
            [
                SNew(SImage)
                .DesiredSizeOverride(IconSize)
                .Image_Lambda([this]()
                {
                    return FAppStyle::Get().GetBrush(Preview->IsPlaying()
                        ? TEXT("Animation.Pause") : TEXT("Animation.Forward"));
                })
                // 레벨 에디터 툴바처럼 재생은 초록색, 일시 정지는 기본 전경색으로 그린다.
                .ColorAndOpacity_Lambda([this]()
                {
                    return Preview->IsPlaying() ? FSlateColor::UseForeground() : FStyleColors::AccentGreen;
                })
            ]
        ]
        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
        [
            SNew(SButton)
            .ButtonStyle(FAppStyle::Get(), "SimpleButton")
            .ContentPadding(FMargin(4.0f, 2.0f))
            .ToolTipText(FText::FromString(TEXT("Stop / Reset")))
            .OnClicked_Lambda([this]() { Preview->ResetScene(Asset); return FReply::Handled(); })
            [
                // 레벨 에디터의 Stop 버튼과 같은 빨간색을 쓴다.
                SNew(SImage)
                .DesiredSizeOverride(IconSize)
                .Image(FAppStyle::Get().GetBrush(TEXT("Animation.Stop")))
                .ColorAndOpacity(FStyleColors::AccentRed)
            ]
        ]
        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
        [
            SNew(SCheckBox)
            .Style(FAppStyle::Get(), "ToggleButtonCheckbox")
            .Padding(FMargin(4.0f, 2.0f))
            .ToolTipText(FText::FromString(
                TEXT("Repeat the preview: restart the action when it finishes. ")
                TEXT("This is a preview-only setting and does not change the asset's Loop Policy.")))
            .IsChecked_Lambda([this]()
            {
                return bPreviewRepeat ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
            })
            .OnCheckStateChanged_Lambda([this](ECheckBoxState State)
            {
                bPreviewRepeat = State == ECheckBoxState::Checked;
                SaveEditorSettings();
            })
            [
                SNew(SImage)
                .DesiredSizeOverride(IconSize)
                .Image_Lambda([this]()
                {
                    return FAppStyle::Get().GetBrush(bPreviewRepeat
                        ? TEXT("Animation.Loop.Enabled") : TEXT("Animation.Loop.Disabled"));
                })
                .ColorAndOpacity(FSlateColor::UseForeground())
            ]
        ];
    return SNew(SHorizontalBox)
        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
        [
            SNew(SBorder)
            .BorderImage(&BackplateBrush)
            .Padding(FMargin(2.0f))
            [
                Buttons
            ]
        ]
        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8, 4)
        [
            // Ready·Playing·Paused 같은 평상시 상태는 버튼 모양으로 알 수 있으므로 시작 실패만 표시한다.
            SNew(STextBlock)
            .Text_Lambda([this]() { return FText::FromString(Preview->GetStatus()); })
            .ColorAndOpacity(FStyleColors::Error)
            .Visibility_Lambda([this]()
            {
                return Preview->HasStatusError() ? EVisibility::Visible : EVisibility::Collapsed;
            })
        ];
}

TSharedRef<SWidget> FKataActionEditor::MakeTimelinePanel()
{
    // SKataTimeline은 행 수로만 희망 높이를 정하므로 스크롤 영역 안에서는 세 행 높이에 머문다.
    // 스크롤 영역의 현재 높이를 최소 높이로 요구해 행이 적어도 탭을 가득 채우게 한다.
    // 행이 늘어 희망 높이가 이 값을 넘으면 평소대로 스크롤된다.
    SAssignNew(TimelineScrollBox, SScrollBox)
        + SScrollBox::Slot()
        [
            SNew(SBox)
            .MinDesiredHeight_Lambda([this]() -> FOptionalSize
            {
                return TimelineScrollBox.IsValid()
                    ? FOptionalSize(static_cast<float>(TimelineScrollBox->GetTickSpaceGeometry().GetLocalSize().Y))
                    : FOptionalSize();
            })
            [
                Timeline.ToSharedRef()
            ]
        ];

    return SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight().Padding(4)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth()[MakeTransportControls()]
            + SHorizontalBox::Slot().AutoWidth().Padding(8, 4)[SNew(STextBlock).Text(FText::FromString(TEXT("Length")))]
            + SHorizontalBox::Slot().AutoWidth()
            [
                SNew(SBox).WidthOverride(90)
                [
                    SNew(SSpinBox<float>).MinValue(0.1f).MaxValue(3600.0f)
                    .Value_Lambda([this]() { return TimelineLength; })
                    .OnValueChanged_Lambda([this](float Value) { TimelineLength = Value; })
                    .OnValueCommitted_Lambda([this](float Value, ETextCommit::Type)
                    {
                        TimelineLength = Value;
                        SaveEditorSettings();
                    })
                ]
            ]
            + SHorizontalBox::Slot().AutoWidth().Padding(12, 4)
            [
                SNew(STextBlock).Text(FText::FromString(TEXT("Current Time")))
            ]
            + SHorizontalBox::Slot().AutoWidth()
            [
                SNew(SBox).WidthOverride(90)
                [
                    SNew(SSpinBox<float>).MinValue(0.0f).MaxValue(3600.0f).Delta(0.01f)
                    .MaxFractionalDigits(6)
                    .ClearKeyboardFocusOnCommit(true)
                    .Value_Lambda([this]() { return Preview->GetTime(); })
                    .OnValueChanged_Lambda([this](float Value)
                    {
                        SeekFromTimeInput(Value);
                    })
                    .OnValueCommitted_Lambda([this](float Value, ETextCommit::Type)
                    {
                        SeekFromTimeInput(Value);
                    })
                ]
            ]
            + SHorizontalBox::Slot().AutoWidth()[MakeSnapControls()]
            + SHorizontalBox::Slot().AutoWidth().Padding(8, 4)
            [
                SNew(SComboButton)
                .ToolTipText(FText::FromString(TEXT("Choose how task and group editor comments are shown")))
                .OnGetMenuContent_Lambda([this]() { return MakeCommentDisplayMenu(); })
                .ButtonContent()
                [
                    SNew(STextBlock).Text_Lambda([this]()
                    {
                        return FText::Format(NSLOCTEXT("Kata", "CommentDisplayButton", "Comments: {0}"),
                            DescribeCommentDisplay(CommentDisplay));
                    })
                ]
            ]
        ]
        + SVerticalBox::Slot().FillHeight(1)[TimelineScrollBox.ToSharedRef()]
        + SVerticalBox::Slot().AutoHeight().MaxHeight(100).Padding(4)
        [
            SNew(SScrollBox) + SScrollBox::Slot()
            [SNew(STextBlock).Text_Lambda([this]() { return FText::FromString(Diagnostics); }).AutoWrapText(true)]
        ];
}

TSharedRef<SWidget> FKataActionEditor::MakeSnapControls()
{
    return SNew(SHorizontalBox)
        + SHorizontalBox::Slot().AutoWidth().Padding(12, 4)[SNew(STextBlock).Text(FText::FromString(TEXT("Interval (s)")))]
        + SHorizontalBox::Slot().AutoWidth()
        [
            SNew(SBox).WidthOverride(90)
            [
                SNew(SSpinBox<float>).MinValue(0.001f).MaxValue(60.0f).Delta(0.01f)
                .ToolTipText(FText::FromString(TEXT("Timeline grid and snap interval")))
                .Value_Lambda([this]() { return SnapInterval; })
                .OnValueChanged_Lambda([this](float Value) { SnapInterval = FMath::Max(0.001f, Value); })
            ]
        ]
        + SHorizontalBox::Slot().AutoWidth().Padding(8, 4)
        [
            SNew(SCheckBox)
            .ToolTipText(FText::FromString(TEXT("Snap dragged tasks to the interval and to other task edges")))
            .IsChecked_Lambda([this]() { return bSnapEnabled ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
            .OnCheckStateChanged_Lambda([this](ECheckBoxState State) { bSnapEnabled = State == ECheckBoxState::Checked; })
            [
                SNew(STextBlock).Text(FText::FromString(TEXT("Snap")))
            ]
        ];
}

TSharedRef<SWidget> FKataActionEditor::MakeCommentDisplayMenu()
{
    FMenuBuilder Builder(true, nullptr);
    auto AddMode = [this, &Builder](EKataTimelineCommentDisplay Mode, const TCHAR* Description)
    {
        Builder.AddMenuEntry(DescribeCommentDisplay(Mode), FText::FromString(Description), FSlateIcon(),
            FUIAction(
                FExecuteAction::CreateLambda([this, Mode]()
                {
                    CommentDisplay = Mode;
                    SaveEditorSettings();
                    Timeline->Invalidate(EInvalidateWidgetReason::Paint);
                }),
                FCanExecuteAction(),
                FIsActionChecked::CreateLambda([this, Mode]() { return CommentDisplay == Mode; })),
            NAME_None, EUserInterfaceActionType::RadioButton);
    };
    AddMode(EKataTimelineCommentDisplay::Hidden, TEXT("Do not show editor comments"));
    AddMode(EKataTimelineCommentDisplay::Tooltip, TEXT("Show the comment when hovering a task or group"));
    AddMode(EKataTimelineCommentDisplay::Inline, TEXT("Draw the comment in the clip and keep the hover tooltip"));
    return Builder.MakeWidget();
}

TSharedRef<SWidget> FKataActionEditor::MakeSettingsPanel()
{
    return SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight()
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth()[SNew(SButton).Text(FText::FromString(TEXT("Create Child")))
                .OnClicked(this, &FKataActionEditor::CreateChild)]
            + SHorizontalBox::Slot().AutoWidth()
            [
                SNew(SComboButton).ButtonContent()[SNew(STextBlock).Text(FText::FromString(TEXT("Reset Override")))]
                    .OnGetMenuContent_Lambda([this]() { return MakeResetMenu(false); })
            ]
        ]
        + SVerticalBox::Slot().FillHeight(1)[SettingsDetails.ToSharedRef()];
}

TSharedRef<SWidget> FKataActionEditor::MakeTaskPanel()
{
    return SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight()
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
            [
                SNew(STextBlock).Text_Lambda([this]()
                {
                    if (SelectedGroupId.IsValid())
                    {
#if WITH_EDITORONLY_DATA
                        if (Asset)
                        {
                            if (const FKataTimelineGroup* Group = Asset->TimelineGroups.FindByPredicate(
                                [this](const FKataTimelineGroup& Entry)
                                {
                                    return Entry.GroupId == SelectedGroupId;
                                }))
                            {
                                return FText::Format(NSLOCTEXT("Kata", "SelectedGroupDetails", "Group: {0}"),
                                    Group->Title.IsEmpty() ? NSLOCTEXT("Kata", "UnnamedGroup", "Unnamed") : Group->Title);
                            }
                        }
#endif
                        return NSLOCTEXT("Kata", "NoGroupSelected", "No group selected");
                    }
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
                    .Visibility_Lambda([this]()
                    {
                        return SelectedGroupId.IsValid() ? EVisibility::Collapsed : EVisibility::Visible;
                    })
                    .OnGetMenuContent_Lambda([this]() { return MakeResetMenu(true); })
            ]
        ]
        + SVerticalBox::Slot().FillHeight(1)[TaskDetails.ToSharedRef()];
}

TSharedRef<SWidget> FKataActionEditor::MakeTaskClassMenu()
{
    FClassViewerInitializationOptions Options;
    Options.Mode = EClassViewerMode::ClassPicker;
    Options.bShowNoneOption = false;
    TSharedRef<FKataClassFilter> Filter = MakeShared<FKataClassFilter>();
    Filter->BaseClass = UKataTask::StaticClass();
    Options.ClassFilters.Add(Filter);
    FClassViewerModule& Classes = FModuleManager::LoadModuleChecked<FClassViewerModule>("ClassViewer");
    return SNew(SBox).WidthOverride(350).HeightOverride(400)
    [
        Classes.CreateClassViewer(Options, FOnClassPicked::CreateSP(this, &FKataActionEditor::AddTask))
    ];
}

TSharedRef<SWidget> FKataActionEditor::MakeResetMenu(bool bTask)
{
    FMenuBuilder Menu(true, nullptr);
    if (bTask)
    {
        if (const FKataTaskOverride* Override = Asset->TaskOverrides.FindByPredicate(
            [this](const FKataTaskOverride& Item) { return Item.TargetTaskId == SelectedId; }))
        {
            Menu.AddMenuEntry(FText::FromString(TEXT("All Properties")), FText::GetEmpty(), FSlateIcon(),
                FUIAction(FExecuteAction::CreateSP(this, &FKataActionEditor::ResetTaskProperty, FName())));
            for (FName Path : Override->OverriddenProperties)
            {
                Menu.AddMenuEntry(FText::FromName(Path), FText::GetEmpty(), FSlateIcon(),
                    FUIAction(FExecuteAction::CreateSP(this, &FKataActionEditor::ResetTaskProperty, Path)));
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
                FUIAction(FExecuteAction::CreateSP(this, &FKataActionEditor::ResetSetting, Path)));
        }
    }
    return Menu.MakeWidget();
}

TSharedPtr<SWidget> FKataActionEditor::MakeTimelineContextMenu(float Time, FGuid GroupId)
{
    // 우클릭한 위치를 새 태스크와 붙여넣기의 시작 시각으로 사용한다.
    InsertTime = FMath::Max(0.0f, Time);
    InsertGroupId = GroupId;
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
        FCanExecuteAction::CreateSP(this, &FKataActionEditor::CanPasteTask));
    FMenuBuilder Menu(true, MenuCommands);
    if (GroupId.IsValid())
    {
        Menu.BeginSection(TEXT("KataTimelineGroup"), NSLOCTEXT("Kata", "GroupSection", "Group"));
        Menu.AddSubMenu(
            NSLOCTEXT("Kata", "AddTaskToGroupLabel", "Add Task to Group"),
            NSLOCTEXT("Kata", "AddTaskToGroupTip", "Create a task at this time and place it in this group"),
            FNewMenuDelegate::CreateLambda([this](FMenuBuilder& SubMenu)
            {
                SubMenu.AddWidget(MakeTaskClassMenu(), FText::GetEmpty(), true);
            }));
        Menu.AddMenuEntry(
            NSLOCTEXT("Kata", "AddSelectedToGroupLabel", "Add Selected Tasks to Group"),
            NSLOCTEXT("Kata", "AddSelectedToGroupTip", "Move the currently selected tasks into this group"),
            FSlateIcon(),
            FUIAction(FExecuteAction::CreateSP(this, &FKataActionEditor::AddSelectedTasksToGroup, GroupId),
                FCanExecuteAction::CreateLambda([this]() { return !SelectedIds.IsEmpty(); })));
        Menu.AddSeparator();
        Menu.AddMenuEntry(
            NSLOCTEXT("Kata", "EditGroupLabel", "Edit Group"),
            NSLOCTEXT("Kata", "EditGroupTip", "Change this timeline group's title, color, and comment"),
            FSlateIcon(),
            FUIAction(FExecuteAction::CreateSP(this, &FKataActionEditor::RenameTimelineGroup, GroupId)));
        Menu.AddMenuEntry(
            NSLOCTEXT("Kata", "DeleteGroupLabel", "Delete Group"),
            NSLOCTEXT("Kata", "DeleteGroupTip", "Remove the group without deleting its tasks"),
            FSlateIcon(),
            FUIAction(FExecuteAction::CreateSP(this, &FKataActionEditor::DeleteTimelineGroup, GroupId)));
        Menu.EndSection();
        return Menu.MakeWidget();
    }
    InsertGroupId.Invalidate();
    Menu.BeginSection(TEXT("KataTask"), NSLOCTEXT("Kata", "TaskSection", "Task"));
    Menu.AddSubMenu(NSLOCTEXT("Kata", "AddTaskLabel", "Add Task"), NSLOCTEXT("Kata", "AddTaskTip", "Add a task at this time"),
        FNewMenuDelegate::CreateLambda([this](FMenuBuilder& SubMenu)
        {
            SubMenu.AddWidget(MakeTaskClassMenu(), FText::GetEmpty(), true);
        }));
    Menu.AddMenuEntry(Commands.Delete, NAME_None, NSLOCTEXT("Kata", "DeleteTaskLabel", "Delete Task"));
    Menu.AddMenuEntry(
        NSLOCTEXT("Kata", "GroupSelectedLabel", "Group Selected Tasks"),
        NSLOCTEXT("Kata", "GroupSelectedTip", "Create a timeline group from the selected tasks"),
        FSlateIcon(),
        FUIAction(FExecuteAction::CreateSP(this, &FKataActionEditor::GroupSelectedTasks),
            FCanExecuteAction::CreateSP(this, &FKataActionEditor::CanGroupSelectedTasks)));
#if WITH_EDITORONLY_DATA
    Menu.AddSubMenu(
        NSLOCTEXT("Kata", "MoveSelectedToGroupLabel", "Move Selected Tasks to Group"),
        NSLOCTEXT("Kata", "MoveSelectedToGroupTip", "Move the selected existing tasks into a timeline group"),
        FNewMenuDelegate::CreateLambda([this](FMenuBuilder& SubMenu)
        {
            if (!Asset || Asset->TimelineGroups.IsEmpty())
            {
                SubMenu.AddMenuEntry(
                    NSLOCTEXT("Kata", "NoExistingGroupsLabel", "No Existing Groups"),
                    FText::GetEmpty(), FSlateIcon(),
                    FUIAction(FExecuteAction(), FCanExecuteAction::CreateLambda([]() { return false; })));
                return;
            }
            for (const FKataTimelineGroup& Group : Asset->TimelineGroups)
            {
                const FGuid TargetGroupId = Group.GroupId;
                const FText GroupLabel = Group.Title.IsEmpty()
                    ? NSLOCTEXT("Kata", "UnnamedExistingGroup", "Unnamed Group") : Group.Title;
                SubMenu.AddMenuEntry(
                    GroupLabel,
                    NSLOCTEXT("Kata", "MoveSelectedToNamedGroupTip", "Move the selected tasks into this group"),
                    FSlateIcon(),
                    FUIAction(FExecuteAction::CreateSP(
                        this, &FKataActionEditor::AddSelectedTasksToGroup, TargetGroupId),
                        FCanExecuteAction::CreateLambda([this]() { return !SelectedIds.IsEmpty(); })));
            }
        }));
#endif
    Menu.AddMenuEntry(
        NSLOCTEXT("Kata", "UngroupSelectedLabel", "Ungroup Selected Tasks"),
        NSLOCTEXT("Kata", "UngroupSelectedTip", "Remove the selected tasks from their timeline groups"),
        FSlateIcon(),
        FUIAction(FExecuteAction::CreateSP(this, &FKataActionEditor::UngroupSelectedTasks),
            FCanExecuteAction::CreateSP(this, &FKataActionEditor::CanUngroupSelectedTasks)));
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

void FKataActionEditor::ExtendToolbar()
{
    const TSharedRef<FExtender> Extender = MakeShared<FExtender>();
    Extender->AddToolBarExtension(TEXT("Asset"), EExtensionHook::After, GetToolkitCommands(),
        FToolBarExtensionDelegate::CreateSP(this, &FKataActionEditor::FillToolbar));
    AddToolbarExtender(Extender);
}

void FKataActionEditor::FillToolbar(FToolBarBuilder& Builder)
{
    Builder.BeginSection(TEXT("KataPreview"));
    Builder.AddToolBarButton(
        FUIAction(
            FExecuteAction::CreateSP(this, &FKataActionEditor::TogglePreviewSlot, EKataPreviewActorSlot::Self),
            FCanExecuteAction(),
            FIsActionChecked::CreateSP(this, &FKataActionEditor::IsPreviewSlotActive, EKataPreviewActorSlot::Self)),
        NAME_None,
        NSLOCTEXT("Kata", "SelectSelf", "Select Self"),
        NSLOCTEXT("Kata", "SelectSelfTip",
            "Move the preview self actor with the transform widget. W and E switch move and rotate. "
            "A Character preview actor falls to the floor, so its height is not preserved."),
        FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("EditorViewport.TranslateMode")),
        EUserInterfaceActionType::ToggleButton);
    Builder.AddToolBarButton(
        FUIAction(
            FExecuteAction::CreateSP(this, &FKataActionEditor::TogglePreviewSlot, EKataPreviewActorSlot::Target),
            FCanExecuteAction(),
            FIsActionChecked::CreateSP(this, &FKataActionEditor::IsPreviewSlotActive, EKataPreviewActorSlot::Target)),
        NAME_None,
        NSLOCTEXT("Kata", "SelectTarget", "Select Target"),
        NSLOCTEXT("Kata", "SelectTargetTip",
            "Move the preview target actor with the transform widget. W and E switch move and rotate."),
        FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("EditorViewport.TranslateMode")),
        EUserInterfaceActionType::ToggleButton);
    Builder.AddToolBarButton(
        FUIAction(FExecuteAction::CreateSP(this, &FKataActionEditor::ResizeViewToTasks)),
        NAME_None,
        NSLOCTEXT("Kata", "ResizeView", "Resize"),
        NSLOCTEXT("Kata", "ResizeViewTip", "Fit the timeline view to the task that ends last."),
        FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("Icons.Adjust")));
    Builder.EndSection();
}

void FKataActionEditor::TogglePreviewSlot(EKataPreviewActorSlot Slot)
{
    if (!Preview.IsValid())
    {
        return;
    }
    // 같은 버튼을 다시 누르면 해제하고, 다른 버튼을 누르면 그 자리로 옮긴다.
    // 자리를 하나만 유지하므로 두 버튼이 동시에 켜지지 않는다.
    const bool bAlreadyActive = Preview->GetManipulatedSlot() == Slot;
    Preview->SetManipulatedSlot(bAlreadyActive ? EKataPreviewActorSlot::None : Slot);
}

bool FKataActionEditor::IsPreviewSlotActive(EKataPreviewActorSlot Slot) const
{
    return Preview.IsValid() && Preview->GetManipulatedSlot() == Slot;
}

void FKataActionEditor::ResizeViewToTasks()
{
    float End = 0.0f;
    if (EditingAction)
    {
        for (const UKataTask* Task : EditingAction->Tasks)
        {
            End = FMath::Max(End, Task->StartTime + Task->Duration);
        }
    }
    // 태스크가 없으면 기본 범위를 유지한다.
    TimelineLength = End > UE_KINDA_SMALL_NUMBER ? End : 5.0f;
    SaveEditorSettings();
}

bool FKataActionEditor::CanGroupSelectedTasks() const
{
    return Asset != nullptr && !SelectedIds.IsEmpty();
}

bool FKataActionEditor::CanUngroupSelectedTasks() const
{
#if WITH_EDITORONLY_DATA
    if (!Asset || SelectedIds.IsEmpty())
    {
        return false;
    }
    for (const FKataTimelineGroup& Group : Asset->TimelineGroups)
    {
        for (const FKataTaskId& TaskId : Group.TaskIds)
        {
            if (SelectedIds.Contains(TaskId))
            {
                return true;
            }
        }
    }
#endif
    return false;
}

void FKataActionEditor::GroupSelectedTasks()
{
#if WITH_EDITORONLY_DATA
    if (!CanGroupSelectedTasks())
    {
        return;
    }
    FGuid NewGroupId;
    {
        const FScopedTransaction Transaction(NSLOCTEXT("Kata", "GroupTasks", "Group Kata Tasks"));
        Asset->Modify();

        // 한 태스크는 한 그룹에만 속한다.
        for (FKataTimelineGroup& Group : Asset->TimelineGroups)
        {
            Group.TaskIds.RemoveAll([this](const FKataTaskId& TaskId) { return SelectedIds.Contains(TaskId); });
        }
        Asset->TimelineGroups.RemoveAll([](const FKataTimelineGroup& Group) { return Group.TaskIds.IsEmpty(); });

        FKataTimelineGroup& Group = Asset->TimelineGroups.AddDefaulted_GetRef();
        Group.GroupId = FGuid::NewGuid();
        NewGroupId = Group.GroupId;
        Group.Title = FText::Format(NSLOCTEXT("Kata", "DefaultGroupTitle", "Group {0}"),
            FText::AsNumber(Asset->TimelineGroups.Num()));
        Group.DisplayColor = MakeStableTimelineColor(Group.GroupId);
        if (EditingAction)
        {
            for (const UKataTask* Task : EditingAction->Tasks)
            {
                if (Task && SelectedIds.Contains(Task->TaskId))
                {
                    Group.TaskIds.Add(Task->TaskId);
                }
            }
        }
        Changed();
    }
    SelectTimelineGroup(NewGroupId);
#endif
}

void FKataActionEditor::AddSelectedTasksToGroup(FGuid GroupId)
{
#if WITH_EDITORONLY_DATA
    if (!Asset || !GroupId.IsValid() || SelectedIds.IsEmpty())
    {
        return;
    }
    const FScopedTransaction Transaction(NSLOCTEXT("Kata", "AddTasksToGroup", "Add Tasks to Kata Timeline Group"));
    Asset->Modify();

    // 한 태스크는 한 그룹에만 속하므로 기존 그룹에서 먼저 제거한다.
    for (FKataTimelineGroup& Group : Asset->TimelineGroups)
    {
        if (Group.GroupId != GroupId)
        {
            Group.TaskIds.RemoveAll([this](const FKataTaskId& TaskId)
            {
                return SelectedIds.Contains(TaskId);
            });
        }
    }
    Asset->TimelineGroups.RemoveAll([GroupId](const FKataTimelineGroup& Group)
    {
        return Group.GroupId != GroupId && Group.TaskIds.IsEmpty();
    });

    if (FKataTimelineGroup* TargetGroup = Asset->TimelineGroups.FindByPredicate(
        [GroupId](const FKataTimelineGroup& Group) { return Group.GroupId == GroupId; }))
    {
        // 현재 액션의 표시 순서를 유지해 그룹 안에서도 태스크가 예측 가능한 순서로 보이게 한다.
        if (EditingAction)
        {
            for (const UKataTask* Task : EditingAction->Tasks)
            {
                if (Task && SelectedIds.Contains(Task->TaskId))
                {
                    TargetGroup->TaskIds.AddUnique(Task->TaskId);
                }
            }
        }
        CollapsedTimelineGroups.Remove(GroupId);
        Changed();
    }
#endif
}

void FKataActionEditor::UngroupSelectedTasks()
{
#if WITH_EDITORONLY_DATA
    if (!CanUngroupSelectedTasks())
    {
        return;
    }
    const FScopedTransaction Transaction(NSLOCTEXT("Kata", "UngroupTasks", "Ungroup Kata Tasks"));
    Asset->Modify();
    for (FKataTimelineGroup& Group : Asset->TimelineGroups)
    {
        Group.TaskIds.RemoveAll([this](const FKataTaskId& TaskId) { return SelectedIds.Contains(TaskId); });
    }
    Asset->TimelineGroups.RemoveAll([](const FKataTimelineGroup& Group) { return Group.TaskIds.IsEmpty(); });
    Changed();
#endif
}

void FKataActionEditor::ToggleTimelineGroup(FGuid GroupId)
{
    if (!GroupId.IsValid())
    {
        return;
    }
    if (CollapsedTimelineGroups.Contains(GroupId))
    {
        CollapsedTimelineGroups.Remove(GroupId);
    }
    else
    {
        CollapsedTimelineGroups.Add(GroupId);
    }
    SaveEditorSettings();
    RefreshRows();
}

void FKataActionEditor::SelectTimelineGroup(FGuid GroupId)
{
#if WITH_EDITORONLY_DATA
    if (!Asset || !Asset->TimelineGroups.ContainsByPredicate(
        [GroupId](const FKataTimelineGroup& Group) { return Group.GroupId == GroupId; }))
    {
        return;
    }
    SelectedGroupId = GroupId;
    SelectedId.Invalidate();
    SelectedIds.Reset();
    RefreshTaskDetails();
    RefreshRows();
#endif
}

void FKataActionEditor::RenameTimelineGroup(FGuid GroupId)
{
#if WITH_EDITORONLY_DATA
    if (!Asset)
    {
        return;
    }
    FKataTimelineGroup* Group = Asset->TimelineGroups.FindByPredicate(
        [GroupId](const FKataTimelineGroup& Entry) { return Entry.GroupId == GroupId; });
    if (!Group)
    {
        return;
    }

    TSharedPtr<SEditableTextBox> TitleBox;
    TSharedPtr<SMultiLineEditableTextBox> CommentBox;
    const TSharedRef<FLinearColor> EditedColor = MakeShared<FLinearColor>(Group->DisplayColor);
    bool bAccepted = false;
    const TSharedRef<SWindow> Window = SNew(SWindow)
        .Title(NSLOCTEXT("Kata", "EditGroupWindow", "Edit Timeline Group"))
        .ClientSize(FVector2D(420.0f, 280.0f))
        .SupportsMinimize(false)
        .SupportsMaximize(false);
    Window->SetContent(
        SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight().Padding(8, 8, 8, 2)
        [
            SNew(STextBlock).Text(NSLOCTEXT("Kata", "GroupTitleLabel", "Title"))
        ]
        + SVerticalBox::Slot().AutoHeight().Padding(8, 2)
        [
            SAssignNew(TitleBox, SEditableTextBox).Text(Group->Title)
        ]
        + SVerticalBox::Slot().AutoHeight().Padding(8, 8, 8, 2)
        [
            SNew(STextBlock).Text(NSLOCTEXT("Kata", "GroupColorLabel", "Display Color (RGB)"))
        ]
        + SVerticalBox::Slot().AutoHeight().Padding(8, 2)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 8, 0)
            [
                SNew(SColorBlock).Color_Lambda([EditedColor]() { return *EditedColor; }).Size(FVector2D(42.0f, 22.0f))
            ]
            + SHorizontalBox::Slot().FillWidth(1).Padding(2, 0)
            [
                SNew(SSpinBox<float>).MinValue(0.0f).MaxValue(1.0f).Delta(0.01f)
                .Value_Lambda([EditedColor]() { return EditedColor->R; })
                .OnValueChanged_Lambda([EditedColor](float Value) { EditedColor->R = Value; })
            ]
            + SHorizontalBox::Slot().FillWidth(1).Padding(2, 0)
            [
                SNew(SSpinBox<float>).MinValue(0.0f).MaxValue(1.0f).Delta(0.01f)
                .Value_Lambda([EditedColor]() { return EditedColor->G; })
                .OnValueChanged_Lambda([EditedColor](float Value) { EditedColor->G = Value; })
            ]
            + SHorizontalBox::Slot().FillWidth(1).Padding(2, 0)
            [
                SNew(SSpinBox<float>).MinValue(0.0f).MaxValue(1.0f).Delta(0.01f)
                .Value_Lambda([EditedColor]() { return EditedColor->B; })
                .OnValueChanged_Lambda([EditedColor](float Value) { EditedColor->B = Value; })
            ]
        ]
        + SVerticalBox::Slot().AutoHeight().Padding(8, 8, 8, 2)
        [
            SNew(STextBlock).Text(NSLOCTEXT("Kata", "GroupCommentLabel", "Comment"))
        ]
        + SVerticalBox::Slot().FillHeight(1).Padding(8, 2)
        [
            SAssignNew(CommentBox, SMultiLineEditableTextBox).Text(Group->EditorComment)
        ]
        + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(8)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth().Padding(2)
            [
                SNew(SButton).Text(NSLOCTEXT("Kata", "EditGroupOk", "OK"))
                .OnClicked_Lambda([&bAccepted, Window]()
                {
                    bAccepted = true;
                    Window->RequestDestroyWindow();
                    return FReply::Handled();
                })
            ]
            + SHorizontalBox::Slot().AutoWidth().Padding(2)
            [
                SNew(SButton).Text(NSLOCTEXT("Kata", "EditGroupCancel", "Cancel"))
                .OnClicked_Lambda([Window]()
                {
                    Window->RequestDestroyWindow();
                    return FReply::Handled();
                })
            ]
        ]);
    FSlateApplication::Get().AddModalWindow(Window, nullptr);

    if (bAccepted && TitleBox.IsValid() && CommentBox.IsValid() && !TitleBox->GetText().IsEmpty())
    {
        const FScopedTransaction Transaction(NSLOCTEXT("Kata", "EditGroup", "Edit Kata Timeline Group"));
        Asset->Modify();
        Group->Title = TitleBox->GetText();
        Group->DisplayColor = *EditedColor;
        Group->DisplayColor.A = 1.0f;
        Group->EditorComment = CommentBox->GetText();
        Changed();
    }
#endif
}

void FKataActionEditor::DeleteTimelineGroup(FGuid GroupId)
{
#if WITH_EDITORONLY_DATA
    if (!Asset || !GroupId.IsValid())
    {
        return;
    }
    const FScopedTransaction Transaction(NSLOCTEXT("Kata", "DeleteGroup", "Delete Kata Timeline Group"));
    Asset->Modify();
    Asset->TimelineGroups.RemoveAll([GroupId](const FKataTimelineGroup& Group) { return Group.GroupId == GroupId; });
    if (SelectedGroupId == GroupId)
    {
        SelectedGroupId.Invalidate();
    }
    CollapsedTimelineGroups.Remove(GroupId);
    SaveEditorSettings();
    Changed();
#endif
}

void FKataActionEditor::LoadEditorSettings()
{
    // 이전 설정 키를 한 번 읽어 기존 사용자의 표시 범위를 유지한다.
    if (!GConfig->GetFloat(EditorSettingsSection, TEXT("TimelineLength"), TimelineLength, GEditorPerProjectIni))
    {
        GConfig->GetFloat(EditorSettingsSection, TEXT("ViewDuration"), TimelineLength, GEditorPerProjectIni);
    }
    TimelineLength = FMath::Clamp(TimelineLength, 0.1f, 3600.0f);
    int32 CommentDisplayValue = static_cast<int32>(EKataTimelineCommentDisplay::Tooltip);
    GConfig->GetInt(EditorSettingsSection, TEXT("TaskCommentDisplay"), CommentDisplayValue, GEditorPerProjectIni);
    CommentDisplay = static_cast<EKataTimelineCommentDisplay>(FMath::Clamp(CommentDisplayValue,
        static_cast<int32>(EKataTimelineCommentDisplay::Hidden), static_cast<int32>(EKataTimelineCommentDisplay::Inline)));
    GConfig->GetBool(EditorSettingsSection, TEXT("PreviewRepeat"), bPreviewRepeat, GEditorPerProjectIni);

    CollapsedTimelineGroups.Reset();
    if (Asset)
    {
        FString AssetKey = Asset->GetPathName();
        AssetKey.ReplaceInline(TEXT("/"), TEXT("_"));
        AssetKey.ReplaceInline(TEXT("."), TEXT("_"));
        FString SerializedGroups;
        GConfig->GetString(EditorSettingsSection,
            *FString::Printf(TEXT("CollapsedGroups.%s"), *AssetKey), SerializedGroups, GEditorPerProjectIni);
        TArray<FString> GroupStrings;
        SerializedGroups.ParseIntoArray(GroupStrings, TEXT(","), true);
        for (const FString& GroupString : GroupStrings)
        {
            FGuid GroupId;
            if (FGuid::Parse(GroupString, GroupId))
            {
                CollapsedTimelineGroups.Add(GroupId);
            }
        }
    }
}

void FKataActionEditor::SaveEditorSettings() const
{
    GConfig->SetFloat(EditorSettingsSection, TEXT("TimelineLength"), TimelineLength, GEditorPerProjectIni);
    GConfig->SetInt(EditorSettingsSection, TEXT("TaskCommentDisplay"),
        static_cast<int32>(CommentDisplay), GEditorPerProjectIni);
    GConfig->SetBool(EditorSettingsSection, TEXT("PreviewRepeat"), bPreviewRepeat, GEditorPerProjectIni);
    if (Asset)
    {
        FString AssetKey = Asset->GetPathName();
        AssetKey.ReplaceInline(TEXT("/"), TEXT("_"));
        AssetKey.ReplaceInline(TEXT("."), TEXT("_"));
        TArray<FString> GroupStrings;
        for (const FGuid& GroupId : CollapsedTimelineGroups)
        {
            GroupStrings.Add(GroupId.ToString(EGuidFormats::DigitsWithHyphens));
        }
        GroupStrings.Sort();
        GConfig->SetString(EditorSettingsSection,
            *FString::Printf(TEXT("CollapsedGroups.%s"), *AssetKey), *FString::Join(GroupStrings, TEXT(",")),
            GEditorPerProjectIni);
    }
    GConfig->Flush(false, GEditorPerProjectIni);
}

void FKataActionEditor::ApplyPreviewActorTransform(EKataPreviewActorSlot Slot, const FTransform& Transform)
{
    if (!Asset || Slot == EKataPreviewActorSlot::None)
    {
        return;
    }
    const bool bSelf = Slot == EKataPreviewActorSlot::Self;
    const FScopedTransaction Transaction(bSelf
        ? NSLOCTEXT("Kata", "MoveSelf", "Move Kata Preview Self")
        : NSLOCTEXT("Kata", "MoveTarget", "Move Kata Preview Target"));
    Asset->Modify();
    // 중력을 받는 Character는 착지하면서 Z가 달라지지만 기록은 조작을 끝낸 시점의 값으로 남긴다.
    FTransform& AssetTransform = bSelf ? Asset->PreviewActorTransform : Asset->PreviewTargetTransform;
    AssetTransform = Transform;
    if (Settings)
    {
        (bSelf ? Settings->PreviewActorTransform : Settings->PreviewTargetTransform) = Transform;
    }
    Asset->MarkPackageDirty();
    // 장면을 다시 만들지 않고 값만 갱신해 위젯 조작을 이어서 할 수 있게 한다.
    PreviewDetails->ForceRefresh();
}

void FKataActionEditor::BindCommands()
{
    const FGenericCommands& Commands = FGenericCommands::Get();
    TimelineCommands->MapAction(Commands.Delete,
        FExecuteAction::CreateSP(this, &FKataActionEditor::DeleteSelectedTask),
        FCanExecuteAction::CreateSP(this, &FKataActionEditor::CanDeleteTask));
    TimelineCommands->MapAction(Commands.Copy,
        FExecuteAction::CreateSP(this, &FKataActionEditor::CopySelectedTask),
        FCanExecuteAction::CreateSP(this, &FKataActionEditor::CanCopyTask));
    // 단축키로 붙여넣을 때는 재생 헤드 위치를 시작 시각으로 쓴다.
    TimelineCommands->MapAction(Commands.Paste,
        FExecuteAction::CreateLambda([this]()
        {
            InsertTime = Preview->GetTime();
            PasteTask();
        }),
        FCanExecuteAction::CreateSP(this, &FKataActionEditor::CanPasteTask));
    const FExecuteAction Undo = FExecuteAction::CreateLambda([]() { GEditor->UndoTransaction(); });
    const FExecuteAction Redo = FExecuteAction::CreateLambda([]() { GEditor->RedoTransaction(); });
    TimelineCommands->MapAction(Commands.Undo, Undo);
    TimelineCommands->MapAction(Commands.Redo, Redo);
    // 실행 취소는 타임라인 밖에서도 동작하도록 툴킷 명령에도 연결한다.
    GetToolkitCommands()->MapAction(Commands.Undo, Undo);
    GetToolkitCommands()->MapAction(Commands.Redo, Redo);
}

void FKataActionEditor::Refresh()
{
    bRefreshQueued = false;
    Settings = Asset->MakeEffectiveSettings(GetTransientPackage());
    EditingAction = Asset->Resolve(GetTransientPackage(), true);
    SettingsDetails->SetObject(Settings, true);
    PreviewDetails->SetObject(Settings, true);
    ExpandDetailsByDefault(SettingsDetails, TEXT("KataEditor.SettingsDetails"), Settings->GetClass());
    ExpandDetailsByDefault(PreviewDetails, TEXT("KataEditor.PreviewDetails"), Settings->GetClass());
    for (UKataTask* Task : EditingAction->Tasks)
    {
        Task->SetFlags(RF_Transactional);
    }
    for (auto It = SelectedIds.CreateIterator(); It; ++It)
    {
        if (!EditingAction->FindTask(*It))
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

    UKataResolvedAction* RuntimeDefinition = Asset->Resolve(GetTransientPackage());
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

void FKataActionEditor::RefreshRows()
{
    TArray<FKataTimelineRow> Rows;
    if (!EditingAction)
    {
        Timeline->SetRows(MoveTemp(Rows), SelectedIds);
        return;
    }

    auto AddTaskRow = [&Rows, this](UKataTask* Task, const FKataTimelineGroup* Group)
    {
        if (!Task)
        {
            return;
        }
        FKataTimelineRow& Row = Rows.AddDefaulted_GetRef();
        Row.Id = Task->TaskId;
        Row.Label = Task->GetDisplayName();
        Row.Start = Task->StartTime;
        // 한 프레임 태스크는 길이를 차지하지 않으므로 최소 폭 표식으로 그려진다.
        Row.Duration = Task->bSingleFrame ? 0.0f : Task->Duration;
        Row.bSingleFrame = Task->bSingleFrame;
        Row.bEnabled = Task->bEnabled;
        Row.bInherited = !IsLocalTask(Row.Id);
        if (Group)
        {
            Row.GroupId = Group->GroupId;
            Row.GroupColor = Group->DisplayColor;
        }
#if WITH_EDITORONLY_DATA
        Row.DisplayColor = Task->bUseAutomaticTimelineColor
            ? MakeStableTimelineColor(Task->TaskId.Value) : Task->TimelineDisplayColor;
        Row.Comment = Task->EditorComment.ToString();
#endif
    };

    TMap<FKataTaskId, UKataTask*> TasksById;
    for (UKataTask* Task : EditingAction->Tasks)
    {
        if (Task)
        {
            TasksById.Add(Task->TaskId, Task);
        }
    }

    TSet<FKataTaskId> GroupedTaskIds;
#if WITH_EDITORONLY_DATA
    if (Asset)
    {
        for (const FKataTimelineGroup& Group : Asset->TimelineGroups)
        {
            if (!Group.GroupId.IsValid())
            {
                continue;
            }
            FKataTimelineRow& Header = Rows.AddDefaulted_GetRef();
            Header.GroupId = Group.GroupId;
            Header.Label = Group.Title.IsEmpty() ? TEXT("Group") : Group.Title.ToString();
            Header.bGroupHeader = true;
            Header.bGroupCollapsed = CollapsedTimelineGroups.Contains(Group.GroupId);
            Header.bGroupSelected = SelectedGroupId == Group.GroupId;
            Header.DisplayColor = Group.DisplayColor;
            Header.Comment = Group.EditorComment.ToString();

            if (!Header.bGroupCollapsed)
            {
                for (const FKataTaskId& TaskId : Group.TaskIds)
                {
                    if (UKataTask** Task = TasksById.Find(TaskId); Task && !GroupedTaskIds.Contains(TaskId))
                    {
                        AddTaskRow(*Task, &Group);
                        GroupedTaskIds.Add(TaskId);
                    }
                }
            }
            else
            {
                for (const FKataTaskId& TaskId : Group.TaskIds)
                {
                    if (TasksById.Contains(TaskId))
                    {
                        GroupedTaskIds.Add(TaskId);
                    }
                }
            }
        }
    }
#endif

    for (UKataTask* Task : EditingAction->Tasks)
    {
        if (Task && !GroupedTaskIds.Contains(Task->TaskId))
        {
            AddTaskRow(Task, nullptr);
        }
    }
    Timeline->SetRows(MoveTemp(Rows), SelectedIds);
}

bool FKataActionEditor::IsLocalTask(FKataTaskId Id) const
{
    return Asset->TimelineTasks.ContainsByPredicate([Id](const FKataTimelineEntry& Entry)
        { return Entry.Task && Entry.Task->TaskId == Id; });
}

UKataTask* FKataActionEditor::GetSelectedTask() const
{
    return EditingAction ? const_cast<UKataTask*>(EditingAction->FindTask(SelectedId)) : nullptr;
}

TArray<UKataTask*> FKataActionEditor::GetSelectedTasks() const
{
    TArray<UKataTask*> Result;
    if (EditingAction)
    {
        for (UKataTask* Task : EditingAction->Tasks)
        {
            if (Task && SelectedIds.Contains(Task->TaskId))
            {
                Result.Add(Task);
            }
        }
    }
    return Result;
}

void FKataActionEditor::RefreshTaskDetails()
{
#if WITH_EDITORONLY_DATA
    if (SelectedGroupId.IsValid() && Asset && GroupDetails)
    {
        if (const FKataTimelineGroup* Group = Asset->TimelineGroups.FindByPredicate(
            [this](const FKataTimelineGroup& Entry) { return Entry.GroupId == SelectedGroupId; }))
        {
            GroupDetails->Title = Group->Title;
            GroupDetails->DisplayColor = Group->DisplayColor;
            GroupDetails->EditorComment = Group->EditorComment;
            GroupDetails->TaskCount = Group->TaskIds.Num();
            TaskDetails->SetObject(GroupDetails, true);
            ExpandDetailsByDefault(TaskDetails, TEXT("KataEditor.TimelineDetails"), GroupDetails->GetClass());
            return;
        }
        SelectedGroupId.Invalidate();
    }
#endif
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

void FKataActionEditor::SelectTask(FKataTaskId Id, bool bToggle)
{
    SelectedGroupId.Invalidate();
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

void FKataActionEditor::AddTask(UClass* Class)
{
    FSlateApplication::Get().DismissAllMenus();
    if (!Class || !Class->IsChildOf(UKataTask::StaticClass()) || Class->HasAnyClassFlags(CLASS_Abstract))
    {
        return;
    }
    const FScopedTransaction Transaction(NSLOCTEXT("Kata", "AddTask", "Add Kata Task"));
    Asset->Modify();
    UKataTask* Task = NewObject<UKataTask>(Asset, Class, NAME_None, RF_Transactional);
    // 새로 만든 태스크를 트랜잭션에 등록한다. 등록하지 않으면 Undo로 배열에서 빠졌을 때
    // 태스크를 참조하는 프로퍼티가 사라져 GC 대상이 되고, Redo가 배열만 되살려
    // Task가 null인 엔트리가 남는다.
    Task->Modify();
    Task->TaskId = FKataTaskId::NewId();
    Task->Duration = 1.0f;
    Task->StartTime = InsertTime;
    Asset->TimelineTasks.AddDefaulted_GetRef().Task = Task;
#if WITH_EDITORONLY_DATA
    if (InsertGroupId.IsValid())
    {
        if (FKataTimelineGroup* Group = Asset->TimelineGroups.FindByPredicate(
            [this](const FKataTimelineGroup& Entry) { return Entry.GroupId == InsertGroupId; }))
        {
            Group->TaskIds.Add(Task->TaskId);
            CollapsedTimelineGroups.Remove(InsertGroupId);
        }
    }
#endif
    InsertGroupId.Invalidate();
    SelectedGroupId.Invalidate();
    SelectedId = Task->TaskId;
    SelectedIds.Reset();
    SelectedIds.Add(SelectedId);
    Changed();
}

void FKataActionEditor::ApplyTaskProperty(UKataTask* Edited, FName Path)
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

void FKataActionEditor::MoveTask(FKataTaskId Id, float Start, float Duration)
{
    UKataTask* Task = EditingAction ? const_cast<UKataTask*>(EditingAction->FindTask(Id)) : nullptr;
    if (!Task || (Task->StartTime == Start && (Task->bSingleFrame || Task->Duration == Duration)))
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
    // 한 프레임 태스크의 길이는 타임라인에서 바꾸지 않는다.
    if (!Task->bSingleFrame && Task->Duration != Duration)
    {
        Task->Duration = Duration;
        ApplyTaskProperty(Task, GET_MEMBER_NAME_CHECKED(UKataTask, Duration));
    }
    Changed();
}

void FKataActionEditor::OnTaskEdited(const FPropertyChangedEvent& Event)
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

void FKataActionEditor::OnGroupDetailsEdited(const FPropertyChangedEvent& Event)
{
#if WITH_EDITORONLY_DATA
    if (!SelectedGroupId.IsValid() || !Asset || !GroupDetails || !Event.MemberProperty)
    {
        return;
    }
    FKataTimelineGroup* Group = Asset->TimelineGroups.FindByPredicate(
        [this](const FKataTimelineGroup& Entry) { return Entry.GroupId == SelectedGroupId; });
    if (!Group)
    {
        return;
    }
    const FScopedTransaction Transaction(NSLOCTEXT("Kata", "EditGroupDetails", "Edit Kata Timeline Group"));
    Asset->Modify();
    Group->Title = GroupDetails->Title;
    Group->DisplayColor = GroupDetails->DisplayColor;
    Group->DisplayColor.A = 1.0f;
    Group->EditorComment = GroupDetails->EditorComment;
    Changed();
#endif
}

void FKataActionEditor::OnSettingsEdited(const FPropertyChangedEvent& Event)
{
    if (!Event.MemberProperty)
    {
        return;
    }
    const FName Path = KataPropertyOverride::GetPropertyPath(Event.MemberProperty, Event.Property);
    const FName RootName = Event.MemberProperty->GetFName();
    const FScopedTransaction Transaction(NSLOCTEXT("Kata", "EditSettings", "Edit Kata Settings"));
    Asset->Modify();
    if (RootName == GET_MEMBER_NAME_CHECKED(UKataAction, ParentAction))
    {
        // 부모를 바꾸기 전에 변경 후보의 상속 체인에 자신이 포함되는지 확인한다.
        TArray<const UKataAction*> Chain;
        const bool bValid = !Settings->ParentAction
            || (Settings->ParentAction->CollectActionChain(Chain) && !Chain.Contains(Asset));
        if (!bValid)
        {
            Diagnostics = TEXT("Parent Kata would create an inheritance cycle");
            Settings->ParentAction = Asset->ParentAction;
            return;
        }
    }
    FString Error;
    if (KataPropertyOverride::CopyOverriddenProperty(Asset, Settings, Path, Error))
    {
        if (Asset->ParentAction && RootName != GET_MEMBER_NAME_CHECKED(UKataAction, ParentAction)
            && RootName != GET_MEMBER_NAME_CHECKED(UKataAction, OverriddenSettings)
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

void FKataActionEditor::ResetSetting(FName Path)
{
    const FScopedTransaction Transaction(NSLOCTEXT("Kata", "ResetSetting", "Reset Kata Setting"));
    Asset->Modify();
    Asset->OverriddenSettings.Remove(Path);
    Changed();
}

void FKataActionEditor::ResetTaskProperty(FName Path)
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

bool FKataActionEditor::CanDeleteTask() const
{
    return !GetSelectedTasks().IsEmpty();
}

bool FKataActionEditor::CanCopyTask() const
{
    return SelectedIds.Num() == 1 && GetSelectedTask() != nullptr;
}

bool FKataActionEditor::CanPasteTask() const
{
    return TaskClipboard.IsValid();
}

void FKataActionEditor::CopySelectedTask()
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

void FKataActionEditor::PasteTask()
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
    // AddTask와 같은 이유로 복제한 태스크도 트랜잭션에 등록한다.
    Task->Modify();
    // 붙여넣은 항목은 부모·자식 오버라이드와 겹치지 않도록 새 ID를 받는다.
    Task->TaskId = FKataTaskId::NewId();
    Task->StartTime = FMath::Max(0.0f, InsertTime);
    Asset->TimelineTasks.AddDefaulted_GetRef().Task = Task;
    SelectedId = Task->TaskId;
    SelectedIds.Reset();
    SelectedIds.Add(SelectedId);
    Changed();
}

void FKataActionEditor::DeleteSelectedTask()
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
            // 배열에서 빼기 전에 트랜잭션이 태스크를 참조로 붙잡게 한다.
            // 붙잡지 않으면 제거 직후 GC 대상이 되어 Undo가 빈 엔트리를 되살린다.
            Task->Modify();
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
#if WITH_EDITORONLY_DATA
        for (FKataTimelineGroup& Group : Asset->TimelineGroups)
        {
            Group.TaskIds.Remove(Id);
        }
#endif
    }
#if WITH_EDITORONLY_DATA
    Asset->TimelineGroups.RemoveAll([](const FKataTimelineGroup& Group) { return Group.TaskIds.IsEmpty(); });
#endif
    SelectedId.Invalidate();
    SelectedIds.Reset();
    Changed();
}

FReply FKataActionEditor::CreateChild()
{
    UKataActionFactory* Factory = NewObject<UKataActionFactory>();
    Factory->ParentAction = Asset;
    FAssetToolsModule& Tools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
    UObject* Child = Tools.Get().CreateAssetWithDialog(Asset->GetName() + TEXT("_Child"),
        FPackageName::GetLongPackagePath(Asset->GetOutermost()->GetName()), UKataAction::StaticClass(), Factory);
    if (Child)
    {
        GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Child);
    }
    return FReply::Handled();
}

void FKataActionEditor::SeekFromTimeInput(float Value)
{
    // SSpinBox는 같은 값을 확정하거나 포커스를 잃을 때도 콜백을 보낸다.
    // 표시값을 되돌려 받은 경우에는 실행 인스턴스를 스크럽용 장면으로 교체하지 않는다.
    if (FMath::IsFinite(Value) && !FMath::IsNearlyEqual(Value, Preview->GetTime(), UE_KINDA_SMALL_NUMBER))
    {
        const float RequestedTime = FMath::Clamp(Value, 0.0f, TimelineLength);
        if (!FMath::IsNearlyEqual(RequestedTime, Preview->GetTime(), UE_KINDA_SMALL_NUMBER))
        {
            Preview->Seek(Asset, EditingAction, RequestedTime);
        }
    }
}

void FKataActionEditor::Changed()
{
    Asset->MarkPackageDirty();
    Preview->Stop();
    bRefreshQueued = true;
    // 부모 에셋을 열어 둔 다른 Kata 에디터에도 갱신을 알린다.
    FPropertyChangedEvent Event(nullptr);
    FCoreUObjectDelegates::OnObjectPropertyChanged.Broadcast(Asset, Event);
}

void FKataActionEditor::OnObjectChanged(UObject* Object, FPropertyChangedEvent& Event)
{
    TArray<const UKataAction*> Chain;
    if (!Asset->CollectActionChain(Chain))
    {
        return;
    }
    for (const UKataAction* Entry : Chain)
    {
        if (Object == Entry || (Object && Object->IsIn(Entry)))
        {
            bRefreshQueued = true;
            return;
        }
    }
}

void FKataActionEditor::Tick(float DeltaTime)
{
    if (bRefreshQueued)
    {
        // Details 콜백 안에서 패널을 재구성하지 않는다.
        Preview->ResetScene(Asset);
        Refresh();
    }
    Preview->TickSimulation(DeltaTime);
    if (Preview->HasCompletedPlayback())
    {
        if (bPreviewRepeat)
        {
            // 반복은 편집기 설정이므로 에셋을 건드리지 않고 프리뷰만 처음부터 다시 실행한다.
            Preview->Play(Asset);
        }
        else
        {
            // 끝난 뒤 남은 위치·포즈를 다음 재생 전에 치우도록 Stop / Reset과 같은 초기화를 바로 수행한다.
            // 에셋 Loop Policy의 반복은 인스턴스 안에서 이어지므로 마지막 회차가 끝난 뒤에만 여기에 온다.
            Preview->ResetScene(Asset);
        }
    }
    // 재생·탐색 중 바뀌는 Attribute를 타임라인의 보존 렌더링에 즉시 반영한다.
    Timeline->Invalidate(EInvalidateWidgetReason::Paint);
}

void FKataActionEditor::PostUndo(bool bSuccess)
{
    if (bSuccess)
    {
        bRefreshQueued = true;
    }
}

void FKataActionEditor::AddReferencedObjects(FReferenceCollector& Collector)
{
    Collector.AddReferencedObject(Asset);
    Collector.AddReferencedObject(Settings);
    Collector.AddReferencedObject(EditingAction);
    Collector.AddReferencedObject(GroupDetails);
}
