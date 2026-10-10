#include "KataActionFactory.h"
#include "Action/KataAction.h"
#include "Action/KataActionTemplate.h"
#include "Action/KataPreviewSetup.h"
#include "AssetToolsModule.h"
#include "Editor.h"
#include "Misc/PackageName.h"
#include "Subsystems/AssetEditorSubsystem.h"

UKataActionFactory::UKataActionFactory()
{
    SupportedClass = UKataAction::StaticClass();
    bCreateNew = true;
    bEditAfterNew = true;
}

UObject* UKataActionFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name,
    EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
    UKataAction* Asset = NewObject<UKataAction>(InParent, Class, Name, Flags | RF_Transactional);
    Asset->ParentAction = ParentAction;
#if WITH_EDITORONLY_DATA
    if (ParentAction)
    {
        Asset->PreviewActorClass = ParentAction->PreviewActorClass;
        Asset->PreviewTargetClass = ParentAction->PreviewTargetClass;
        // Instanced 설정은 부모의 객체를 공유하지 않도록 새 에셋 아래로 복제한다.
        for (const TObjectPtr<UKataPreviewSetup>& Setup : ParentAction->PreviewSetups)
        {
            if (Setup != nullptr)
            {
                Asset->PreviewSetups.Add(DuplicateObject<UKataPreviewSetup>(Setup, Asset));
            }
        }
        Asset->PreviewActorTransform = ParentAction->PreviewActorTransform;
        Asset->PreviewTargetTransform = ParentAction->PreviewTargetTransform;
        Asset->PreviewLightRotation = ParentAction->PreviewLightRotation;
        Asset->PreviewLightBrightness = ParentAction->PreviewLightBrightness;
        Asset->PreviewLightColor = ParentAction->PreviewLightColor;
        Asset->PreviewBackgroundColor = ParentAction->PreviewBackgroundColor;
        Asset->PreviewEnvironmentSize = ParentAction->PreviewEnvironmentSize;
        Asset->bPreviewShowFrontWall = ParentAction->bPreviewShowFrontWall;
        Asset->bPreviewShowSideWall = ParentAction->bPreviewShowSideWall;
        Asset->bPreviewShowDebugShape = ParentAction->bPreviewShowDebugShape;
        Asset->PreviewDebugShape = ParentAction->PreviewDebugShape;
        Asset->PreviewDebugColor = ParentAction->PreviewDebugColor;
        Asset->PreviewDebugThickness = ParentAction->PreviewDebugThickness;
        Asset->PreviewGridCellSize = ParentAction->PreviewGridCellSize;
    }
#endif
    return Asset;
}

UKataAction* UKataActionFactory::CreateChildWithDialog(UKataActionTemplate* Template)
{
    if (Template == nullptr)
    {
        return nullptr;
    }
    UKataActionFactory* Factory = NewObject<UKataActionFactory>();
    Factory->ParentAction = Template;
    FAssetToolsModule& Tools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
    UKataAction* Child = Cast<UKataAction>(Tools.Get().CreateAssetWithDialog(Template->GetName() + TEXT("_Child"),
        FPackageName::GetLongPackagePath(Template->GetOutermost()->GetName()), UKataAction::StaticClass(), Factory));
    if (Child)
    {
        GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Child);
    }
    return Child;
}

UKataActionTemplateFactory::UKataActionTemplateFactory()
{
    SupportedClass = UKataActionTemplate::StaticClass();
    bCreateNew = true;
    bEditAfterNew = true;
}

UObject* UKataActionTemplateFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name,
    EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
    return NewObject<UKataActionTemplate>(InParent, Class, Name, Flags | RF_Transactional);
}
