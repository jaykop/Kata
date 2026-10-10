#pragma once

#include "AssetDefinitionDefault.h"
#include "Action/KataActionTemplate.h"
#include "AssetDefinition_KataActionTemplate.generated.h"

/** Content Browser의 Kata Action Template 타입과 Kata 액션 에디터 연결을 제공한다. */
UCLASS()
class UAssetDefinition_KataActionTemplate : public UAssetDefinitionDefault
{
    GENERATED_BODY()
public:
    virtual FText GetAssetDisplayName() const override { return NSLOCTEXT("Kata", "TemplateAssetName", "Kata Action Template"); }
    virtual FLinearColor GetAssetColor() const override { return FLinearColor(0.55f, 0.4f, 0.85f); }
    virtual TSoftClassPtr<UObject> GetAssetClass() const override { return UKataActionTemplate::StaticClass(); }
    virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override;
    virtual EAssetCommandResult OpenAssets(const FAssetOpenArgs& OpenArgs) const override;
};
