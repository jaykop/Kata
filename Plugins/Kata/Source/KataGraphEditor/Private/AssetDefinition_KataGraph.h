#pragma once

#include "AssetDefinitionDefault.h"
#include "KataGraph.h"
#include "AssetDefinition_KataGraph.generated.h"

/** Content Browser의 Kata Graph 타입과 전용 에디터 연결을 제공한다. */
UCLASS()
class UAssetDefinition_KataGraph : public UAssetDefinitionDefault
{
    GENERATED_BODY()
public:
    virtual FText GetAssetDisplayName() const override { return NSLOCTEXT("Kata", "GraphAssetName", "Kata Graph"); }
    virtual FLinearColor GetAssetColor() const override { return FLinearColor(0.85f, 0.55f, 0.15f); }
    virtual TSoftClassPtr<UObject> GetAssetClass() const override { return UKataGraph::StaticClass(); }
    virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override;
    virtual EAssetCommandResult OpenAssets(const FAssetOpenArgs& OpenArgs) const override;
};
