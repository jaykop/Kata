#include "KataGraphEditorModule.h"

#include "EdGraphUtilities.h"
#include "KataAliasNode.h"
#include "KataAliasSourceSetCustomization.h"
#include "KataEmbeddedSubGraphReferenceCustomization.h"
#include "KataSubGraphPortNode.h"
#include "KataGraphEditorStyle.h"
#include "KataGraphNodeFactory.h"
#include "KataGraphBase.h"
#include "KataGraphBuildContext.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "UObject/ObjectSaveContext.h"
#include "UObject/Package.h"
#include "UObject/UObjectHash.h"

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
    PropertyModule.RegisterCustomPropertyTypeLayout(
        FKataEmbeddedSubGraphReference::StaticStruct()->GetFName(),
        FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FKataEmbeddedSubGraphReferenceCustomization::MakeInstance));
    PropertyModule.NotifyCustomizationModuleChanged();

    // 열린 툴킷 수와 무관하게 패키지당 한 번 재구성한다. 닫힌 외장 원본의 Save All도 같은 경로다.
    PackagePreSaveHandle = UPackage::PreSavePackageWithContextEvent.AddLambda(
        [](UPackage* Package, FObjectPreSaveContext SaveContext)
        {
            if (Package == nullptr || SaveContext.IsCooking())
            {
                return;
            }
            TArray<UObject*> Objects;
            GetObjectsWithOuter(Package, Objects, EGetObjectsFlags::None);
            for (UObject* Object : Objects)
            {
                UKataGraphBase* Graph = Cast<UKataGraphBase>(Object);
                if (Graph != nullptr && Graph->IsAsset())
                {
                    FKataGraphBuildContext::Rebuild(Graph, true);
                }
            }
        });
}

void FKataGraphEditorModule::ShutdownModule()
{
    UPackage::PreSavePackageWithContextEvent.Remove(PackagePreSaveHandle);
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
        PropertyModule.UnregisterCustomPropertyTypeLayout(
            FKataEmbeddedSubGraphReference::StaticStruct()->GetFName());
        PropertyModule.NotifyCustomizationModuleChanged();
    }

    FKataGraphEditorStyle::Shutdown();
}

IMPLEMENT_MODULE(FKataGraphEditorModule, KataGraphEditor)
