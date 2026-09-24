#include "KataActionFactory.h"
#include "Action/KataAction.h"

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
