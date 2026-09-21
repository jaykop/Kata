#pragma once

#include "CoreMinimal.h"
#include "KataGraphEditorSettings.h"
#include "KataGraphBase.h"

#if ENGINE_MAJOR_VERSION == 5
#include "UObject/ObjectSaveContext.h"
#endif // #if ENGINE_MAJOR_VERSION == 5

class FGGAssetEditorToolbar;

class KATAGRAPHEDITOR_API FKataGraphAssetEditor : public FAssetEditorToolkit, public FNotifyHook, public FGCObject
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

protected:
	TSharedRef<SDockTab> SpawnTab_Viewport(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_GraphDetails(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_SelectionDetails(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_EditorSettings(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_Search(const FSpawnTabArgs& Args);

	void CreateInternalWidgets();
	TSharedRef<SGraphEditor> CreateViewportWidget();


	void BindCommands();

	void CreateEdGraph();

	void CreateCommandList();

	TSharedPtr<SGraphEditor> GetCurrGraphEditor() const;

	FGraphPanelSelectionSet GetSelectedNodes() const;

	void RebuildKataGraph();

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
	void PasteNodesHere(const FVector2D& Location);
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

#if ENGINE_MAJOR_VERSION < 5
	void OnPackageSaved(const FString& PackageFileName, UObject* Outer);
#else // #if ENGINE_MAJOR_VERSION < 5
	void OnPackageSavedWithContext(const FString& PackageFileName, UPackage* Package, FObjectPostSaveContext ObjectSaveContext);
#endif // #else // #if ENGINE_MAJOR_VERSION < 5

protected:
	UKataGraphEditorSettings* KataGraphEditorSettings;

	UKataGraphBase* EditingGraph;

	//Toolbar
	TSharedPtr<class FKataGraphEditorToolbar> ToolbarBuilder;

	/** Handle to the registered OnPackageSave delegate */
	FDelegateHandle OnPackageSavedDelegateHandle;

	TSharedPtr<SGraphEditor> ViewportWidget;
	TSharedPtr<class IDetailsView> GraphDetailsWidget;
	TSharedPtr<class IDetailsView> SelectionDetailsWidget;
	TSharedPtr<class IDetailsView> EditorSettingsWidget;
	TSharedPtr<class SKataFindInGraph> SearchWidget;

	/** The command list for this editor */
	TSharedPtr<FUICommandList> GraphEditorCommands;
};

