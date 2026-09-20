#include "KataAssetFactory.h"
#include "Definition/KataAsset.h"

UKataAssetFactory::UKataAssetFactory()
{
    SupportedClass = UKataAsset::StaticClass();
    bCreateNew = true;
    bEditAfterNew = true;
}

UObject* UKataAssetFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name,
    EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
    UKataAsset* Asset = NewObject<UKataAsset>(InParent, Class, Name, Flags | RF_Transactional);
    Asset->ParentKata = ParentAsset;
#if WITH_EDITORONLY_DATA
    if (ParentAsset)
    {
        Asset->PreviewActorClass = ParentAsset->PreviewActorClass;
        Asset->PreviewTargetClass = ParentAsset->PreviewTargetClass;
        Asset->PreviewActorTransform = ParentAsset->PreviewActorTransform;
        Asset->PreviewTargetTransform = ParentAsset->PreviewTargetTransform;
        Asset->PreviewLightRotation = ParentAsset->PreviewLightRotation;
        Asset->PreviewLightBrightness = ParentAsset->PreviewLightBrightness;
        Asset->PreviewLightColor = ParentAsset->PreviewLightColor;
        Asset->PreviewBackgroundColor = ParentAsset->PreviewBackgroundColor;
        Asset->PreviewEnvironmentSize = ParentAsset->PreviewEnvironmentSize;
        Asset->bPreviewShowDebugShape = ParentAsset->bPreviewShowDebugShape;
        Asset->PreviewDebugShape = ParentAsset->PreviewDebugShape;
        Asset->PreviewDebugColor = ParentAsset->PreviewDebugColor;
        Asset->PreviewDebugThickness = ParentAsset->PreviewDebugThickness;
        Asset->PreviewGridCellSize = ParentAsset->PreviewGridCellSize;
    }
#endif
    return Asset;
}
