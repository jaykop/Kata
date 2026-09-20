#include "KataGraphEditorModule.h"

#include "EdGraphUtilities.h"
#include "KataGraphEditorStyle.h"
#include "KataGraphNodeFactory.h"
#include "Modules/ModuleManager.h"

void FKataGraphEditorModule::StartupModule()
{
    FKataGraphEditorStyle::Initialize();

    NodeFactory = MakeShareable(new FKataGraphNodeFactory());
    FEdGraphUtilities::RegisterVisualNodeFactory(NodeFactory);
}

void FKataGraphEditorModule::ShutdownModule()
{
    if (NodeFactory.IsValid())
    {
        FEdGraphUtilities::UnregisterVisualNodeFactory(NodeFactory);
        NodeFactory.Reset();
    }

    FKataGraphEditorStyle::Shutdown();
}

IMPLEMENT_MODULE(FKataGraphEditorModule, KataGraphEditor)
