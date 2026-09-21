#pragma once

#include "CoreMinimal.h"

class KATAGRAPHEDITOR_API FKataGraphEditorCommands : public TCommands<FKataGraphEditorCommands>
{
public:
	/** Constructor */
	FKataGraphEditorCommands()
		: TCommands<FKataGraphEditorCommands>("KataGraphEditor", NSLOCTEXT("Contexts", "KataGraphEditor", "Kata Graph Editor"), NAME_None, FAppStyle::GetAppStyleSetName())
	{
	}
	
	TSharedPtr<FUICommandInfo> GraphSettings;
	TSharedPtr<FUICommandInfo> AutoArrange;
	TSharedPtr<FUICommandInfo> FindInGraph;

	virtual void RegisterCommands() override;
};
