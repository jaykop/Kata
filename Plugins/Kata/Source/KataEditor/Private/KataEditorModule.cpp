#include "KataEditorModule.h"

#include "Modules/ModuleManager.h"

IMPLEMENT_MODULE(FKataEditorModule, KataEditor)

FName KataEditor::GetPreviewViewportToolbarMenuName()
{
    return TEXT("KataActionEditor.ViewportToolbar");
}
