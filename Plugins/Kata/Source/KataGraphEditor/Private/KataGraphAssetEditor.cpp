#include "KataGraphAssetEditor.h"
#include "KataGraphEditorPrivate.h"
#include "KataGraphEditorToolbar.h"
#include "KataGraphSchema.h"
#include "KataGraphEditorCommands.h"
#include "KataEdGraph.h"
#include "AssetToolsModule.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Framework/Commands/GenericCommands.h"
#include "GraphEditorActions.h"
#include "IDetailsView.h"
#include "PropertyEditorModule.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "EdGraphUtilities.h"
#include "EdGraphNode_Comment.h"
#include "Layout/SlateRect.h"
#include "SKataFindInGraph.h"
#include "KataGraph.h"
#include "KataAliasNode.h"
#include "KataEntryNode.h"
#include "KataSubGraphPortNode.h"
#include "KataSubGraphNode.h"
#include "KataEmbeddedSubGraphEditor.h"
#include "KataGraphBuildContext.h"
#include "Editor.h"
#include "Framework/Application/SlateApplication.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectHash.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "KataEdGraph.h"
#include "KataEdNode.h"
#include "KataEdNodeEdge.h"
#include "AutoLayout/KataGraphTreeLayoutStrategy.h"
#include "AutoLayout/KataGraphForceDirectedLayoutStrategy.h"

#define LOCTEXT_NAMESPACE "AssetEditor_KataGraph"

const FName KataGraphEditorAppName = FName(TEXT("KataGraphEditorApp"));

struct FKataGraphAssetEditorTabs
{
	// Tab identifiers
	static const FName KataGraphDetailsID;
	static const FName SelectionDetailsID;
	static const FName ViewportID;
	static const FName KataGraphEditorSettingsID;
	static const FName SearchID;
};

//////////////////////////////////////////////////////////////////////////

const FName FKataGraphAssetEditorTabs::KataGraphDetailsID(TEXT("KataGraphProperty"));
const FName FKataGraphAssetEditorTabs::SelectionDetailsID(TEXT("KataGraphSelectionDetails"));
const FName FKataGraphAssetEditorTabs::ViewportID(TEXT("Viewport"));
const FName FKataGraphAssetEditorTabs::KataGraphEditorSettingsID(TEXT("KataGraphEditorSettings"));
const FName FKataGraphAssetEditorTabs::SearchID(TEXT("KataGraphSearch"));

//////////////////////////////////////////////////////////////////////////

FKataGraphAssetEditor::FKataGraphAssetEditor()
{
	EditingGraph = nullptr;
    RootGraphAsset = nullptr;

	KataGraphEditorSettings = NewObject<UKataGraphEditorSettings>(UKataGraphEditorSettings::StaticClass());
}

FKataGraphAssetEditor::~FKataGraphAssetEditor()
{
    FTSTicker::RemoveTicker(GraphNavigationTicker);
    FTSTicker::RemoveTicker(DependencyStatusTicker);
    if (GEditor != nullptr)
    {
        GEditor->UnregisterForUndo(this);
    }
}

void FKataGraphAssetEditor::InitKataGraphEditor(const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, UKataGraphBase* Graph)
{
	EditingGraph = Graph;
    RootGraphAsset = Graph;
    CreateEdGraph(Graph);

    RememberEmbeddedGraphs();

	FGenericCommands::Register();
	FGraphEditorCommands::Register();
	FKataGraphEditorCommands::Register();

	if (!ToolbarBuilder.IsValid())
	{
		ToolbarBuilder = MakeShareable(new FKataGraphEditorToolbar(SharedThis(this)));
	}

	BindCommands();

	CreateInternalWidgets();

    // 이전 원본 보존 정책으로 남은 후보도 동일한 저작 참조 기준으로 정리한다.
    {
        const FScopedTransaction Transaction(LOCTEXT("RemoveUnusedSubGraphs", "Remove Unused Embedded SubGraphs"));
        RemoveUnusedEmbeddedSubGraphs();
    }

	TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);

	ToolbarBuilder->AddKataGraphToolbar(ToolbarExtender);

	// Layout
	const TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout("Standalone_KataGraphEditor_Layout_v5")
		->AddArea
		(
			FTabManager::NewPrimaryArea()->SetOrientation(Orient_Vertical)
#if ENGINE_MAJOR_VERSION < 5
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.1f)
				->AddTab(GetToolbarTabId(), ETabState::OpenedTab)->SetHideTabWell(true)
			)
#endif // #if ENGINE_MAJOR_VERSION < 5
			->Split
			(
				FTabManager::NewSplitter()->SetOrientation(Orient_Horizontal)->SetSizeCoefficient(0.9f)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.65f)
					->AddTab(FKataGraphAssetEditorTabs::ViewportID, ETabState::OpenedTab)->SetHideTabWell(true)
				)
				->Split
				(
					FTabManager::NewSplitter()->SetOrientation(Orient_Vertical)
					->Split
					(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.7f)
						->AddTab(FKataGraphAssetEditorTabs::KataGraphDetailsID, ETabState::OpenedTab)
						->AddTab(FKataGraphAssetEditorTabs::SelectionDetailsID, ETabState::OpenedTab)
						->SetForegroundTab(FKataGraphAssetEditorTabs::SelectionDetailsID)
					)
					->Split
					(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.3f)
						->AddTab(FKataGraphAssetEditorTabs::KataGraphEditorSettingsID, ETabState::OpenedTab)
						->AddTab(FKataGraphAssetEditorTabs::SearchID, ETabState::OpenedTab)
                        ->SetForegroundTab(FKataGraphAssetEditorTabs::KataGraphEditorSettingsID)
					)
				)
			)
		);

	const bool bCreateDefaultStandaloneMenu = true;
	const bool bCreateDefaultToolbar = true;
	FAssetEditorToolkit::InitAssetEditor(Mode, InitToolkitHost, KataGraphEditorAppName, StandaloneDefaultLayout, bCreateDefaultStandaloneMenu, bCreateDefaultToolbar, RootGraphAsset, false);

    if (GEditor != nullptr)
    {
        GEditor->RegisterForUndo(this);
    }

    // 재개봉은 메모리 사본만 갱신한다. 마지막 저장 세대와 의존 기록은 저장 때만 변경한다.
    FKataGraphBuildContext::Rebuild(RootGraphAsset);
    RefreshDependencyStatus(0.0f);
    DependencyStatusTicker = FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateSP(this, &FKataGraphAssetEditor::RefreshDependencyStatus), 1.0f);
	RegenerateMenusAndToolbars();
}

void FKataGraphAssetEditor::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("WorkspaceMenu_KataGraphEditor", "Kata Graph Editor"));
	auto WorkspaceMenuCategoryRef = WorkspaceMenuCategory.ToSharedRef();

	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	InTabManager->RegisterTabSpawner(FKataGraphAssetEditorTabs::ViewportID, FOnSpawnTab::CreateSP(this, &FKataGraphAssetEditor::SpawnTab_Viewport))
		.SetDisplayName(LOCTEXT("GraphCanvasTab", "Viewport"))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "GraphEditor.EventGraph_16x"));

	InTabManager->RegisterTabSpawner(FKataGraphAssetEditorTabs::KataGraphDetailsID, FOnSpawnTab::CreateSP(this, &FKataGraphAssetEditor::SpawnTab_GraphDetails))
		.SetDisplayName(LOCTEXT("GraphDetailsTab", "Kata Graph Details"))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));

	InTabManager->RegisterTabSpawner(FKataGraphAssetEditorTabs::SelectionDetailsID, FOnSpawnTab::CreateSP(this, &FKataGraphAssetEditor::SpawnTab_SelectionDetails))
		.SetDisplayName(LOCTEXT("SelectionDetailsTab", "Selection Details"))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));

	InTabManager->RegisterTabSpawner(FKataGraphAssetEditorTabs::KataGraphEditorSettingsID, FOnSpawnTab::CreateSP(this, &FKataGraphAssetEditor::SpawnTab_EditorSettings))
		.SetDisplayName(LOCTEXT("EditorSettingsTab", "Kata Graph Editor Settings"))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));

	InTabManager->RegisterTabSpawner(FKataGraphAssetEditorTabs::SearchID, FOnSpawnTab::CreateSP(this, &FKataGraphAssetEditor::SpawnTab_Search))
		.SetDisplayName(LOCTEXT("SearchTab", "Find in Graph"))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "Kismet.Tabs.FindResults"));

}

void FKataGraphAssetEditor::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);

	InTabManager->UnregisterTabSpawner(FKataGraphAssetEditorTabs::ViewportID);
	InTabManager->UnregisterTabSpawner(FKataGraphAssetEditorTabs::KataGraphDetailsID);
	InTabManager->UnregisterTabSpawner(FKataGraphAssetEditorTabs::SelectionDetailsID);
	InTabManager->UnregisterTabSpawner(FKataGraphAssetEditorTabs::KataGraphEditorSettingsID);
	InTabManager->UnregisterTabSpawner(FKataGraphAssetEditorTabs::SearchID);
}

FName FKataGraphAssetEditor::GetToolkitFName() const
{
	return FName("FKataGraphEditor");
}

FText FKataGraphAssetEditor::GetBaseToolkitName() const
{
	return LOCTEXT("KataGraphEditorAppLabel", "Kata Graph Editor");
}

FText FKataGraphAssetEditor::GetToolkitName() const
{
	const bool bDirtyState = RootGraphAsset->GetOutermost()->IsDirty();

	FFormatNamedArguments Args;
	Args.Add(TEXT("KataGraphName"), FText::FromString(RootGraphAsset->GetName()));
	Args.Add(TEXT("DirtyState"), bDirtyState ? FText::FromString(TEXT("*")) : FText::GetEmpty());
	return FText::Format(LOCTEXT("KataGraphEditorToolkitName", "{KataGraphName}{DirtyState}"), Args);
}

FText FKataGraphAssetEditor::GetToolkitToolTipText() const
{
	return FAssetEditorToolkit::GetToolTipTextForObject(RootGraphAsset);
}

FLinearColor FKataGraphAssetEditor::GetWorldCentricTabColorScale() const
{
	return FLinearColor::White;
}

FString FKataGraphAssetEditor::GetWorldCentricTabPrefix() const
{
	return TEXT("KataGraphEditor");
}

FString FKataGraphAssetEditor::GetDocumentationLink() const
{
	return TEXT("");
}

void FKataGraphAssetEditor::SaveAsset_Execute()
{
    // 일반 저장과 Save All을 같은 PreSave 진입점으로 모아 내장 전체를 중복 재구성하지 않는다.
	FAssetEditorToolkit::SaveAsset_Execute();
}

void FKataGraphAssetEditor::AddReferencedObjects(FReferenceCollector& Collector)
{
    Collector.AddReferencedObject(RootGraphAsset);
    Collector.AddReferencedObject(PendingGraph);
	Collector.AddReferencedObject(EditingGraph);
    Collector.AddReferencedObject(KataGraphEditorSettings);
}

UKataGraphEditorSettings* FKataGraphAssetEditor::GetSettings() const
{
	return KataGraphEditorSettings;
}

bool FKataGraphAssetEditor::OnRequestClose(EAssetEditorCloseReason InCloseReason)
{
    if (!FAssetEditorToolkit::OnRequestClose(InCloseReason))
    {
        return false;
    }
    if (const TSharedPtr<FTabManager> Manager = GetTabManager())
    {
        // 크기 조절의 지연 저장을 기다리지 않고, 도킹 영역이 해체되기 전에 마지막 비율을 기록한다.
        Manager->SavePersistentLayout();
    }
    return true;
}

UKataGraph* FKataGraphAssetEditor::GetRootKataGraph() const
{
    return Cast<UKataGraph>(RootGraphAsset);
}

void FKataGraphAssetEditor::OpenGraph(UKataGraphBase* Graph)
{
    const UKataGraph* Root = GetRootKataGraph();
    if (Graph == nullptr || (Graph != RootGraphAsset && (Root == nullptr || !Root->ContainsGraph(Cast<UKataGraph>(Graph)))))
    {
        return;
    }

    // 더블클릭을 처리 중인 그래프 위젯을 같은 콜백 안에서 해제하지 않는다.
    PendingGraph = Graph;
    if (!GraphNavigationTicker.IsValid())
    {
        GraphNavigationTicker = FTSTicker::GetCoreTicker().AddTicker(
            FTickerDelegate::CreateSP(this, &FKataGraphAssetEditor::ApplyPendingGraphNavigation));
    }
}

bool FKataGraphAssetEditor::ApplyPendingGraphNavigation(float DeltaTime)
{
    GraphNavigationTicker.Reset();
    UKataGraphBase* Graph = PendingGraph;
    PendingGraph = nullptr;
    OpenGraphNow(Graph);
    return false;
}

void FKataGraphAssetEditor::OpenGraphNow(UKataGraphBase* Graph)
{
    const UKataGraph* Root = GetRootKataGraph();
    if (Graph == nullptr || (Graph != RootGraphAsset && (Root == nullptr || !Root->ContainsGraph(Cast<UKataGraph>(Graph)))))
    {
        return;
    }
    if (Graph == EditingGraph)
    {
        return;
    }

    if (ViewportWidget.IsValid())
    {
        FKataGraphViewState& State = GraphViewStates.FindOrAdd(EditingGraph.Get());
        ViewportWidget->GetViewLocation(State.Location, State.Zoom);
        ViewportWidget->ClearSelectionSet();
    }

    EditingGraph = Graph;
    CreateEdGraph(Graph);
    SelectionDetailsWidget->SetObject(nullptr);
    GraphDetailsWidget->SetObject(Graph);
    ViewportWidget = CreateViewportWidget();
    ViewportContainer->SetContent(ViewportWidget.ToSharedRef());

    if (const FKataGraphViewState* State = GraphViewStates.Find(Graph))
    {
        ViewportWidget->SetViewLocation(State->Location, State->Zoom);
    }

    // 검색 위젯도 교체해 이전 그래프의 결과와 노드 참조를 남기지 않는다.
    SearchWidget = SNew(SKataFindInGraph, SharedThis(this), Graph->EdGraph);
    SearchContainer->SetContent(SearchWidget.ToSharedRef());
    FSlateApplication::Get().SetKeyboardFocus(ViewportWidget.ToSharedRef(), EFocusCause::SetDirectly);
}

void FKataGraphAssetEditor::RefreshSubGraphUI()
{
    if (RootGraphAsset != nullptr && RootGraphAsset->EdGraph != nullptr)
    {
        RootGraphAsset->EdGraph->GetSchema()->ForceVisualizationCacheClear();
        RootGraphAsset->EdGraph->NotifyGraphChanged();
    }
    if (ViewportWidget.IsValid())
    {
        ViewportWidget->NotifyGraphChanged();
    }
}

void FKataGraphAssetEditor::RememberEmbeddedGraphs()
{
    if (UKataGraph* Root = GetRootKataGraph())
    {
        TArray<UObject*> Objects;
        GetObjectsWithOuter(Root, Objects, EGetObjectsFlags::IncludeNestedObjects);
        for (UObject* Object : Objects)
        {
            UKataGraph* Graph = Cast<UKataGraph>(Object);
            UKataGraph* Owner = Graph != nullptr ? Cast<UKataGraph>(Graph->GetOuter()) : nullptr;
            if (Owner != nullptr && Root->ContainsGraph(Owner))
            {
                KnownEmbeddedGraphs.Add(Graph, Owner);
            }
        }
    }
}

UKataGraphBase* FKataGraphAssetEditor::GetValidGraphAncestor(UKataGraphBase* Graph) const
{
    const UKataGraph* Root = GetRootKataGraph();
    TSet<UKataGraphBase*> Seen;
    while (Root != nullptr && Graph != nullptr && !Seen.Contains(Graph))
    {
        if (Root->ContainsGraph(Cast<UKataGraph>(Graph)))
        {
            return Graph;
        }
        Seen.Add(Graph);
        const TWeakObjectPtr<UKataGraph>* Owner = KnownEmbeddedGraphs.Find(Cast<UKataGraph>(Graph));
        Graph = Owner != nullptr ? Owner->Get() : nullptr;
    }
    return RootGraphAsset;
}

void FKataGraphAssetEditor::RemoveUnusedEmbeddedSubGraphs()
{
    UKataGraph* Root = GetRootKataGraph();
    if (Root == nullptr)
    {
        return;
    }
    RememberEmbeddedGraphs();
    TArray<UKataGraph*> Pages;
    Pages.Add(Root);
    bool bChanged = false;
    for (int32 Index = 0; Index < Pages.Num(); ++Index)
    {
        UKataGraph* Page = Pages[Index];
        bChanged |= KataEmbeddedSubGraphEditor::RemoveUnused(Page);
        for (UKataGraph* Child : Page->EmbeddedSubGraphs)
        {
            if (Page->OwnsEmbeddedSubGraph(Child))
            {
                Pages.AddUnique(Child);
            }
        }
    }
    if (!bChanged)
    {
        return;
    }
    if (!Root->ContainsGraph(Cast<UKataGraph>(EditingGraph)))
    {
        OpenGraph(GetValidGraphAncestor(EditingGraph));
    }
    else if (PendingGraph != nullptr && !Root->ContainsGraph(Cast<UKataGraph>(PendingGraph)))
    {
        OpenGraph(GetValidGraphAncestor(PendingGraph));
    }
    RefreshSubGraphUI();
}

bool FKataGraphAssetEditor::RefreshDependencyStatus(float DeltaTime)
{
    if (RootGraphAsset == nullptr)
    {
        return true;
    }
    // 경고 표시는 저장 가능한 새 사본과 구분한다. 원본 미저장·참조 오류로 부모를 매번 dirty로 만들지 않는다.
    bool bNeedsSave = false;
    DependencyStatus = FKataGraphBuildContext::GetDependencyStatus(RootGraphAsset, bNeedsSave);
    if (bNeedsSave && !RootGraphAsset->GetOutermost()->IsDirty())
    {
        RootGraphAsset->MarkPackageDirty();
    }
    return true;
}

void FKataGraphAssetEditor::PostUndo(bool bSuccess)
{
    if (!bSuccess || RootGraphAsset == nullptr)
    {
        return;
    }

    if (UKataGraph* Root = GetRootKataGraph())
    {
        RememberEmbeddedGraphs();
        // 목록과 소유 페이지의 Undo 결과에 따라 각 객체를 복구한다. 하위 트리는 부모와 함께 살아남는다.
        for (const TPair<TWeakObjectPtr<UKataGraph>, TWeakObjectPtr<UKataGraph>>& Pair : KnownEmbeddedGraphs)
        {
            UKataGraph* Graph = Pair.Key.Get();
            UKataGraph* Owner = Pair.Value.Get();
            if (Graph == nullptr || Owner == nullptr)
            {
                continue;
            }
            const bool bListed = Owner->EmbeddedSubGraphs.Contains(Graph);
            if (bListed && Graph->GetOuter() == GetTransientPackage())
            {
                Graph->Rename(nullptr, Owner, REN_DontCreateRedirectors | REN_DoNotDirty);
            }
            else if (!bListed && Graph->GetOuter() == Owner)
            {
                Graph->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_DoNotDirty);
            }
        }
        if (!Root->ContainsGraph(Cast<UKataGraph>(EditingGraph)))
        {
            OpenGraph(GetValidGraphAncestor(EditingGraph));
        }
        if (PendingGraph != nullptr && !Root->ContainsGraph(Cast<UKataGraph>(PendingGraph)))
        {
            OpenGraph(GetValidGraphAncestor(PendingGraph));
        }
    }

    ViewportWidget->ClearSelectionSet();
    SelectionDetailsWidget->SetObject(nullptr);
    GraphDetailsWidget->SetObject(EditingGraph, true);
    RefreshSubGraphUI();
    // Undo로 삭제·복구된 노드를 이전 검색 결과가 계속 가리키지 않도록 현재 페이지에서 다시 검색한다.
    SearchWidget = SNew(SKataFindInGraph, SharedThis(this), EditingGraph->EdGraph);
    SearchContainer->SetContent(SearchWidget.ToSharedRef());
}

FReply FKataGraphAssetEditor::NavigateToRoot()
{
    OpenGraph(RootGraphAsset);
    return FReply::Handled();
}

void FKataGraphAssetEditor::NavigateBack()
{
    if (EditingGraph != RootGraphAsset)
    {
        // 입력 처리 중 위젯을 교체하지 않도록 기존 페이지 전환 예약 경로를 사용한다.
        UKataGraphBase* Parent = EditingGraph->GetTypedOuter<UKataGraphBase>();
        OpenGraph(GetValidGraphAncestor(Parent));
    }
}

FText FKataGraphAssetEditor::GetCurrentGraphName() const
{
    TArray<FString> Names;
    const UKataGraph* Graph = Cast<UKataGraph>(EditingGraph);
    while (Graph != nullptr && Graph != RootGraphAsset)
    {
        Names.Insert(Graph->GetGraphDisplayName().ToString(), 0);
        Graph = Cast<UKataGraph>(Graph->GetOuter());
    }
    return FText::FromString(FString::Join(Names, TEXT(" / ")));
}

TSharedRef<SDockTab> FKataGraphAssetEditor::SpawnTab_Viewport(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == FKataGraphAssetEditorTabs::ViewportID);

	TSharedRef<SDockTab> SpawnedTab = SNew(SDockTab)
		.Label(LOCTEXT("ViewportTab_Title", "Viewport"));

    SpawnedTab->SetContent(
        SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight().Padding(4.0f)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth()
            [
                SNew(SButton)
                .Text_Lambda([this]() { return FText::FromString(RootGraphAsset->GetName()); })
                .OnClicked(this, &FKataGraphAssetEditor::NavigateToRoot)
            ]
            + SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(8.0f, 0.0f)
            [
                SNew(STextBlock)
                .Text(this, &FKataGraphAssetEditor::GetCurrentGraphName)
                .Visibility_Lambda([this]() { return EditingGraph != RootGraphAsset ? EVisibility::Visible : EVisibility::Collapsed; })
            ]
        ]
        + SVerticalBox::Slot().AutoHeight().Padding(4.0f)
        [
            SNew(STextBlock)
            .Text_Lambda([this]() { return DependencyStatus; })
            .AutoWrapText(true)
            .ColorAndOpacity(FLinearColor(1.0f, 0.65f, 0.15f))
            .Visibility_Lambda([this]() { return DependencyStatus.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible; })
        ]
        + SVerticalBox::Slot().FillHeight(1.0f)
        [
            ViewportContainer.ToSharedRef()
        ]);

	return SpawnedTab;
}

TSharedRef<SDockTab> FKataGraphAssetEditor::SpawnTab_GraphDetails(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == FKataGraphAssetEditorTabs::KataGraphDetailsID);

	return SNew(SDockTab)
#if ENGINE_MAJOR_VERSION < 5
		.Icon(FAppStyle::GetBrush("LevelEditor.Tabs.Details"))
#endif // #if ENGINE_MAJOR_VERSION < 5
		.Label(LOCTEXT("GraphDetails_Title", "Kata Graph Details"))
		[
			GraphDetailsWidget.ToSharedRef()
		];
}

TSharedRef<SDockTab> FKataGraphAssetEditor::SpawnTab_SelectionDetails(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == FKataGraphAssetEditorTabs::SelectionDetailsID);
	return SNew(SDockTab)
		.Label(LOCTEXT("SelectionDetails_Title", "Selection Details"))
		[
			SelectionDetailsWidget.ToSharedRef()
		];
}

TSharedRef<SDockTab> FKataGraphAssetEditor::SpawnTab_EditorSettings(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == FKataGraphAssetEditorTabs::KataGraphEditorSettingsID);

	return SNew(SDockTab)
#if ENGINE_MAJOR_VERSION < 5
		.Icon(FAppStyle::GetBrush("LevelEditor.Tabs.Details"))
#endif // #if ENGINE_MAJOR_VERSION < 5
		.Label(LOCTEXT("EditorSettings_Title", "Kata Graph Editor Settings"))
		[
			EditorSettingsWidget.ToSharedRef()
		];
}

TSharedRef<SDockTab> FKataGraphAssetEditor::SpawnTab_Search(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == FKataGraphAssetEditorTabs::SearchID);

	return SNew(SDockTab)
		.Label(LOCTEXT("Search_Title", "Find in Graph"))
		[
			SearchContainer.ToSharedRef()
		];
}

void FKataGraphAssetEditor::CreateInternalWidgets()
{
	ViewportWidget = CreateViewportWidget();
    ViewportContainer = SNew(SBox)[ViewportWidget.ToSharedRef()];

	FDetailsViewArgs Args;
	Args.bHideSelectionTip = true;
	Args.NotifyHook = this;

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	GraphDetailsWidget = PropertyModule.CreateDetailView(Args);
	GraphDetailsWidget->SetObject(EditingGraph);
	GraphDetailsWidget->OnFinishedChangingProperties().AddSP(this, &FKataGraphAssetEditor::OnFinishedChangingProperties);

	SelectionDetailsWidget = PropertyModule.CreateDetailView(Args);
	SelectionDetailsWidget->OnFinishedChangingProperties().AddSP(this, &FKataGraphAssetEditor::OnFinishedChangingProperties);

	EditorSettingsWidget = PropertyModule.CreateDetailView(Args);
	EditorSettingsWidget->SetObject(KataGraphEditorSettings);

	SearchWidget = SNew(SKataFindInGraph, SharedThis(this), EditingGraph->EdGraph);
    SearchContainer = SNew(SBox)[SearchWidget.ToSharedRef()];
}

TSharedRef<SGraphEditor> FKataGraphAssetEditor::CreateViewportWidget()
{
	FGraphAppearanceInfo AppearanceInfo;
	AppearanceInfo.CornerText = LOCTEXT("AppearanceCornerText_KataGraph", "Kata Graph");

	CreateCommandList();

	SGraphEditor::FGraphEditorEvents InEvents;
	InEvents.OnSelectionChanged = SGraphEditor::FOnSelectionChanged::CreateSP(this, &FKataGraphAssetEditor::OnSelectedNodesChanged);
	InEvents.OnNodeDoubleClicked = FSingleNodeEvent::CreateSP(this, &FKataGraphAssetEditor::OnNodeDoubleClicked);

	return SNew(SGraphEditor)
		.AdditionalCommands(GraphEditorCommands)
		.IsEditable(true)
		.Appearance(AppearanceInfo)
		.GraphToEdit(EditingGraph->EdGraph)
		.GraphEvents(InEvents)
        .OnNavigateHistoryBack(this, &FKataGraphAssetEditor::NavigateBack)
		.AutoExpandActionMenu(true)
		.ShowGraphStateOverlay(false);
}

void FKataGraphAssetEditor::BindCommands()
{
	ToolkitCommands->MapAction(FKataGraphEditorCommands::Get().GraphSettings,
		FExecuteAction::CreateSP(this, &FKataGraphAssetEditor::GraphSettings),
		FCanExecuteAction::CreateSP(this, &FKataGraphAssetEditor::CanGraphSettings)
	);

	ToolkitCommands->MapAction(FKataGraphEditorCommands::Get().AutoArrange,
		FExecuteAction::CreateSP(this, &FKataGraphAssetEditor::AutoArrange),
		FCanExecuteAction::CreateSP(this, &FKataGraphAssetEditor::CanAutoArrange)
	);
	ToolkitCommands->MapAction(FKataGraphEditorCommands::Get().FindInGraph,
		FExecuteAction::CreateSP(this, &FKataGraphAssetEditor::FindInGraph)
	);
}

void FKataGraphAssetEditor::CreateEdGraph(UKataGraphBase* Graph)
{
    if (Graph != nullptr && Graph->EdGraph == nullptr)
	{
        Graph->Modify();
        Graph->EdGraph = CastChecked<UKataEdGraph>(FBlueprintEditorUtils::CreateNewGraph(Graph, NAME_None, UKataEdGraph::StaticClass(), UKataGraphSchema::StaticClass()));
        Graph->EdGraph->SetFlags(RF_Transactional);
        Graph->EdGraph->bAllowDeletion = false;

		// Give the schema a chance to fill out any required nodes (like the results node)
        const UEdGraphSchema* Schema = Graph->EdGraph->GetSchema();
        Schema->CreateDefaultNodesForGraph(*Graph->EdGraph);
	}
}

void FKataGraphAssetEditor::CreateCommandList()
{
	if (GraphEditorCommands.IsValid())
	{
		return;
	}

	GraphEditorCommands = MakeShareable(new FUICommandList);

	// Can't use CreateSP here because derived editor are already implementing TSharedFromThis<FAssetEditorToolkit>
	// however it should be safe, since commands are being used only within this editor
	// if it ever crashes, this function will have to go away and be reimplemented in each derived class

	GraphEditorCommands->MapAction(FKataGraphEditorCommands::Get().GraphSettings,
		FExecuteAction::CreateRaw(this, &FKataGraphAssetEditor::GraphSettings),
		FCanExecuteAction::CreateRaw(this, &FKataGraphAssetEditor::CanGraphSettings));

	GraphEditorCommands->MapAction(FKataGraphEditorCommands::Get().AutoArrange,
		FExecuteAction::CreateRaw(this, &FKataGraphAssetEditor::AutoArrange),
		FCanExecuteAction::CreateRaw(this, &FKataGraphAssetEditor::CanAutoArrange));

	GraphEditorCommands->MapAction(FGenericCommands::Get().SelectAll,
		FExecuteAction::CreateRaw(this, &FKataGraphAssetEditor::SelectAllNodes),
		FCanExecuteAction::CreateRaw(this, &FKataGraphAssetEditor::CanSelectAllNodes)
	);

	GraphEditorCommands->MapAction(FGenericCommands::Get().Delete,
		FExecuteAction::CreateRaw(this, &FKataGraphAssetEditor::DeleteSelectedNodes),
		FCanExecuteAction::CreateRaw(this, &FKataGraphAssetEditor::CanDeleteNodes)
	);

	GraphEditorCommands->MapAction(FGenericCommands::Get().Copy,
		FExecuteAction::CreateRaw(this, &FKataGraphAssetEditor::CopySelectedNodes),
		FCanExecuteAction::CreateRaw(this, &FKataGraphAssetEditor::CanCopyNodes)
	);

	GraphEditorCommands->MapAction(FGenericCommands::Get().Cut,
		FExecuteAction::CreateRaw(this, &FKataGraphAssetEditor::CutSelectedNodes),
		FCanExecuteAction::CreateRaw(this, &FKataGraphAssetEditor::CanCutNodes)
	);

	GraphEditorCommands->MapAction(FGenericCommands::Get().Paste,
		FExecuteAction::CreateRaw(this, &FKataGraphAssetEditor::PasteNodes),
		FCanExecuteAction::CreateRaw(this, &FKataGraphAssetEditor::CanPasteNodes)
	);

	GraphEditorCommands->MapAction(FGenericCommands::Get().Duplicate,
		FExecuteAction::CreateRaw(this, &FKataGraphAssetEditor::DuplicateNodes),
		FCanExecuteAction::CreateRaw(this, &FKataGraphAssetEditor::CanDuplicateNodes)
	);

	GraphEditorCommands->MapAction(FGenericCommands::Get().Rename,
		FExecuteAction::CreateSP(this, &FKataGraphAssetEditor::OnRenameNode),
		FCanExecuteAction::CreateSP(this, &FKataGraphAssetEditor::CanRenameNodes)
	);

	GraphEditorCommands->MapAction(FGraphEditorCommands::Get().CreateComment,
		FExecuteAction::CreateRaw(this, &FKataGraphAssetEditor::CreateComment),
		FCanExecuteAction::CreateRaw(this, &FKataGraphAssetEditor::CanCreateComment));

	GraphEditorCommands->MapAction(FKataGraphEditorCommands::Get().FindInGraph,
		FExecuteAction::CreateSP(this, &FKataGraphAssetEditor::FindInGraph));
}

void FKataGraphAssetEditor::CreateComment()
{
	// 선택한 노드를 감쌀 때 바깥으로 남길 여백. 블루프린트 편집기와 같은 값이다.
	constexpr float SelectionPadding = 50.0f;

	// 감쌀 선택이 없을 때 사용할 기본 크기.
	constexpr int32 DefaultWidth = 400;
	constexpr int32 DefaultHeight = 200;

	TSharedPtr<SGraphEditor> GraphEditor = GetCurrGraphEditor();
	if (!GraphEditor.IsValid() || !EditingGraph || !EditingGraph->EdGraph)
	{
		return;
	}

	// 배치 정보는 노드를 추가하기 전에 구한다.
	// 선택 영역은 패널의 노드 위젯 위치로 계산하는데, 그래프를 수정하면 패널이 위젯을
	// 다시 만들 수 있어 추가한 뒤에 물으면 빈 결과가 나온다. 엔진의 코멘트 생성도 같은 순서다.
	FSlateRect SelectionBounds;
	const bool bWrapSelection = GraphEditor->GetBoundsForSelectedNodes(SelectionBounds, SelectionPadding);

	// 붙여넣기 위치는 그래프 패널 위에서 마우스가 움직일 때마다 갱신되므로 커서를 따라간다.
	const FVector2f SpawnLocation = GraphEditor->GetPasteLocation2f();

	const FScopedTransaction Transaction(LOCTEXT("CreateComment", "Create Kata Graph Comment"));
	EditingGraph->EdGraph->Modify();
	UEdGraphNode_Comment* Comment = NewObject<UEdGraphNode_Comment>(EditingGraph->EdGraph);
	EditingGraph->EdGraph->AddNode(Comment, true, true);
	Comment->CreateNewGuid();
	Comment->PostPlacedNewNode();

	// 선택한 노드가 있으면 그 노드들을 감싼다. 블루프린트 편집기와 같은 동작이다.
	if (bWrapSelection)
	{
		Comment->SetBounds(SelectionBounds);
		return;
	}

	// 선택이 없으면 마우스 위치에 기본 크기로 만든다.
	Comment->NodePosX = FMath::RoundToInt(SpawnLocation.X);
	Comment->NodePosY = FMath::RoundToInt(SpawnLocation.Y);
	Comment->NodeWidth = DefaultWidth;
	Comment->NodeHeight = DefaultHeight;
}

bool FKataGraphAssetEditor::CanCreateComment() const
{
	return EditingGraph != nullptr && EditingGraph->EdGraph != nullptr;
}

TSharedPtr<SGraphEditor> FKataGraphAssetEditor::GetCurrGraphEditor() const
{
	return ViewportWidget;
}

FGraphPanelSelectionSet FKataGraphAssetEditor::GetSelectedNodes() const
{
	FGraphPanelSelectionSet CurrentSelection;
	TSharedPtr<SGraphEditor> FocusedGraphEd = GetCurrGraphEditor();
	if (FocusedGraphEd.IsValid())
	{
		CurrentSelection = FocusedGraphEd->GetSelectedNodes();
	}

	return CurrentSelection;
}

void FKataGraphAssetEditor::SelectAllNodes()
{
	TSharedPtr<SGraphEditor> CurrentGraphEditor = GetCurrGraphEditor();
	if (CurrentGraphEditor.IsValid())
	{
		CurrentGraphEditor->SelectAllNodes();
	}
}

bool FKataGraphAssetEditor::CanSelectAllNodes()
{
	return true;
}

void FKataGraphAssetEditor::DeleteSelectedNodes()
{
	TSharedPtr<SGraphEditor> CurrentGraphEditor = GetCurrGraphEditor();
	if (!CurrentGraphEditor.IsValid())
	{
		return;
	}

	const FScopedTransaction Transaction(FGenericCommands::Get().Delete->GetDescription());

	CurrentGraphEditor->GetCurrentGraph()->Modify();

	const FGraphPanelSelectionSet SelectedNodes = CurrentGraphEditor->GetSelectedNodes();
	CurrentGraphEditor->ClearSelectionSet();

	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		UEdGraphNode* EdNode = Cast<UEdGraphNode>(*NodeIt);
		if (EdNode == nullptr || !EdNode->CanUserDeleteNode())
			continue;;

		if (UKataEdNode* EdNode_Node = Cast<UKataEdNode>(EdNode))
		{
			EdNode_Node->Modify();

			const UEdGraphSchema* Schema = EdNode_Node->GetSchema();
			if (Schema != nullptr)
			{
				Schema->BreakNodeLinks(*EdNode_Node);
			}

			EdNode_Node->DestroyNode();
		}
		else
		{
			EdNode->Modify();
			EdNode->DestroyNode();
		}
	}
    if (!bPreserveEmbeddedSubGraphsForCut)
    {
        RemoveUnusedEmbeddedSubGraphs();
    }
}

bool FKataGraphAssetEditor::CanDeleteNodes()
{
	// If any of the nodes can be deleted then we should allow deleting
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
		if (Node != nullptr && Node->CanUserDeleteNode())
		{
			return true;
		}
	}

	return false;
}

void FKataGraphAssetEditor::DeleteSelectedDuplicatableNodes()
{
	TSharedPtr<SGraphEditor> CurrentGraphEditor = GetCurrGraphEditor();
	if (!CurrentGraphEditor.IsValid())
	{
		return;
	}

	const FGraphPanelSelectionSet OldSelectedNodes = CurrentGraphEditor->GetSelectedNodes();
	CurrentGraphEditor->ClearSelectionSet();

	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(OldSelectedNodes); SelectedIter; ++SelectedIter)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
		if (Node && Node->CanDuplicateNode())
		{
			CurrentGraphEditor->SetNodeSelection(Node, true);
		}
	}

	// Delete the duplicatable nodes
	DeleteSelectedNodes();

	CurrentGraphEditor->ClearSelectionSet();

	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(OldSelectedNodes); SelectedIter; ++SelectedIter)
	{
		if (UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter))
		{
			CurrentGraphEditor->SetNodeSelection(Node, true);
		}
	}
}

void FKataGraphAssetEditor::CutSelectedNodes()
{
    TSet<UKataGraph*> CopiedSources;
    const FGraphPanelSelectionSet Selection = GetSelectedNodes();
    for (UObject* SelectedObject : Selection)
    {
        const UKataEdNode* EdNode = Cast<UKataEdNode>(SelectedObject);
        const UKataSubGraphNode* Node = EdNode != nullptr ? Cast<UKataSubGraphNode>(EdNode->KataNode) : nullptr;
        if (Node != nullptr && Node->bUseEmbeddedSubGraph)
        {
            if (UKataGraph* Source = Node->GetReferencedSubGraph())
            {
                CopiedSources.Add(Source);
            }
        }
    }
    bool bNeedsOriginalReference = false;
    for (UObject* SelectedObject : Selection)
    {
        const UKataEdNode* EdNode = Cast<UKataEdNode>(SelectedObject);
        const UKataSubGraphPortNode* Port = EdNode != nullptr ? Cast<UKataSubGraphPortNode>(EdNode->KataNode) : nullptr;
        if (Port != nullptr && Port->bUseEmbeddedSubGraph)
        {
            UKataGraph* Source = Port->GetReferencedSubGraph();
            bNeedsOriginalReference |= Source != nullptr && !CopiedSources.Contains(Source);
        }
    }
    // 원본 참조만 복사한 포트가 있을 때만 보존한다. 내부 내용이 담긴 SubGraph는 삭제 규칙을 따른다.
    TGuardValue<bool> PreserveSources(bPreserveEmbeddedSubGraphsForCut, bNeedsOriginalReference);
	CopySelectedNodes();
	DeleteSelectedDuplicatableNodes();
}

bool FKataGraphAssetEditor::CanCutNodes()
{
	return CanCopyNodes() && CanDeleteNodes();
}

void FKataGraphAssetEditor::CopySelectedNodes()
{
	// Export the selected nodes and place the text on the clipboard
	FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();

	FString ExportedText;
    TMap<UObject*, UObject*> OriginalOuters;

    // 엣지 포함 여부는 복제 불가 노드를 제외한 최종 선택으로 판단한다.
    for (FGraphPanelSelectionSet::TIterator It(SelectedNodes); It; ++It)
    {
        const UEdGraphNode* Node = Cast<UEdGraphNode>(*It);
        if (Node == nullptr || !Node->CanDuplicateNode())
        {
            It.RemoveCurrent();
        }
    }

	for (FGraphPanelSelectionSet::TIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
		if (Node == nullptr || !Node->CanDuplicateNode())
		{
			SelectedIter.RemoveCurrent();
			continue;
		}

		if (UKataEdNodeEdge* EdNode_Edge = Cast<UKataEdNodeEdge>(*SelectedIter))
		{
			UKataEdNode* StartNode = EdNode_Edge->GetStartNode();
			UKataEdNode* EndNode = EdNode_Edge->GetEndNode();

			if (!SelectedNodes.Contains(StartNode) || !SelectedNodes.Contains(EndNode))
			{
				SelectedIter.RemoveCurrent();
				continue;
			}
		}

        if (UKataEdNode* EdNode = Cast<UKataEdNode>(Node))
        {
            if (EdNode->KataNode != nullptr)
            {
                OriginalOuters.Add(EdNode->KataNode, EdNode->KataNode->GetOuter());
            }
        }
        else if (UKataEdNodeEdge* EdEdge = Cast<UKataEdNodeEdge>(Node))
        {
            if (EdEdge->KataEdge != nullptr)
            {
                OriginalOuters.Add(EdEdge->KataEdge, EdEdge->KataEdge->GetOuter());
            }
        }
		Node->PrepareForCopying();
	}

    TSet<UKataGraph*> SnapshottedSources;
    for (UObject* SelectedObject : SelectedNodes)
    {
        UKataEdNode* EdNode = Cast<UKataEdNode>(SelectedObject);
        UKataSubGraphPortNode* Port = EdNode != nullptr ? Cast<UKataSubGraphPortNode>(EdNode->KataNode) : nullptr;
        if (Port == nullptr || !Port->bUseEmbeddedSubGraph)
        {
            continue;
        }
        UKataGraph* Source = Port->GetReferencedSubGraph();
        if (Source == nullptr)
        {
            continue;
        }
        EdNode->ClipboardSubGraphSource = Source->GetPathName();
        if (Port->IsA<UKataSubGraphNode>() && !SnapshottedSources.Contains(Source))
        {
            EdNode->ClipboardSubGraph = KataEmbeddedSubGraphEditor::CopyForClipboard(Source, EdNode);
            if (EdNode->ClipboardSubGraph != nullptr)
            {
                SnapshottedSources.Add(Source);
            }
        }
    }

	FEdGraphUtilities::ExportNodesToText(SelectedNodes, ExportedText);
    for (UObject* SelectedObject : SelectedNodes)
    {
        if (UKataEdNode* EdNode = Cast<UKataEdNode>(SelectedObject))
        {
            if (UKataGraph* Snapshot = EdNode->ClipboardSubGraph)
            {
                Snapshot->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_DoNotDirty);
            }
            EdNode->ClipboardSubGraph = nullptr;
            EdNode->ClipboardSubGraphSource.Reset();
        }
    }
    // 텍스트 내보내기에만 필요한 임시 소유 변경이 실행 데이터와 다음 Undo에 남지 않게 한다.
    for (const TPair<UObject*, UObject*>& Pair : OriginalOuters)
    {
        Pair.Key->Rename(nullptr, Pair.Value, REN_DontCreateRedirectors | REN_DoNotDirty);
    }
	FPlatformApplicationMisc::ClipboardCopy(*ExportedText);
}

bool FKataGraphAssetEditor::CanCopyNodes()
{
	// If any of the nodes can be duplicated then we should allow copying
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
		if (Node && Node->CanDuplicateNode())
		{
			return true;
		}
	}

	return false;
}

void FKataGraphAssetEditor::PasteNodes()
{
	TSharedPtr<SGraphEditor> CurrentGraphEditor = GetCurrGraphEditor();
	if (CurrentGraphEditor.IsValid())
	{
		// Slate 좌표는 float이므로 폐기 예정인 FVector2D 버전 대신 FVector2f 버전을 쓴다.
		PasteNodesHere(CurrentGraphEditor->GetPasteLocation2f());
	}
}

void FKataGraphAssetEditor::PasteNodesHere(const FVector2f& Location)
{
	// Find the graph editor with focus
	TSharedPtr<SGraphEditor> CurrentGraphEditor = GetCurrGraphEditor();
	if (!CurrentGraphEditor.IsValid())
	{
		return;
	}
	// Select the newly pasted stuff
	UEdGraph* EdGraph = CurrentGraphEditor->GetCurrentGraph();

	{
		FScopedTransaction Transaction(FGenericCommands::Get().Paste->GetDescription());
		EdGraph->Modify();

		// Clear the selection set (newly pasted stuff will be selected)
		CurrentGraphEditor->ClearSelectionSet();

		// Grab the text to paste from the clipboard.
		FString TextToImport;
		FPlatformApplicationMisc::ClipboardPaste(TextToImport);

		// Import the nodes
		TSet<UEdGraphNode*> PastedNodes;
		FEdGraphUtilities::ImportNodesFromText(EdGraph, TextToImport, PastedNodes);
        if (PastedNodes.IsEmpty())
        {
            return;
        }

        UKataGraphBase* TargetGraph = CastChecked<UKataGraphBase>(EdGraph->GetOuter());
        UKataGraph* TargetOwner = Cast<UKataGraph>(TargetGraph);
        const bool bCanOwnEmbedded = TargetOwner != nullptr && TargetOwner->GetRootGraph() == GetRootKataGraph();
        bool bContainsSubGraphNode = false;
        for (const UEdGraphNode* Node : PastedNodes)
        {
            const UKataEdNode* EdNode = Cast<UKataEdNode>(Node);
            bContainsSubGraphNode |= EdNode != nullptr && EdNode->KataNode != nullptr && EdNode->KataNode->IsA<UKataSubGraphNode>();
        }
        if (!bCanOwnEmbedded && bContainsSubGraphNode)
        {
            // 소유 트리 밖의 페이지에는 내장을 편입하지 않는다. 혼합 선택은 함께 되돌린다.
            for (UEdGraphNode* Node : PastedNodes)
            {
                Node->DestroyNode();
                Node->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_DoNotDirty);
            }
            Transaction.Cancel();
            CurrentGraphEditor->NotifyGraphChanged();
            LOG_WARNING(TEXT("SubGraph nodes require a page in the current graph asset ownership tree."));
            return;
        }
        TMap<FString, UKataGraph*> PastedSources;
        for (UEdGraphNode* Node : PastedNodes)
        {
            UKataEdNode* EdNode = Cast<UKataEdNode>(Node);
            if (EdNode != nullptr && EdNode->ClipboardSubGraph != nullptr && !EdNode->ClipboardSubGraphSource.IsEmpty()
                && !PastedSources.Contains(EdNode->ClipboardSubGraphSource))
            {
                UKataGraph* NewSource = KataEmbeddedSubGraphEditor::PasteCopy(TargetOwner, EdNode->ClipboardSubGraph);
                if (NewSource != nullptr)
                {
                    PastedSources.Add(EdNode->ClipboardSubGraphSource, NewSource);
                    RememberEmbeddedGraphs();
                }
            }
        }
        TSet<UKataNode*> AuthoredNodes;
        for (UEdGraphNode* Node : EdGraph->Nodes)
        {
            const UKataEdNode* EdNode = Cast<UKataEdNode>(Node);
            if (EdNode != nullptr)
            {
                if (UKataNode* KataNode = Cast<UKataNode>(EdNode->KataNode))
                {
                    AuthoredNodes.Add(KataNode);
                }
            }
        }
        for (UEdGraphNode* Node : PastedNodes)
        {
            Node->SetFlags(RF_Transactional);
            Node->Modify();
            if (UKataEdNode* EdNode = Cast<UKataEdNode>(Node))
            {
                UKataGraphNodeBase* KataNode = EdNode->KataNode;
                if (KataNode == nullptr)
                {
                    continue;
                }
                KataNode->SetFlags(RF_Transactional);
                KataNode->Modify();
                KataNode->Graph = TargetGraph;
                // 저장 때 만든 실행 연결을 복사하지 않는다. 새 저작 핀에서 다음 저장에 다시 만든다.
                KataNode->ParentNodes.Reset();
                KataNode->ChildrenNodes.Reset();
                KataNode->Edges.Reset();
                if (UKataEntryNode* Entry = Cast<UKataEntryNode>(KataNode))
                {
                    Entry->bIsSubGraphEntry = false;
                }
                if (UKataSubGraphPortNode* Port = Cast<UKataSubGraphPortNode>(KataNode))
                {
                    if (Port->bUseEmbeddedSubGraph)
                    {
                        Port->SubGraph = nullptr;
                        if (UKataGraph* const* NewSource = PastedSources.Find(EdNode->ClipboardSubGraphSource))
                        {
                            // 같은 선택의 SubGraph 노드와 참조 포트는 새 원본 한 벌로 함께 연결한다.
                            Port->EmbeddedSubGraph.Graph = *NewSource;
                        }
                        else if (Port->IsA<UKataSubGraphNode>())
                        {
                            // 이전 클립보드에는 저작 사본이 없다. 원본 공유로 조용히 돌아가지 않는다.
                            Port->EmbeddedSubGraph.Graph = nullptr;
                            LOG_WARNING(TEXT("Pasted SubGraph node has no authoring snapshot. Copy the original SubGraph node again."));
                        }
                        else if (Port->EmbeddedSubGraph.Graph != nullptr && Port->GetReferencedSubGraph() == nullptr)
                        {
                            Port->EmbeddedSubGraph.Graph = nullptr;
                            LOG_WARNING(TEXT("Pasted SubGraph node belongs to another owner. Select an embedded source in the target graph."));
                        }
                    }
                    else
                    {
                        Port->EmbeddedSubGraph.Graph = nullptr;
                        if (Port->SubGraph != nullptr && Port->GetReferencedSubGraph() == nullptr)
                        {
                            Port->SubGraph = nullptr;
                            LOG_WARNING(TEXT("Pasted SubGraph node has an invalid external source. The assignment was cleared."));
                        }
                    }
                }
                if (UKataAliasNode* Alias = Cast<UKataAliasNode>(KataNode))
                {
                    Alias->ResolvedSourceNodes.Reset();
                    Alias->SourceNodes.Nodes.RemoveAll([&AuthoredNodes](const TObjectPtr<UKataNode>& Source)
                    {
                        return !AuthoredNodes.Contains(Source.Get());
                    });
                }
                if (UKataGraph* Snapshot = EdNode->ClipboardSubGraph)
                {
                    Snapshot->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_DoNotDirty);
                }
                EdNode->ClipboardSubGraph = nullptr;
                EdNode->ClipboardSubGraphSource.Reset();
            }
            else if (UKataEdNodeEdge* EdEdge = Cast<UKataEdNodeEdge>(Node))
            {
                if (EdEdge->KataEdge != nullptr)
                {
                    EdEdge->KataEdge->SetFlags(RF_Transactional);
                    EdEdge->KataEdge->Modify();
                    EdEdge->KataEdge->Graph = TargetGraph;
                    const UKataEdNode* Start = EdEdge->GetStartNode();
                    const UKataEdNode* End = EdEdge->GetEndNode();
                    EdEdge->KataEdge->StartNode = Start != nullptr ? Start->KataNode : nullptr;
                    EdEdge->KataEdge->EndNode = End != nullptr ? End->KataNode : nullptr;
                }
            }
        }

		//Average position of nodes so we can move them while still maintaining relative distances to each other
		FVector2D AvgNodePosition(0.0f, 0.0f);

		for (TSet<UEdGraphNode*>::TIterator It(PastedNodes); It; ++It)
		{
			UEdGraphNode* Node = *It;
			AvgNodePosition.X += Node->NodePosX;
			AvgNodePosition.Y += Node->NodePosY;
		}

		float InvNumNodes = 1.0f / float(PastedNodes.Num());
		AvgNodePosition.X *= InvNumNodes;
		AvgNodePosition.Y *= InvNumNodes;

		for (TSet<UEdGraphNode*>::TIterator It(PastedNodes); It; ++It)
		{
			UEdGraphNode* Node = *It;
			CurrentGraphEditor->SetNodeSelection(Node, true);

			Node->NodePosX = (Node->NodePosX - AvgNodePosition.X) + Location.X;
			Node->NodePosY = (Node->NodePosY - AvgNodePosition.Y) + Location.Y;

			Node->SnapToGrid(16);

			// Give new node a different Guid from the old one
			Node->CreateNewGuid();
		}
        // 같은 부모의 잘라내기로 남은 미참조 원본은 독립 사본을 만든 뒤 정리한다.
        if (!PastedSources.IsEmpty())
        {
            RemoveUnusedEmbeddedSubGraphs();
        }
	}

	// Update UI
	CurrentGraphEditor->NotifyGraphChanged();

	UObject* GraphOwner = EdGraph->GetOuter();
	if (GraphOwner)
	{
		GraphOwner->PostEditChange();
		GraphOwner->MarkPackageDirty();
	}
}

bool FKataGraphAssetEditor::CanPasteNodes()
{
	TSharedPtr<SGraphEditor> CurrentGraphEditor = GetCurrGraphEditor();
	if (!CurrentGraphEditor.IsValid())
	{
		return false;
	}

	FString ClipboardContent;
	FPlatformApplicationMisc::ClipboardPaste(ClipboardContent);

	return FEdGraphUtilities::CanImportNodesFromText(CurrentGraphEditor->GetCurrentGraph(), ClipboardContent);
}

void FKataGraphAssetEditor::DuplicateNodes()
{
	CopySelectedNodes();
	PasteNodes();
}

bool FKataGraphAssetEditor::CanDuplicateNodes()
{
	return CanCopyNodes();
}

void FKataGraphAssetEditor::GraphSettings()
{
	GraphDetailsWidget->SetObject(EditingGraph);
	if (TabManager.IsValid())
	{
		TabManager->TryInvokeTab(FKataGraphAssetEditorTabs::KataGraphDetailsID);
	}
}

bool FKataGraphAssetEditor::CanGraphSettings() const
{
	return true;
}

void FKataGraphAssetEditor::AutoArrange()
{
	UKataEdGraph* EdGraph = Cast<UKataEdGraph>(EditingGraph->EdGraph);
	check(EdGraph != nullptr);

	const FScopedTransaction Transaction(LOCTEXT("KataGraphEditorAutoArrange", "Kata Graph Editor: Auto Arrange"));

	EdGraph->Modify();

	UKataGraphLayoutStrategy* LayoutStrategy = nullptr;
	switch (KataGraphEditorSettings->AutoLayoutStrategy)
	{
	case EKataGraphLayoutStrategy::Tree:
		LayoutStrategy = NewObject<UKataGraphLayoutStrategy>(EdGraph, UKataGraphTreeLayoutStrategy::StaticClass());
		break;
	case EKataGraphLayoutStrategy::ForceDirected:
		LayoutStrategy = NewObject<UKataGraphLayoutStrategy>(EdGraph, UKataGraphForceDirectedLayoutStrategy::StaticClass());
		break;
	default:
		break;
	}

	if (LayoutStrategy != nullptr)
	{
		LayoutStrategy->Settings = KataGraphEditorSettings;
		LayoutStrategy->Layout(EdGraph);
		LayoutStrategy->ConditionalBeginDestroy();
        if (const TSharedPtr<SGraphEditor> CurrentGraphEditor = GetCurrGraphEditor())
        {
            CurrentGraphEditor->NotifyGraphChanged();
        }
	}
	else
	{
		LOG_ERROR(TEXT("FKataGraphAssetEditor::AutoArrange LayoutStrategy is null."));
	}
}

bool FKataGraphAssetEditor::CanAutoArrange() const
{
	return EditingGraph != nullptr && Cast<UKataEdGraph>(EditingGraph->EdGraph) != nullptr;
}

void FKataGraphAssetEditor::FindInGraph()
{
	if (!SearchWidget.IsValid())
	{
		return;
	}

	// 탭이 닫혀 있거나 뒤에 있을 수 있으므로 먼저 앞으로 꺼낸다.
	if (const TSharedPtr<FTabManager> ToolkitTabManager = GetTabManager())
	{
		ToolkitTabManager->TryInvokeTab(FKataGraphAssetEditorTabs::SearchID);
	}

	SearchWidget->FocusForUse();
}

void FKataGraphAssetEditor::OnRenameNode()
{
	TSharedPtr<SGraphEditor> CurrentGraphEditor = GetCurrGraphEditor();
	if (CurrentGraphEditor.IsValid())
	{
		const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
		for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
		{
			UEdGraphNode* SelectedNode = Cast<UEdGraphNode>(*NodeIt);
			if (SelectedNode != NULL && SelectedNode->bCanRenameNode)
			{
				CurrentGraphEditor->IsNodeTitleVisible(SelectedNode, true);
				break;
			}
		}
	}
}

bool FKataGraphAssetEditor::CanRenameNodes() const
{
	UKataEdGraph* EdGraph = Cast<UKataEdGraph>(EditingGraph->EdGraph);
	check(EdGraph != nullptr);

	UKataGraphBase* Graph = EdGraph->GetKataGraph();
	check(Graph != nullptr)

	return Graph->bCanRenameNode && GetSelectedNodes().Num() == 1;
}

void FKataGraphAssetEditor::OnSelectedNodesChanged(const TSet<class UObject*>& NewSelection)
{
	TArray<UObject*> Selection;

	for (UObject* SelectionEntry : NewSelection)
	{
		Selection.Add(SelectionEntry);
	}

	if (Selection.Num() == 0) 
	{
		SelectionDetailsWidget->SetObject(nullptr);
	}
	else
	{
		SelectionDetailsWidget->SetObjects(Selection);
	}
}

void FKataGraphAssetEditor::OnNodeDoubleClicked(UEdGraphNode* Node)
{
    const UKataEdNode* EdNode = Cast<UKataEdNode>(Node);
    const UKataSubGraphPortNode* Port = EdNode != nullptr ? Cast<UKataSubGraphPortNode>(EdNode->KataNode) : nullptr;
    if (Port == nullptr)
    {
        return;
    }

    UKataGraph* ReferencedGraph = Port->GetReferencedSubGraph();
    if (ReferencedGraph == nullptr)
    {
        return;
    }

    if (Port->bUseEmbeddedSubGraph)
    {
        OpenGraph(ReferencedGraph);
    }
    else if (GEditor != nullptr)
    {
        GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(ReferencedGraph);
    }
}

void FKataGraphAssetEditor::OnFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent)
{
	if (EditingGraph == nullptr)
		return;

	// 제목처럼 프로퍼티에서 끌어오는 표시는 SNodeTitle이 캐시한다. 캐시를 낡았다고 표시하는 것만으로는
	// 그래프 패널이 노드를 다시 그리지 않으므로 변경 알림까지 보낸다. 서브그래프를 할당해도 포트
	// 제목이 그대로이던 문제가 여기였다.
	EditingGraph->EdGraph->GetSchema()->ForceVisualizationCacheClear();

	if (const TSharedPtr<SGraphEditor> GraphEditor = GetCurrGraphEditor())
	{
		GraphEditor->NotifyGraphChanged();
	}
}

void FKataGraphAssetEditor::RegisterToolbarTab(const TSharedRef<class FTabManager>& InTabManager) 
{
	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);
}


#undef LOCTEXT_NAMESPACE
