
#pragma once

#include "CoreMinimal.h"

class FKataGraphAssetEditor;
class FExtender;
class FToolBarBuilder;

class KATAGRAPHEDITOR_API FKataGraphEditorToolbar : public TSharedFromThis<FKataGraphEditorToolbar>
{
public:
	FKataGraphEditorToolbar(TSharedPtr<FKataGraphAssetEditor> InKataGraphEditor)
		: KataGraphEditor(InKataGraphEditor) {}

	void AddKataGraphToolbar(TSharedPtr<FExtender> Extender);

private:
	void FillKataGraphToolbar(FToolBarBuilder& ToolbarBuilder);

protected:
	/** Pointer back to the blueprint editor tool that owns us */
	TWeakPtr<FKataGraphAssetEditor> KataGraphEditor;
};
