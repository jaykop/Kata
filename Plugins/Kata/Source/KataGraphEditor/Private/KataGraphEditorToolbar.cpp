#include "KataGraphEditorToolbar.h"
#include "KataGraphAssetEditor.h"
#include "KataGraphEditorCommands.h"
#include "KataGraphEditorStyle.h"

#define LOCTEXT_NAMESPACE "AssetEditorToolbar_KataGraph"

void FKataGraphEditorToolbar::AddKataGraphToolbar(TSharedPtr<FExtender> Extender)
{
	check(KataGraphEditor.IsValid());
	TSharedPtr<FKataGraphAssetEditor> KataGraphEditorPtr = KataGraphEditor.Pin();

	TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
	ToolbarExtender->AddToolBarExtension("Asset", EExtensionHook::After, KataGraphEditorPtr->GetToolkitCommands(), FToolBarExtensionDelegate::CreateSP( this, &FKataGraphEditorToolbar::FillKataGraphToolbar ));
	KataGraphEditorPtr->AddToolbarExtender(ToolbarExtender);
}

void FKataGraphEditorToolbar::FillKataGraphToolbar(FToolBarBuilder& ToolbarBuilder)
{
	check(KataGraphEditor.IsValid());
	TSharedPtr<FKataGraphAssetEditor> KataGraphEditorPtr = KataGraphEditor.Pin();

	ToolbarBuilder.BeginSection("Kata Graph");
	{
		ToolbarBuilder.AddToolBarButton(FKataGraphEditorCommands::Get().GraphSettings,
			NAME_None,
			LOCTEXT("GraphSettings_Label", "Graph Settings"),
			LOCTEXT("GraphSettings_ToolTip", "Show the Graph Settings"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.GameSettings"));
	}
	ToolbarBuilder.EndSection();

	ToolbarBuilder.BeginSection("Util");
	{
		ToolbarBuilder.AddToolBarButton(FKataGraphEditorCommands::Get().AutoArrange,
			NAME_None,
			LOCTEXT("AutoArrange_Label", "Auto Arrange"),
			LOCTEXT("AutoArrange_ToolTip", "Auto arrange nodes, not perfect, but still handy"),
			FSlateIcon(FKataGraphEditorStyle::GetStyleSetName(), "KataGraphEditor.AutoArrange"));
	}
	ToolbarBuilder.EndSection();

}


#undef LOCTEXT_NAMESPACE
