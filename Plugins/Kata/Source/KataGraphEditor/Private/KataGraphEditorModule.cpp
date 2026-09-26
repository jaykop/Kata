#include "KataGraphEditorModule.h"

#include "EdGraphUtilities.h"
#include "KataAliasNode.h"
#include "KataAliasSourceSetCustomization.h"
#include "KataGraphEditorStyle.h"
#include "KataGraphNodeFactory.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"

void FKataGraphEditorModule::StartupModule()
{
    FKataGraphEditorStyle::Initialize();

    NodeFactory = MakeShareable(new FKataGraphNodeFactory());
    FEdGraphUtilities::RegisterVisualNodeFactory(NodeFactory);

    // 별칭의 출발지는 에셋이 아니라 그래프 안의 노드라서 기본 배열 UI로 고를 수 없다.
    FPropertyEditorModule& PropertyModule =
        FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
    PropertyModule.RegisterCustomPropertyTypeLayout(
        FKataAliasSourceSet::StaticStruct()->GetFName(),
        FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FKataAliasSourceSetCustomization::MakeInstance));
    PropertyModule.NotifyCustomizationModuleChanged();
}

void FKataGraphEditorModule::ShutdownModule()
{
    if (NodeFactory.IsValid())
    {
        FEdGraphUtilities::UnregisterVisualNodeFactory(NodeFactory);
        NodeFactory.Reset();
    }

    if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
    {
        FPropertyEditorModule& PropertyModule =
            FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
        PropertyModule.UnregisterCustomPropertyTypeLayout(
            FKataAliasSourceSet::StaticStruct()->GetFName());
        PropertyModule.NotifyCustomizationModuleChanged();
    }

    FKataGraphEditorStyle::Shutdown();
}

IMPLEMENT_MODULE(FKataGraphEditorModule, KataGraphEditor)
