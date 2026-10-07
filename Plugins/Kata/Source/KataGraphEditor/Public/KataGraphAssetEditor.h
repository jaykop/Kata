#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "EditorUndoClient.h"
#include "GraphEditor.h"
#include "KataGraphEditorSettings.h"
#include "KataGraphBase.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "UObject/GCObject.h"

class FGGAssetEditorToolbar;
class FKataGraphDebugger;
class UKataGraphNodeBase;
class UKataGraph;
class SBox;

class KATAGRAPHEDITOR_API FKataGraphAssetEditor : public FAssetEditorToolkit, public FNotifyHook, public FGCObject, public FEditorUndoClient
{
public:
	FKataGraphAssetEditor();
	virtual ~FKataGraphAssetEditor();

	void InitKataGraphEditor(const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, UKataGraphBase* Graph);

	// IToolkit interface
	virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& TabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& TabManager) override;
	// End of IToolkit interface

	// FAssetEditorToolkit
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FText GetToolkitName() const override;
	virtual FText GetToolkitToolTipText() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FString GetDocumentationLink() const override;
	virtual void SaveAsset_Execute() override;
    virtual bool OnRequestClose(EAssetEditorCloseReason InCloseReason) override;
	// End of FAssetEditorToolkit

	//Toolbar
	void UpdateToolbar();
	TSharedPtr<class FKataGraphEditorToolbar> GetToolbarBuilder() { return ToolbarBuilder; }
	void RegisterToolbarTab(const TSharedRef<class FTabManager>& TabManager);


	// FSerializableObject interface
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	// End of FSerializableObject interface

#if ENGINE_MAJOR_VERSION == 5
	// FGCObject interface
	virtual FString GetReferencerName() const
	{
		return TEXT("FKataGraphAssetEditor");
	}
	// ~FGCObject interface
#endif // #if ENGINE_MAJOR_VERSION == 5

	UKataGraphEditorSettings* GetSettings() const;

    /** 내장 원본을 소유한 루트 에셋을 반환한다. Kata 고유 그래프가 아니면 nullptr이다. */
    UKataGraph* GetRootKataGraph() const;

    /** 입력 콜백이 끝난 뒤 루트 소유 트리의 페이지를 연다. 마지막 요청만 반영한다. */
    void OpenGraph(UKataGraphBase* Graph);

    /** PIE 디버그 대상과 노드 강조 상태. 툴바와 Debug 탭이 같은 객체를 공유한다. */
    TSharedPtr<FKataGraphDebugger> GetDebugger() const { return Debugger; }

    /** 실행 노드가 저작된 페이지로 전환하고 그 노드로 화면을 옮긴다. 페이지 전환은 다음 Tick에 적용된다. */
    void JumpToDebugNode(const UKataGraphNodeBase* Node);

    virtual void PostUndo(bool bSuccess) override;
    virtual void PostRedo(bool bSuccess) override { PostUndo(bSuccess); }

protected:
	TSharedRef<SDockTab> SpawnTab_Viewport(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_GraphDetails(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_SelectionDetails(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_EditorSettings(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_Search(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_Debug(const FSpawnTabArgs& Args);

	void CreateInternalWidgets();
	TSharedRef<SGraphEditor> CreateViewportWidget();


	void BindCommands();

    void CreateEdGraph(UKataGraphBase* Graph);
    void OpenGraphNow(UKataGraphBase* Graph);
    bool ApplyPendingGraphNavigation(float DeltaTime);
    void RefreshSubGraphUI();
    void RemoveUnusedEmbeddedSubGraphs();
    void RememberEmbeddedGraphs();
    UKataGraphBase* GetValidGraphAncestor(UKataGraphBase* Graph) const;
    bool RefreshDependencyStatus(float DeltaTime);
    bool TickDebugger(float DeltaTime);
    FReply NavigateToRoot();
    void NavigateBack();
    FText GetCurrentGraphName() const;

	void CreateCommandList();

	TSharedPtr<SGraphEditor> GetCurrGraphEditor() const;

	FGraphPanelSelectionSet GetSelectedNodes() const;


	// Delegates for graph editor commands
	void SelectAllNodes();
	bool CanSelectAllNodes();
	void DeleteSelectedNodes();
	bool CanDeleteNodes();
	void DeleteSelectedDuplicatableNodes();
	void CutSelectedNodes();
	bool CanCutNodes();
	void CopySelectedNodes();
	bool CanCopyNodes();
	void PasteNodes();
	void PasteNodesHere(const FVector2f& Location);
	bool CanPasteNodes();
	void DuplicateNodes();
	bool CanDuplicateNodes();
	void CreateComment();
	bool CanCreateComment() const;

	void GraphSettings();
	bool CanGraphSettings() const;

	void AutoArrange();
	bool CanAutoArrange() const;

	/** 검색 탭을 열고 검색창에 입력 초점을 준다. */
	void FindInGraph();

	void OnRenameNode();
	bool CanRenameNodes() const;

	//////////////////////////////////////////////////////////////////////////
	// graph editor event
	void OnSelectedNodesChanged(const TSet<class UObject*>& NewSelection);

	void OnNodeDoubleClicked(UEdGraphNode* Node);

	void OnFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent);


protected:
    TObjectPtr<UKataGraphEditorSettings> KataGraphEditorSettings;

    /** 툴킷 등록과 저장 패키지를 결정하는 루트. 화면을 전환해도 바뀌지 않는다. */
    TObjectPtr<UKataGraphBase> RootGraphAsset;

	// 증분 GC에서 AddReferencedObject가 안전하게 추적하도록 TObjectPtr로 보관한다.
	TObjectPtr<UKataGraphBase> EditingGraph;

	//Toolbar
	TSharedPtr<class FKataGraphEditorToolbar> ToolbarBuilder;


	TSharedPtr<SGraphEditor> ViewportWidget;
    TSharedPtr<SBox> ViewportContainer;
    TSharedPtr<SBox> SearchContainer;
    /** 전환 대기 중에도 GC가 대상 원본을 추적한다. */
    TObjectPtr<UKataGraphBase> PendingGraph;
    FTSTicker::FDelegateHandle GraphNavigationTicker;
    FTSTicker::FDelegateHandle DependencyStatusTicker;
    FTSTicker::FDelegateHandle DebuggerTicker;
    TSharedPtr<FKataGraphDebugger> Debugger;
    /** 페이지 전환을 기다리는 디버그 이동 대상. 전환 후 화면을 옮기고 비운다. */
    TWeakObjectPtr<UEdGraphNode> PendingJumpNode;
    FText DependencyStatus;
    /** 내부 내용 없이 참조 포트만 잘라냈을 때 원본 경로를 유지한다. */
    bool bPreserveEmbeddedSubGraphsForCut = false;

    struct FKataGraphViewState
    {
        FVector2f Location = FVector2f::ZeroVector;
        float Zoom = 1.0f;
    };

    /** 저장 대상이 아닌 화면 위치. 삭제한 내장을 이 캐시가 소유하지 않는다. */
    TMap<TWeakObjectPtr<UKataGraphBase>, FKataGraphViewState> GraphViewStates;

    /** 삭제·Undo로 목록이 바뀔 때 원본의 Outer를 복구할 대상. 수명은 소유 목록과 Undo 기록이 관리한다. */
    TMap<TWeakObjectPtr<UKataGraph>, TWeakObjectPtr<UKataGraph>> KnownEmbeddedGraphs;
	TSharedPtr<class IDetailsView> GraphDetailsWidget;
	TSharedPtr<class IDetailsView> SelectionDetailsWidget;
	TSharedPtr<class IDetailsView> EditorSettingsWidget;
	TSharedPtr<class SKataFindInGraph> SearchWidget;

	/** The command list for this editor */
	TSharedPtr<FUICommandList> GraphEditorCommands;
};

