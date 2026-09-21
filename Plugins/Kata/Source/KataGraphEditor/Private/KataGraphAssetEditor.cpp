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
};

//////////////////////////////////////////////////////////////////////////

const FName FKataGraphAssetEditorTabs::KataGraphDetailsID(TEXT("KataGraphProperty"));
const FName FKataGraphAssetEditorTabs::SelectionDetailsID(TEXT("KataGraphSelectionDetails"));
const FName FKataGraphAssetEditorTabs::ViewportID(TEXT("Viewport"));
const FName FKataGraphAssetEditorTabs::KataGraphEditorSettingsID(TEXT("KataGraphEditorSettings"));

//////////////////////////////////////////////////////////////////////////

FKataGraphAssetEditor::FKataGraphAssetEditor()
{
	EditingGraph = nullptr;

	KataGraphEditorSettings = NewObject<UKataGraphEditorSettings>(UKataGraphEditorSettings::StaticClass());

#if ENGINE_MAJOR_VERSION < 5
	OnPackageSavedDelegateHandle = UPackage::PackageSavedEvent.AddRaw(this, &FKataGraphAssetEditor::OnPackageSaved);
#else // #if ENGINE_MAJOR_VERSION < 5
	OnPackageSavedDelegateHandle = UPackage::PackageSavedWithContextEvent.AddRaw(this, &FKataGraphAssetEditor::OnPackageSavedWithContext);
#endif // #else // #if ENGINE_MAJOR_VERSION < 5
}

FKataGraphAssetEditor::~FKataGraphAssetEditor()
{
#if ENGINE_MAJOR_VERSION < 5
	UPackage::PackageSavedEvent.Remove(OnPackageSavedDelegateHandle);
#else // #if ENGINE_MAJOR_VERSION < 5
	UPackage::PackageSavedWithContextEvent.Remove(OnPackageSavedDelegateHandle);
#endif // #else // #if ENGINE_MAJOR_VERSION < 5
}

void FKataGraphAssetEditor::InitKataGraphEditor(const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, UKataGraphBase* Graph)
{
	EditingGraph = Graph;
	CreateEdGraph();

	FGenericCommands::Register();
	FGraphEditorCommands::Register();
	FKataGraphEditorCommands::Register();

	if (!ToolbarBuilder.IsValid())
	{
		ToolbarBuilder = MakeShareable(new FKataGraphEditorToolbar(SharedThis(this)));
	}

	BindCommands();

	CreateInternalWidgets();

	TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);

	ToolbarBuilder->AddKataGraphToolbar(ToolbarExtender);

	// Layout
	const TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout("Standalone_KataGraphEditor_Layout_v2")
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
					)
				)
			)
		);

	const bool bCreateDefaultStandaloneMenu = true;
	const bool bCreateDefaultToolbar = true;
	FAssetEditorToolkit::InitAssetEditor(Mode, InitToolkitHost, KataGraphEditorAppName, StandaloneDefaultLayout, bCreateDefaultStandaloneMenu, bCreateDefaultToolbar, EditingGraph, false);

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
}

void FKataGraphAssetEditor::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);

	InTabManager->UnregisterTabSpawner(FKataGraphAssetEditorTabs::ViewportID);
	InTabManager->UnregisterTabSpawner(FKataGraphAssetEditorTabs::KataGraphDetailsID);
	InTabManager->UnregisterTabSpawner(FKataGraphAssetEditorTabs::SelectionDetailsID);
	InTabManager->UnregisterTabSpawner(FKataGraphAssetEditorTabs::KataGraphEditorSettingsID);
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
	const bool bDirtyState = EditingGraph->GetOutermost()->IsDirty();

	FFormatNamedArguments Args;
	Args.Add(TEXT("KataGraphName"), FText::FromString(EditingGraph->GetName()));
	Args.Add(TEXT("DirtyState"), bDirtyState ? FText::FromString(TEXT("*")) : FText::GetEmpty());
	return FText::Format(LOCTEXT("KataGraphEditorToolkitName", "{KataGraphName}{DirtyState}"), Args);
}

FText FKataGraphAssetEditor::GetToolkitToolTipText() const
{
	return FAssetEditorToolkit::GetToolTipTextForObject(EditingGraph);
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
	if (EditingGraph != nullptr)
	{
		RebuildKataGraph();
	}

	FAssetEditorToolkit::SaveAsset_Execute();
}

void FKataGraphAssetEditor::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(EditingGraph);
	Collector.AddReferencedObject(EditingGraph->EdGraph);
}

UKataGraphEditorSettings* FKataGraphAssetEditor::GetSettings() const
{
	return KataGraphEditorSettings;
}

TSharedRef<SDockTab> FKataGraphAssetEditor::SpawnTab_Viewport(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == FKataGraphAssetEditorTabs::ViewportID);

	TSharedRef<SDockTab> SpawnedTab = SNew(SDockTab)
		.Label(LOCTEXT("ViewportTab_Title", "Viewport"));

	if (ViewportWidget.IsValid())
	{
		SpawnedTab->SetContent(ViewportWidget.ToSharedRef());
	}

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

void FKataGraphAssetEditor::CreateInternalWidgets()
{
	ViewportWidget = CreateViewportWidget();

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
}

void FKataGraphAssetEditor::CreateEdGraph()
{
	if (EditingGraph->EdGraph == nullptr)
	{
		EditingGraph->EdGraph = CastChecked<UKataEdGraph>(FBlueprintEditorUtils::CreateNewGraph(EditingGraph, NAME_None, UKataEdGraph::StaticClass(), UKataGraphSchema::StaticClass()));
		EditingGraph->EdGraph->bAllowDeletion = false;

		// Give the schema a chance to fill out any required nodes (like the results node)
		const UEdGraphSchema* Schema = EditingGraph->EdGraph->GetSchema();
		Schema->CreateDefaultNodesForGraph(*EditingGraph->EdGraph);
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

void FKataGraphAssetEditor::RebuildKataGraph()
{
	if (EditingGraph == nullptr)
	{
		LOG_WARNING(TEXT("FKataGraphAssetEditor::RebuildKataGraph EditingGraph is nullptr"));
		return;
	}

	UKataEdGraph* EdGraph = Cast<UKataEdGraph>(EditingGraph->EdGraph);
	check(EdGraph != nullptr);

	EdGraph->RebuildKataGraph();
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

	for (FGraphPanelSelectionSet::TIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
		if (Node == nullptr)
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

		Node->PrepareForCopying();
	}

	FEdGraphUtilities::ExportNodesToText(SelectedNodes, ExportedText);
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
		PasteNodesHere(CurrentGraphEditor->GetPasteLocation());
	}
}

void FKataGraphAssetEditor::PasteNodesHere(const FVector2D& Location)
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
		const FScopedTransaction Transaction(FGenericCommands::Get().Paste->GetDescription());
		EdGraph->Modify();

		// Clear the selection set (newly pasted stuff will be selected)
		CurrentGraphEditor->ClearSelectionSet();

		// Grab the text to paste from the clipboard.
		FString TextToImport;
		FPlatformApplicationMisc::ClipboardPaste(TextToImport);

		// Import the nodes
		TSet<UEdGraphNode*> PastedNodes;
		FEdGraphUtilities::ImportNodesFromText(EdGraph, TextToImport, PastedNodes);

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
	
}

void FKataGraphAssetEditor::OnFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent)
{
	if (EditingGraph == nullptr)
		return;

	EditingGraph->EdGraph->GetSchema()->ForceVisualizationCacheClear();
}

#if ENGINE_MAJOR_VERSION < 5
void FKataGraphAssetEditor::OnPackageSaved(const FString& PackageFileName, UObject* Outer)
{
	RebuildKataGraph();
}
#else // #if ENGINE_MAJOR_VERSION < 5
void FKataGraphAssetEditor::OnPackageSavedWithContext(const FString& PackageFileName, UPackage* Package, FObjectPostSaveContext ObjectSaveContext)
{
	RebuildKataGraph();
}
#endif // #else // #if ENGINE_MAJOR_VERSION < 5

void FKataGraphAssetEditor::RegisterToolbarTab(const TSharedRef<class FTabManager>& InTabManager) 
{
	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);
}


#undef LOCTEXT_NAMESPACE
