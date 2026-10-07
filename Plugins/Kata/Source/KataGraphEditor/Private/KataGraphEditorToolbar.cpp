#include "KataGraphEditorToolbar.h"
#include "KataGraphAssetEditor.h"
#include "KataGraphEditorCommands.h"
#include "KataGraphEditorStyle.h"
#include "KataGraphComponent.h"
#include "KataGraphDebugger.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Text/STextBlock.h"

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

	ToolbarBuilder.BeginSection("Debugging");
	{
		// 블루프린트 디버그 필터처럼 PIE에서 이 그래프를 실행 중인 액터 하나를 고른다. 메뉴를 열 때마다 목록을 새로 모은다.
		const TWeakPtr<FKataGraphAssetEditor> WeakEditor = KataGraphEditorPtr;
		ToolbarBuilder.AddWidget(
			SNew(SComboButton)
			.ToolTipText(LOCTEXT("DebugObject_ToolTip", "Select a Play-In-Editor actor running this graph to highlight its current node."))
			.ButtonContent()
			[
				SNew(STextBlock)
				.Text_Lambda([WeakEditor]()
				{
					const TSharedPtr<FKataGraphAssetEditor> Editor = WeakEditor.Pin();
					const TSharedPtr<FKataGraphDebugger> Debugger = Editor.IsValid() ? Editor->GetDebugger() : TSharedPtr<FKataGraphDebugger>();
					return Debugger.IsValid() ? Debugger->GetDebugTargetLabel() : FText::GetEmpty();
				})
			]
			.OnGetMenuContent_Lambda([WeakEditor]()
			{
				FMenuBuilder MenuBuilder(true, nullptr);
				const TSharedPtr<FKataGraphAssetEditor> Editor = WeakEditor.Pin();
				const TSharedPtr<FKataGraphDebugger> Debugger = Editor.IsValid() ? Editor->GetDebugger() : TSharedPtr<FKataGraphDebugger>();
				if (!Debugger.IsValid())
				{
					return MenuBuilder.MakeWidget();
				}
				const TWeakPtr<FKataGraphDebugger> WeakDebugger = Debugger;
				MenuBuilder.AddMenuEntry(LOCTEXT("NoDebugObject", "No Debug Object"), FText::GetEmpty(), FSlateIcon(),
					FUIAction(FExecuteAction::CreateLambda([WeakDebugger]()
					{
						if (const TSharedPtr<FKataGraphDebugger> Pinned = WeakDebugger.Pin())
						{
							Pinned->SetDebugTarget(nullptr);
						}
					})));
				TArray<UKataGraphComponent*> Candidates;
				Debugger->GatherCandidates(Candidates);
				if (Candidates.IsEmpty())
				{
					MenuBuilder.AddMenuEntry(LOCTEXT("NoCandidates", "No actor is running this graph in Play"), FText::GetEmpty(),
						FSlateIcon(), FUIAction(FExecuteAction(), FCanExecuteAction::CreateLambda([]() { return false; })));
				}
				for (UKataGraphComponent* Component : Candidates)
				{
					const TWeakObjectPtr<UKataGraphComponent> WeakComponent = Component;
					MenuBuilder.AddMenuEntry(FKataGraphDebugger::GetComponentLabel(Component), FText::GetEmpty(), FSlateIcon(),
						FUIAction(FExecuteAction::CreateLambda([WeakDebugger, WeakComponent]()
						{
							const TSharedPtr<FKataGraphDebugger> Pinned = WeakDebugger.Pin();
							if (Pinned.IsValid() && WeakComponent.IsValid())
							{
								Pinned->SetDebugTarget(WeakComponent.Get());
							}
						})));
				}
				return MenuBuilder.MakeWidget();
			}),
			NAME_None, false);
	}
	ToolbarBuilder.EndSection();
}


#undef LOCTEXT_NAMESPACE
