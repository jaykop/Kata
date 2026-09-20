#pragma once

#include "AssetDefinitionDefault.h"
#include "Action/KataAction.h"
#include "AssetDefinition_KataAction.generated.h"

/** Content Browser의 Kata 타입과 전용 에디터 연결을 제공한다. */
UCLASS()
class UAssetDefinition_KataAction : public UAssetDefinitionDefault
{
    GENERATED_BODY()
public:
    virtual FText GetAssetDisplayName() const override { return NSLOCTEXT("Kata", "AssetName", "Kata Action"); }
    virtual FLinearColor GetAssetColor() const override { return FLinearColor(0.15f, 0.65f, 0.85f); }
    virtual TSoftClassPtr<UObject> GetAssetClass() const override { return UKataAction::StaticClass(); }
    virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override;
    virtual EAssetCommandResult OpenAssets(const FAssetOpenArgs& OpenArgs) const override;
};
