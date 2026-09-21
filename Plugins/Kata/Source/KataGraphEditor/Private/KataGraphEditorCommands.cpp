#include "KataGraphEditorCommands.h"

#define LOCTEXT_NAMESPACE "EditorCommands_KataGraph"

void FKataGraphEditorCommands::RegisterCommands()
{
	UI_COMMAND(GraphSettings, "Graph Settings", "Graph Settings", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(AutoArrange, "Auto Arrange", "Auto Arrange", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(FindInGraph, "Find in Graph", "Search nodes and transitions in this graph", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control, EKeys::F));
}

#undef LOCTEXT_NAMESPACE
